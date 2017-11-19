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

#include "HIDPP10Device.h"

#include <hidpp10/defs.h>
#include <hidpp10/DeviceInfo.h>
#include <hidpp10/IIndividualFeatures.h>
#include <hidpp10/IResolution.h>
#include <hidpp10/ProfileFormat.h>
#include <hidpp10/RAMMapping.h>
#include <hidpp10/IProfile.h>

HIDPP10Device::HIDPP10Device (HIDPP::Device &&device, const std::vector<std::string> &paths):
	_device (std::move (device)),
	_mouse_info (HIDPP10::getMouseInfo (_device.productID ()))
{
	for (const auto &path: paths)
		_evdev.emplace_back (path);
}

HIDPP10Device::~HIDPP10Device ()
{
	auto dispatcher = _device.dispatcher ();
	for (auto it: _listener_iterators)
		dispatcher->unregisterEventHandler (it);
}

void HIDPP10Device::start ()
{
	// TODO: Add HID++ events

	for (auto &evdev: _evdev)
		evdev.start ();
}

void HIDPP10Device::stop ()
{
	// TODO: Add HID++ events

	for (auto &evdev: _evdev)
		evdev.stop ();
}

InputDevice::Event HIDPP10Device::getEvent (InputDevice::Event event)
{
	throw std::invalid_argument ("invalid event type");
}

int32_t HIDPP10Device::getSimpleEvent (uint16_t type, uint16_t code)
{
	throw std::invalid_argument ("invalid event type");
}

std::string HIDPP10Device::driver () const
{
	return "hidpp10";
}

std::string HIDPP10Device::name () const
{
	return _device.name ();
}

std::string HIDPP10Device::serial () const
{
	return std::string ();
}

std::vector<EventDevice *> HIDPP10Device::getEventDevices ()
{
	std::vector<EventDevice *> devices (_evdev.size ());
	std::transform (_evdev.begin (), _evdev.end (), devices.begin (), [] (auto &dev) { return &dev; });
	return devices;
}

std::vector<uint8_t> HIDPP10Device::setRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size)
{
	std::vector<uint8_t> results (result_size);
	_device.setRegister (address, params, &results);
	return results;
}

std::vector<uint8_t> HIDPP10Device::getRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size)
{
	std::vector<uint8_t> results (result_size);
	_device.getRegister (address, &params, results);
	return results;
}

unsigned int HIDPP10Device::getIndividualFeatureFlags ()
{
	return HIDPP10::IIndividualFeatures (&_device).flags ();
}

void HIDPP10Device::setIndividualFeatureFlags (unsigned int flags)
{
	HIDPP10::IIndividualFeatures (&_device).setFlags (flags);
}

std::vector<unsigned int> HIDPP10Device::getCurrentResolution ()
{
	if (!_mouse_info)
		throw std::runtime_error ("device is not a mouse");

	std::vector<unsigned int> dpi;
	switch (_mouse_info->iresolution_type) {
	case HIDPP10::IResolutionType0:
		dpi.resize (1);
		dpi[0] = HIDPP10::IResolution0 (&_device, _mouse_info->sensor)
				.getCurrentResolution ();
		return dpi;

	case HIDPP10::IResolutionType3:
		dpi.resize (2);
		HIDPP10::IResolution3 (&_device, _mouse_info->sensor)
				.getCurrentResolution (dpi[0], dpi[1]);
		return dpi;

	default:
		throw std::runtime_error ("resolution type not supported");
	}
}

void HIDPP10Device::setCurrentResolution (const std::vector<unsigned int> &dpi)
{
	if (!_mouse_info)
		throw std::runtime_error ("device is not a mouse");

	if (dpi.size () < 1)
		throw std::invalid_argument ("requires at least one dpi value");
	switch (_mouse_info->iresolution_type) {
	case HIDPP10::IResolutionType0:
		HIDPP10::IResolution0 (&_device, _mouse_info->sensor)
				.setCurrentResolution (dpi[0]);
		return;

	case HIDPP10::IResolutionType3:
		HIDPP10::IResolution3 (&_device, _mouse_info->sensor)
				.setCurrentResolution (dpi[0],
						       (dpi.size () < 2 ? dpi[0] : dpi[1]));
		return;

	default:
		throw std::runtime_error ("resolution type not supported");
	}
}

struct SettingConvert
{
	const HIDPP::SettingDesc &desc;

	HIDPP::Setting operator() (bool value)
	{
		if (desc.type () != HIDPP::Setting::Type::Boolean)
			throw std::invalid_argument ("Type not accepted for this setting");
		return value;
	}

	HIDPP::Setting operator() (int value)
	{
		switch (desc.type ()) {
		case HIDPP::Setting::Type::Boolean:
			return value != 0;
		case HIDPP::Setting::Type::Integer:
			return value;
		case HIDPP::Setting::Type::Enum:
			return HIDPP::EnumValue (desc.enumDesc (), value);
		default:
			throw std::invalid_argument ("Type not accepted for this setting");
		}
	}

	HIDPP::Setting operator() (const std::string &value)
	{
		return desc.convertFromString (value);
	}

	HIDPP::Setting operator() (const HIDPP::LEDVector &vec)
	{
		if (desc.type () != HIDPP::Setting::Type::LEDVector)
			throw std::invalid_argument ("Type not accepted for this setting");
		return vec;
	}
};

template <typename T>
struct BitField
{
	T operator() (int value)
	{
		if (value < 0 || value >= (int) (sizeof (T) * 8))
			std::invalid_argument ("Value too big");
		return 1 << value;
	}

	T operator() (const std::vector<int> &values)
	{
		T res = 0;
		for (int value: values) {
			if (value < 0 || value >= (int) (sizeof (T) * 8))
				std::invalid_argument ("Value too big");
			res |= 1 << value;
		}
		return res;
	}
};

void HIDPP10Device::loadTemporaryProfile (const std::map<std::string, setting_t> &settings,
					  const std::vector<std::map<std::string, button_t>> &buttons,
					  const std::vector<std::map<std::string, setting_t>> &modes)
{
	HIDPP::Profile profile;
	auto profile_format = HIDPP10::getProfileFormat (&_device);

	const auto &general_settings = profile_format->generalSettings ();
	for (const auto &[name, value]: settings) {
		auto it = general_settings.find (name);
		if (it == general_settings.end ()) {
			Log::warning () << "Ignoring unknown setting: " << name << std::endl;
			continue;
		}
		SettingConvert convert { it->second };
		profile.settings.emplace (name, std::visit (convert, value));
	}

	for (const auto button_props: buttons) {
		auto &button = profile.buttons.emplace_back ();
		uint8_t modifiers = 0, key = 0;
		for (const auto &[name, value]: button_props) {
			if (name == "mouse-buttons") {
				button.setMouseButtons (std::visit (BitField<uint16_t> (), value));
			}
			else if (name == "key") {
				key = std::get<int> (value);
			}
			else if (name == "modifiers") {
				modifiers = std::visit (BitField<uint8_t> (), value);
			}
			else if (name == "special") {
				button.setSpecial (std::get<int> (value));
			}
			else if (name == "consumer-control") {
				button.setConsumerControl (std::get<int> (value));
			}
			else
				Log::warning () << "Ignoring unknown button value: " << name << std::endl;
		}
		if (key || modifiers)
			button.setKey (modifiers, key);
	}

	const auto &mode_settings = profile_format->modeSettings ();
	for (const auto mode: modes) {
		auto &current = profile.modes.emplace_back ();
		for (const auto &[name, value]: mode) {
			auto it = mode_settings.find (name);
			if (it == mode_settings.end ()) {
				Log::warning () << "Ignoring unknown mode setting: " << name << std::endl;
				continue;
			}
			SettingConvert convert { it->second };
			current.emplace (name, std::visit (convert, value));
		}
	}

	HIDPP10::RAMMapping memory (&_device);
	HIDPP::Address addr { 0, 0, 0 };
	profile_format->write (profile, memory.getWritableIterator (addr));
	memory.sync ();
	HIDPP10::IProfile (&_device).loadProfileFromAddress (addr);
}

void HIDPP10Device::loadProfileFromIndex (unsigned int index)
{
	HIDPP10::IProfile (&_device).loadProfileFromIndex (index);
}

const JSClass HIDPP10Device::js_class = jstpl::make_class<HIDPP10Device> ("HIDPP10Device");

const JSFunctionSpec HIDPP10Device::js_fs[] = {
	jstpl::make_method<&HIDPP10Device::getEventDevices> ("getEventDevices"),
	jstpl::make_method<&HIDPP10Device::setRegister> ("setRegister"),
	jstpl::make_method<&HIDPP10Device::getRegister> ("getRegister"),
	jstpl::make_method<&HIDPP10Device::getIndividualFeatureFlags> ("getIndividualFeatureFlags"),
	jstpl::make_method<&HIDPP10Device::setIndividualFeatureFlags> ("setIndividualFeatureFlags"),
	jstpl::make_method<&HIDPP10Device::getCurrentResolution> ("getCurrentResolution"),
	jstpl::make_method<&HIDPP10Device::setCurrentResolution> ("setCurrentResolution"),
	jstpl::make_method<&HIDPP10Device::loadTemporaryProfile> ("loadTemporaryProfile"),
	jstpl::make_method<&HIDPP10Device::loadProfileFromIndex> ("loadProfileFromIndex"),
	JS_FS_END
};

#define DEFINE_HIDPP_REGISTER_ADDRESS(name) { #name, HIDPP10::name }
#define DEFINE_HIDPP_CONSTANT(interface_name, value_name) { \
	#interface_name "_" #value_name, \
	HIDPP10::I##interface_name::value_name \
}
#define DEFINE_HIDPP_ENUM(interface_name, enum_name, value_name) { \
	#interface_name "_" #enum_name "_" #value_name, \
	HIDPP10::I##interface_name::enum_name::value_name \
}
#define DEFINE_ENUM(type, value) { #type "_" #value, static_cast<int> (type::value) }
const std::pair<std::string, int> HIDPP10Device::js_int_const[] = {
	DEFINE_HIDPP_REGISTER_ADDRESS(EnableNotifications),
	DEFINE_HIDPP_REGISTER_ADDRESS(EnableIndividualFeatures),
	DEFINE_HIDPP_REGISTER_ADDRESS(ConnectionState),
	DEFINE_HIDPP_REGISTER_ADDRESS(BatteryStatus),
	DEFINE_HIDPP_REGISTER_ADDRESS(BatteryMileage),
	DEFINE_HIDPP_REGISTER_ADDRESS(CurrentProfile),
	DEFINE_HIDPP_REGISTER_ADDRESS(LEDStatus),
	DEFINE_HIDPP_REGISTER_ADDRESS(LEDIntensity),
	DEFINE_HIDPP_REGISTER_ADDRESS(LEDColor),
	DEFINE_HIDPP_REGISTER_ADDRESS(SensorSettings),
	DEFINE_HIDPP_REGISTER_ADDRESS(SensorResolution),
	DEFINE_HIDPP_REGISTER_ADDRESS(USBPollRate),
	DEFINE_HIDPP_REGISTER_ADDRESS(MemoryOperation),
	DEFINE_HIDPP_REGISTER_ADDRESS(ResetSeqNum),
	DEFINE_HIDPP_REGISTER_ADDRESS(MemoryRead),
	DEFINE_HIDPP_REGISTER_ADDRESS(DevicePairing),
	DEFINE_HIDPP_REGISTER_ADDRESS(DeviceActivity),
	DEFINE_HIDPP_REGISTER_ADDRESS(DevicePairingInfo),
	DEFINE_HIDPP_REGISTER_ADDRESS(FirmwareInfo),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, SpecialButtonFunction),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, EnhancedKeyUsage),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, FastForwardRewind),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, ScrollingAcceleration),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, ButtonsControlResolution),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, InhibitLockKeySound),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, MXAir3DEngine),
	DEFINE_HIDPP_CONSTANT(IndividualFeatures, LEDControl),
	DEFINE_ENUM(Modifier, LeftControl),
	DEFINE_ENUM(Modifier, LeftShift),
	DEFINE_ENUM(Modifier, LeftAlt),
	DEFINE_ENUM(Modifier, LeftMeta),
	DEFINE_ENUM(Modifier, RightControl),
	DEFINE_ENUM(Modifier, RightShift),
	DEFINE_ENUM(Modifier, RightAlt),
	DEFINE_ENUM(Modifier, RightMeta),
	DEFINE_ENUM(SpecialAction, WheelLeft),
	DEFINE_ENUM(SpecialAction, WheelRight),
	DEFINE_ENUM(SpecialAction, BatteryLevel),
	DEFINE_ENUM(SpecialAction, ResolutionNext),
	DEFINE_ENUM(SpecialAction, ResolutionCycleNext),
	DEFINE_ENUM(SpecialAction, ResolutionPrev),
	DEFINE_ENUM(SpecialAction, ResolutionCyclePrev),
	DEFINE_ENUM(SpecialAction, ProfileNext),
	DEFINE_ENUM(SpecialAction, ProfileCycleNext),
	DEFINE_ENUM(SpecialAction, ProfilePrev),
	DEFINE_ENUM(SpecialAction, ProfileCyclePrev),
	DEFINE_ENUM(SpecialAction, ProfileSwitch0),
	DEFINE_ENUM(SpecialAction, ProfileSwitch1),
	DEFINE_ENUM(SpecialAction, ProfileSwitch2),
	DEFINE_ENUM(SpecialAction, ProfileSwitch3),
	DEFINE_ENUM(SpecialAction, ProfileSwitch4),
	{ "", 0 }
};

JSObject *HIDPP10Device::makeJsObject (const jstpl::Thread *thread)
{
	return thread->makeJsObject (this);
}

bool HIDPP10Device::_registered = jstpl::ClassManager::registerClass<HIDPP10Device::JsClass> ("InputDevice");
