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

const JSClass HIDPP10Device::js_class = jstpl::make_class<HIDPP10Device> ("HIDPP10Device");

const JSFunctionSpec HIDPP10Device::js_fs[] = {
	jstpl::make_method<&HIDPP10Device::getEventDevices> ("getEventDevices"),
	jstpl::make_method<&HIDPP10Device::setRegister> ("setRegister"),
	jstpl::make_method<&HIDPP10Device::getRegister> ("getRegister"),
	jstpl::make_method<&HIDPP10Device::getIndividualFeatureFlags> ("getIndividualFeatureFlags"),
	jstpl::make_method<&HIDPP10Device::setIndividualFeatureFlags> ("setIndividualFeatureFlags"),
	jstpl::make_method<&HIDPP10Device::getCurrentResolution> ("getCurrentResolution"),
	jstpl::make_method<&HIDPP10Device::setCurrentResolution> ("setCurrentResolution"),
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
#define DEFINE_CONSTANT(value) { #value, value }
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
	{ "", 0 }
};

JSObject *HIDPP10Device::makeJsObject (const jstpl::Thread *thread)
{
	return thread->makeJsObject (this);
}

bool HIDPP10Device::_registered = jstpl::ClassManager::registerClass<HIDPP10Device::JsClass> ("InputDevice");
