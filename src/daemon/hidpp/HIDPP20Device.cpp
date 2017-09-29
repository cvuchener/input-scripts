/*
 * Copyright 2017 Clément Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "HIDPP20Device.h"

#include "../ClassManager.h"

#include <hidpp20/IFeatureSet.h>
#include <hidpp20/IMouseButtonSpy.h>
#include <hidpp20/IOnboardProfiles.h>
#include <hidpp20/IReprogControlsV4.h>
#include <hidpp20/UnsupportedFeature.h>

struct HIDPP20Device::MouseButtonSpy
{
	HIDPP20::IMouseButtonSpy i;
	uint16_t buttons;

	MouseButtonSpy (HIDPP20::Device *device):
		i (device)
	{
		buttons = 0;
	}
};

struct HIDPP20Device::OnboardProfiles
{
	HIDPP20::IOnboardProfiles i;
	HIDPP20::IOnboardProfiles::MemoryType current_profile_mem_type;
	unsigned int current_profile_page, current_dpi_index;

	OnboardProfiles (HIDPP20::Device *device):
		i (device)
	{
		std::tie (current_profile_mem_type,
			  current_profile_page) = i.getCurrentProfile ();
		current_dpi_index = i.getCurrentDPIIndex ();
	}
};

struct HIDPP20Device::ReprogControlsV4
{
	HIDPP20::IReprogControlsV4 i;
	std::vector<uint16_t> buttons;

	ReprogControlsV4 (HIDPP20::Device *device):
		i (device)
	{
	}
};

HIDPP20Device::HIDPP20Device (HIDPP::Device &&device):
	_device (std::move (device)),
	_send_raw_events (false)
{
	auto index = _device.deviceIndex ();
	auto dispatcher = _device.dispatcher ();
	auto event_handler = [this] (const HIDPP::Report &report) {
		_report_queue.push (report);
		return true;
	};
	HIDPP20::IFeatureSet feature_set (&_device);
	unsigned int feature_count = feature_set.getCount ();
	for (unsigned int i = 0; i < feature_count; ++i) {
		bool hidden;
		uint16_t id = feature_set.getFeatureID (i, nullptr, &hidden, nullptr);
		if (hidden)
			continue;
		_features_id_index.emplace (id, i);
		_features_index_id.emplace (i, id);
		auto it = dispatcher->registerEventHandler (index, i, event_handler);
		_listener_iterators.push_back (it);
	}

	if (hasFeature (HIDPP20::IMouseButtonSpy::ID))
		_mbs = std::make_unique<MouseButtonSpy> (&_device);
	if (hasFeature (HIDPP20::IOnboardProfiles::ID))
		_op = std::make_unique<OnboardProfiles> (&_device);
	if (hasFeature (HIDPP20::IReprogControlsV4::ID))
		_rc4 = std::make_unique<ReprogControlsV4> (&_device);
}

HIDPP20Device::~HIDPP20Device ()
{
	auto dispatcher = _device.dispatcher ();
	for (auto it: _listener_iterators)
		dispatcher->unregisterEventHandler (it);
}

void HIDPP20Device::interrupt ()
{
	_report_queue.interrupt ();
}

void HIDPP20Device::readEvents ()
{
	_report_queue.resetInterruption ();
	while (auto opt = _report_queue.pop ()) {
		HIDPP::Report &report = opt.value ();
		if (_mbs && report.featureIndex () == _mbs->i.index ()) {
			switch (report.function ()) {
			case HIDPP20::IMouseButtonSpy::MouseButtonEvent: {
				uint16_t new_buttons = HIDPP20::IMouseButtonSpy::mouseButtonEvent (report);
				uint16_t button_changed = _mbs->buttons ^ new_buttons;
				_mbs->buttons = new_buttons;
				for (unsigned int i = 0; i < 16; ++i) {
					if (!(button_changed & (1<<i)))
						continue;
					eventRead ({
						{ "type", EV_KEY },
						{ "code", BTN_MOUSE + i },
						{ "value", (new_buttons & (1<<i) ? 1 : 0) },
					});
				}
				break;
			}
			default:
				Log::warning () << "Unsupported event from MouseButtonSpy: " << report.function () << std::endl;
			}
		}
		else if (_op && report.featureIndex () == _op->i.index ()) {
			switch (report.function ()) {
			case HIDPP20::IOnboardProfiles::CurrentProfileChanged:
				std::tie (_op->current_profile_mem_type,
					  _op->current_profile_page) = HIDPP20::IOnboardProfiles::currentProfileChanged (report);
				eventRead ({
					{ "type", EventOnboardProfilesCurrentProfile },
					{ "rom", _op->current_profile_mem_type },
					{ "index", _op->current_profile_page },
				});
				break;
			case HIDPP20::IOnboardProfiles::CurrentDPIIndexChanged:
				_op->current_dpi_index = HIDPP20::IOnboardProfiles::currentDPIIndexChanged (report);
				eventRead ({
					{ "type", EventOnboardProfilesCurrentDPIIndex },
					{ "index", _op->current_dpi_index },
				});
				break;
			default:
				Log::warning () << "Unsupported event from OnboardProfiles: " << report.function () << std::endl;
			}
		}
		else if (_rc4 && report.featureIndex () == _rc4->i.index ()) {
			switch (report.function ()) {
			case HIDPP20::IReprogControlsV4::DivertedButtonEvent: {
				auto new_buttons = HIDPP20::IReprogControlsV4::divertedButtonEvent (report);
				unsigned int old_i = 0, new_i = 0;
				for (; old_i < _rc4->buttons.size (); ++old_i) {
					if (new_i >= new_buttons.size () ||
					    _rc4->buttons[old_i] != new_buttons[new_i]) {
						// An old button is missing, it was released.
						eventRead ({
							{ "type", EventReprogControlsV4Button },
							{ "code", _rc4->buttons[old_i] },
							{ "value", 0 },
						});
					}
					else {
						// Same button in both array: no change.
						++new_i;
					}
				}
				for (; new_i < new_buttons.size (); ++new_i) {
					// New buttons that were not in the old vector are pressed.
					eventRead ({
						{ "type", EventReprogControlsV4Button },
						{ "code", new_buttons[new_i] },
						{ "value", 1 },
					});
				}
				_rc4->buttons = std::move (new_buttons);
				break;
			}

			case HIDPP20::IReprogControlsV4::DivertedRawXYEvent: {
				auto move = HIDPP20::IReprogControlsV4::divertedRawXYEvent (report);
				eventRead ({
					{ "type", EventReprogControlsV4RawXY },
					{ "x", move.x },
					{ "y", move.y },
				});
				break;
			}
			}

		}

		if (_send_raw_events) {
			HIDPP20::IFeatureSet feature_set (&_device);
			auto feature = _features_index_id.find (report.featureIndex ());
			if (feature == _features_index_id.end ()) {
				Log::error () << "Received HID++ report with unknown feature index: " << report.featureIndex () << std::endl;
				continue;
			}
			Event event = {
				{ "type", EventRawHIDPP },
				{ "feature", feature->second },
				{ "function", report.function () },
			};
			unsigned int i = 0;
			for (auto it = report.parameterBegin ();
			     it != report.parameterEnd ();
			     ++i, ++it) {
				std::stringstream key;
				key << "data" << i;
				event.emplace (key.str (), *it);
			}
			eventRead (event);
		}
	}
}

InputDevice::Event HIDPP20Device::getEvent (InputDevice::Event event)
{
	int type = event.at ("type");
	int code;
	switch (type) {
	case EV_KEY:
		code = event.at ("code");
		if (code >= BTN_MOUSE && code < BTN_MOUSE+16)
			event["value"] = (_mbs->buttons & (1<<(code-BTN_MOUSE)) ? 1 : 0);
		return event;

	case EventOnboardProfilesCurrentProfile:
		event["rom"] = _op->current_profile_mem_type;
		event["index"] = _op->current_profile_page;
		return event;

	case EventOnboardProfilesCurrentDPIIndex:
		event["index"] = _op->current_dpi_index;
		return event;

	case EventRawHIDPP:
		throw std::invalid_argument ("raw HID++ cannot be queried");

	case EventReprogControlsV4Button: {
		code = event.at ("code");
		auto it = std::find (_rc4->buttons.begin (), _rc4->buttons.end (), code);
		event["value"] = (it == _rc4->buttons.end () ? 0 : 1);
		return event;
	}

	default:
		throw std::invalid_argument ("invalid event type");
	}
}

std::string HIDPP20Device::driver () const
{
	return "hidpp20";
}

std::string HIDPP20Device::name () const
{
	return _device.name ();
}

std::string HIDPP20Device::serial () const
{
	return std::string ();
}

HIDPP20Device::operator bool () const
{
	// The device is always valid, the driver will remove it if there is an error.
	return true;
}

bool HIDPP20Device::hasFeature (uint16_t feature_id) const
{
	return _features_id_index.find (feature_id) != _features_id_index.end ();
}

std::vector<uint8_t> HIDPP20Device::callFunction (uint16_t feature_id, unsigned int function, const std::vector<uint8_t> &params)
{
	auto feature = _features_id_index.find (feature_id);
	if (feature == _features_id_index.end ())
		throw HIDPP20::UnsupportedFeature (feature_id, "?");
	return _device.callFunction (feature->second, function, params);
}

void HIDPP20Device::sendRawEvents (bool enable)
{
	_send_raw_events = enable;
}

#define ASSERT_FEATURE(interface_name, var) \
	do { \
		if (!var) \
			throw HIDPP20::UnsupportedFeature (HIDPP20::I##interface_name::ID, \
							   #interface_name); \
	} while (false)

unsigned int HIDPP20Device::MouseButtonSpy_GetButtonCount ()
{
	ASSERT_FEATURE (MouseButtonSpy, _mbs);
	return _mbs->i.getMouseButtonCount ();
}

void HIDPP20Device::MouseButtonSpy_Start ()
{
	ASSERT_FEATURE (MouseButtonSpy, _mbs);
	_mbs->i.startMouseButtonSpy ();
}

void HIDPP20Device::MouseButtonSpy_Stop ()
{
	ASSERT_FEATURE (MouseButtonSpy, _mbs);
	_mbs->i.stopMouseButtonSpy ();
}

std::vector<uint8_t> HIDPP20Device::MouseButtonSpy_GetMapping ()
{
	ASSERT_FEATURE (MouseButtonSpy, _mbs);
	return _mbs->i.getMouseButtonMapping ();
}

void HIDPP20Device::MouseButtonSpy_SetMapping (const std::vector<uint8_t> &button_mapping)
{
	ASSERT_FEATURE (MouseButtonSpy, _mbs);
	_mbs->i.setMouseButtonMapping (button_mapping);
}

bool HIDPP20Device::OnboardProfiles_GetHostMode ()
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	return _op->i.getMode () == HIDPP20::IOnboardProfiles::Mode::Host;
}

void HIDPP20Device::OnboardProfiles_SetHostMode (bool host_mode)
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	_op->i.setMode (host_mode
			? HIDPP20::IOnboardProfiles::Mode::Host
			: HIDPP20::IOnboardProfiles::Mode::Onboard);
}

std::map<std::string, unsigned int> HIDPP20Device::OnboardProfiles_GetCurrentProfile ()
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	auto current_profile =  _op->i.getCurrentProfile ();
	return {
		{ "rom", std::get<0> (current_profile) },
		{ "index", std::get<1> (current_profile) }
	};
}

void HIDPP20Device::OnboardProfiles_SetCurrentProfile (bool rom, unsigned int index)
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	_op->i.setCurrentProfile (rom
				  ? HIDPP20::IOnboardProfiles::ROM
				  : HIDPP20::IOnboardProfiles::Writeable,
				  index);
}

unsigned int HIDPP20Device::OnboardProfiles_GetCurrentDPIIndex ()
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	return _op->i.getCurrentDPIIndex ();
}

void HIDPP20Device::OnboardProfiles_SetCurrentDPIIndex (unsigned int index)
{
	ASSERT_FEATURE (OnboardProfiles, _op);
	_op->i.setCurrentDPIIndex (index);
}

unsigned int HIDPP20Device::ReprogControlsV4_GetControlCount ()
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	return _rc4->i.getControlCount ();
}

std::map<std::string, int> HIDPP20Device::ReprogControlsV4_GetControlInfo (unsigned int index)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	auto info =  _rc4->i.getControlInfo (index);
	return std::map<std::string, int> {
		{ "control_id", info.control_id },
		{ "task_id", info.task_id },
		{ "mouse_button", info.flags & HIDPP20::IReprogControlsV4::MouseButton },
		{ "fkey", info.flags & HIDPP20::IReprogControlsV4::FKey },
		{ "hotkey", info.flags & HIDPP20::IReprogControlsV4::HotKey },
		{ "fntoggle", info.flags & HIDPP20::IReprogControlsV4::FnToggle },
		{ "reprog", info.flags & HIDPP20::IReprogControlsV4::ReprogHint },
		{ "divert", info.flags & HIDPP20::IReprogControlsV4::TemporaryDivertable },
		{ "persist", info.flags & HIDPP20::IReprogControlsV4::PersistentDivertable },
		{ "virtual", info.flags & HIDPP20::IReprogControlsV4::Virtual },
		{ "rawxy", info.additional_flags & HIDPP20::IReprogControlsV4::RawXY },
		{ "pos", info.pos },
		{ "group", info.group },
		{ "group_mask", info.group_mask },
	};
}

uint16_t HIDPP20Device::ReprogControlsV4_GetRemap (uint16_t control_id)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags;
	return _rc4->i.getControlReporting (control_id, flags);
}

void HIDPP20Device::ReprogControlsV4_SetRemap (uint16_t control_id, uint16_t remap)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	_rc4->i.setControlReporting (control_id, 0, remap);
}

bool HIDPP20Device::ReprogControlsV4_GetDivert (uint16_t control_id)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags;
	_rc4->i.getControlReporting (control_id, flags);
	return flags & HIDPP20::IReprogControlsV4::TemporaryDiverted;
}

void HIDPP20Device::ReprogControlsV4_SetDivert (uint16_t control_id, bool divert)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags = HIDPP20::IReprogControlsV4::ChangeTemporaryDivert;
	if (divert)
		flags |= HIDPP20::IReprogControlsV4::TemporaryDiverted;
	_rc4->i.setControlReporting (control_id, flags, 0);
}

bool HIDPP20Device::ReprogControlsV4_GetPersistentDivert (uint16_t control_id)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags;
	_rc4->i.getControlReporting (control_id, flags);
	return flags & HIDPP20::IReprogControlsV4::PersistentDiverted;
}

void HIDPP20Device::ReprogControlsV4_SetPersistentDivert (uint16_t control_id, bool divert)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags = HIDPP20::IReprogControlsV4::ChangePersistentDivert;
	if (divert)
		flags |= HIDPP20::IReprogControlsV4::PersistentDiverted;
	_rc4->i.setControlReporting (control_id, flags, 0);
}

bool HIDPP20Device::ReprogControlsV4_GetRawXYDivert (uint16_t control_id)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags;
	_rc4->i.getControlReporting (control_id, flags);
	return flags & HIDPP20::IReprogControlsV4::RawXYDiverted;
}

void HIDPP20Device::ReprogControlsV4_SetRawXYDivert (uint16_t control_id, bool divert)
{
	ASSERT_FEATURE (ReprogControlsV4, _rc4);
	uint8_t flags = HIDPP20::IReprogControlsV4::ChangeRawXYDivert;
	if (divert)
		flags |= HIDPP20::IReprogControlsV4::RawXYDiverted;
	_rc4->i.setControlReporting (control_id, flags, 0);
}

const JSClass HIDPP20Device::js_class = JS_HELPERS_CLASS("HIDPP20Device", HIDPP20Device);

#define HIDPP_JS_WRAPPER(interface_name, method_name) JS_HELPERS_METHOD(#interface_name "_" #method_name, HIDPP20Device::interface_name##_##method_name)
const JSFunctionSpec HIDPP20Device::js_fs[] = {
	JS_HELPERS_METHOD("hasFeature", HIDPP20Device::hasFeature),
	JS_HELPERS_METHOD("callFunction", HIDPP20Device::callFunction),
	JS_HELPERS_METHOD("sendRawEvents", HIDPP20Device::sendRawEvents),
	HIDPP_JS_WRAPPER(MouseButtonSpy, GetButtonCount),
	HIDPP_JS_WRAPPER(MouseButtonSpy, Start),
	HIDPP_JS_WRAPPER(MouseButtonSpy, Stop),
	HIDPP_JS_WRAPPER(MouseButtonSpy, GetMapping),
	HIDPP_JS_WRAPPER(MouseButtonSpy, SetMapping),
	HIDPP_JS_WRAPPER(OnboardProfiles, GetHostMode),
	HIDPP_JS_WRAPPER(OnboardProfiles, SetHostMode),
	HIDPP_JS_WRAPPER(OnboardProfiles, GetCurrentProfile),
	HIDPP_JS_WRAPPER(OnboardProfiles, SetCurrentProfile),
	HIDPP_JS_WRAPPER(OnboardProfiles, GetCurrentDPIIndex),
	HIDPP_JS_WRAPPER(OnboardProfiles, SetCurrentDPIIndex),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetControlCount),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetControlInfo),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetRemap),
	HIDPP_JS_WRAPPER(ReprogControlsV4, SetRemap),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetDivert),
	HIDPP_JS_WRAPPER(ReprogControlsV4, SetDivert),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetPersistentDivert),
	HIDPP_JS_WRAPPER(ReprogControlsV4, SetPersistentDivert),
	HIDPP_JS_WRAPPER(ReprogControlsV4, GetRawXYDivert),
	HIDPP_JS_WRAPPER(ReprogControlsV4, SetRawXYDivert),
	JS_FS_END
};

#define DEFINE_HIDPP_FEATURE(interface_name) { #interface_name, HIDPP20::I##interface_name::ID }
#define DEFINE_HIDPP_CONSTANT(interface_name, enum_name, value_name) { \
	#interface_name "_" #enum_name "_" #value_name, \
	HIDPP20::I##interface_name::enum_name::value_name \
}
#define DEFINE_CONSTANT(value) { #value, value }
const std::pair<std::string, int> HIDPP20Device::js_int_const[] = {
	DEFINE_HIDPP_FEATURE(MouseButtonSpy),
	DEFINE_HIDPP_FEATURE(OnboardProfiles),
	DEFINE_HIDPP_FEATURE(ReprogControlsV4),
	DEFINE_CONSTANT(EventRawHIDPP),
	DEFINE_CONSTANT(EventOnboardProfilesCurrentProfile),
	DEFINE_CONSTANT(EventOnboardProfilesCurrentDPIIndex),
	DEFINE_CONSTANT(EventReprogControlsV4Button),
	DEFINE_CONSTANT(EventReprogControlsV4RawXY),
	{ "", 0 }
};

JSObject *HIDPP20Device::makeJsObject (const JsHelpers::Thread *thread)
{
	return thread->makeJsObject (this);
}

bool HIDPP20Device::_registered = ClassManager::registerClass<HIDPP20Device::JsClass> ("InputDevice");
