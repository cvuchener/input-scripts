/*
 * Copyright 2016 Clément Vuchener
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

#include "SteamControllerDevice.h"

#include "SteamControllerReceiver.h"

#include "SteamControllerProtocol.h"
using namespace SteamController;

#include "../Log.h"

extern "C" {
#include <unistd.h>
#include <fcntl.h>
}

SteamControllerDevice::SteamControllerDevice (SteamControllerReceiver *receiver):
	_receiver (receiver),
	_report_single_axis (true),
	_report_linked_axes (false)
{
	receiver->inputReport = [this] (const std::array<uint8_t, 64> &report) {
		_report_queue.push (report);
	};
	_serial = querySerial ();
}

SteamControllerDevice::~SteamControllerDevice ()
{
	_receiver->inputReport = std::function<void (const std::array<uint8_t, 64> &)> ();
}

void SteamControllerDevice::interrupt ()
{
	_stop = true;
	_report_queue.push (std::array<uint8_t, 64> ());
}

template <typename T, unsigned int size = sizeof (T)>
static typename std::enable_if<std::is_integral<T>::value, T>::type
readLE (uint8_t *buffer) {
	T value = 0;
	for (unsigned int i = 0; i < size; ++i) {
		value |= buffer[i] << (8*i);
	}
	return value;
}

template <typename T, unsigned int size = sizeof (T)>
static typename std::enable_if<std::is_integral<T>::value>::type
writeLE (uint8_t *buffer, T value) {
	for (unsigned int i = 0; i < size; ++i) {
		buffer[i] = (value >> (8*i)) & 0xFF;
	}
}

void SteamControllerDevice::readEvents ()
{
	_stop = false;
	while (!_stop) {
		std::array<uint8_t, 64> report = _report_queue.pop ();
		if (_stop)
			break;
		//uint8_t type = report[2];
		//uint8_t length = report[3];
		//uint32_t seq = readLE<uint32_t> (&report[4]);
		uint32_t buttons = readLE<uint32_t, 3> (&report[8]);
		uint8_t triggers[2] = { report[11], report[12] };
		int16_t touchpad[2][2] = {
			{ // Left touchpad/stick
				readLE<int16_t> (&report[16]),
				readLE<int16_t> (&report[18])
			},
			{ // Right touchpad
				readLE<int16_t> (&report[20]),
				readLE<int16_t> (&report[22])
			}
		};
		// 16 bits triggers are not available on wireless devices
		//int16_t triggers_16[2] = {
		//	readLE<int16_t> (&report[24]),
		//	readLE<int16_t> (&report[26])
		//};
		int16_t accel[3] = {
			readLE<int16_t> (&report[28]),
			readLE<int16_t> (&report[30]),
			readLE<int16_t> (&report[32])
		};
		int16_t gyro[3] = {
			readLE<int16_t> (&report[34]),
			readLE<int16_t> (&report[36]),
			readLE<int16_t> (&report[38])
		};
		int16_t quaternion[4] = {
			readLE<int16_t> (&report[40]),
			readLE<int16_t> (&report[42]),
			readLE<int16_t> (&report[44]),
			readLE<int16_t> (&report[46])
		};

		bool changed = false;
		// Touchpads (and stick)
		bool touchpad_changed[2] = { false };
		static constexpr uint16_t AxisCodes[2][2] = {
			{ AbsLeftX, AbsLeftY },
			{ AbsRightX, AbsRightY }
		};
		static constexpr uint16_t TouchPadCodes[2] = { TouchPadLeft, TouchPadRight };
		for (unsigned int i = 0; i < 2; ++i) {
			for (unsigned int j = 0; j < 2; ++j) {
				if (std::abs (_state.touchpad[i][j]-touchpad[i][j]) > AxisFuzz) {
					_state.touchpad[i][j] = touchpad[i][j];
					if (_report_single_axis) {
						eventRead ({
							{ "type", EventAbs },
							{ "code", AxisCodes[i][j] },
							{ "value", touchpad[i][j] }
						});
					}
					touchpad_changed[i] = changed = true;
				}
			}
			if (touchpad_changed[i] && _report_linked_axes) {
				eventRead ({
					{ "type", EventTouchPad },
					{ "code", TouchPadCodes[i] },
					{ "x", _state.touchpad[i][0] },
					{ "y", _state.touchpad[i][1] }
				});
			}
		}
		// Triggers
		for (unsigned int i = 0; i < 2; ++i) {
			if (std::abs (_state.triggers[i]-triggers[i]) > TriggerFuzz) {
				_state.triggers[i] = triggers[i];
				eventRead ({
					{ "type", EventAbs },
					{ "code", AbsLeftTrigger+i },
					{ "value", triggers[i] }
				});
				changed = true;
			}
		}
		// Sensor events
		bool accel_changed = false, gyro_changed = false;
		for (unsigned int i = 0; i < 3; ++i) {
			if (std::abs (_state.accel[i]-accel[i]) > SensorFuzz) {
				_state.accel[i] = accel[i];
				accel_changed = true;
			}
			if (std::abs (_state.gyro[i]-gyro[i]) > SensorFuzz) {
				_state.gyro[i] = gyro[i];
				gyro_changed = true;
			}
		}
		if (accel_changed) {
			eventRead ({
				{ "type", EventSensor },
				{ "code", SensorAccel },
				{ "x", accel[0] },
				{ "y", accel[1] },
				{ "z", accel[2] }
			});
			changed = true;
		}
		if (gyro_changed) {
			eventRead ({
				{ "type", EventSensor },
				{ "code", SensorGyro },
				{ "x", gyro[0] },
				{ "y", gyro[1] },
				{ "z", gyro[2] }
			});
			changed = true;
		}
		// Orientation quaternion
		bool q_changed = false;
		for (unsigned int i = 0; i < 4; ++i) {
			if (std::abs (_state.quaternion[i]-quaternion[i]) > SensorFuzz) {
				_state.quaternion[i] = quaternion[i];
				q_changed = true;
			}
		}
		if (q_changed) {
			eventRead ({
				{ "type", EventOrientation },
				{ "w", quaternion[0] },
				{ "x", quaternion[1] },
				{ "y", quaternion[2] },
				{ "z", quaternion[3] }
			});
			changed = true;
		}
		// Buttons
		uint32_t buttons_diff = buttons ^ _state.buttons;
		_state.buttons = buttons;
		for (unsigned int i = 0; i < BtnCount; ++i) {
			if (buttons_diff & (1<<i)) {
				eventRead ({
					{ "type", EventBtn },
					{ "code", i },
					{ "value", (buttons & (1<<i) ? 1 : 0) }
				});
				changed = true;
			}
		}
		// Send SYN event only when something else was sent
		if (changed) {
			eventRead ({
				{ "type", EV_SYN },
				{ "code", SYN_REPORT },
				{ "value", 0 }
			});
		}
	}
}

InputDevice::Event SteamControllerDevice::getEvent (InputDevice::Event event)
{
	int type = event["type"];
	int code;
	switch (type) {
	case EventBtn:
		code = event["code"];
		if (code < 0 || code >= BtnCount)
			throw std::invalid_argument ("invalid button code");
		event["value"] = (_state.buttons & (1<<code)) >> code;
		return event;

	case EventAbs:
		code = event["code"];
		switch (code) {
		case AbsLeftX:
			event["value"] =  _state.touchpad[0][0];
			return event;
		case AbsLeftY:
			event["value"] =  _state.touchpad[0][1];
			return event;
		case AbsRightX:
			event["value"] =  _state.touchpad[1][0];
			return event;
		case AbsRightY:
			event["value"] =  _state.touchpad[1][1];
			return event;
		case AbsLeftTrigger:
			event["value"] =  _state.triggers[0];
			return event;
		case AbsRightTrigger:
			event["value"] =  _state.triggers[1];
			return event;
		default:
			throw std::invalid_argument ("invalid abs axis code");
		}

	case EventSensor:
		code = event["code"];
		switch (code) {
		case SensorAccel:
			event["x"] = _state.accel[0];
			event["y"] = _state.accel[1];
			event["z"] = _state.accel[2];
			return event;
		case SensorGyro:
			event["x"] = _state.gyro[0];
			event["y"] = _state.gyro[1];
			event["z"] = _state.gyro[2];
			return event;
		default:
			throw std::invalid_argument ("invalid sensor code");
		}

	case EventOrientation:
		event["w"] = _state.quaternion[0];
		event["x"] = _state.quaternion[1];
		event["y"] = _state.quaternion[2];
		event["z"] = _state.quaternion[3];
		return event;

	default:
		throw std::invalid_argument ("invalid event type");
	}
}

std::string SteamControllerDevice::driver () const
{
	return "steamcontroller";
}

std::string SteamControllerDevice::name () const
{
	return _receiver->name ();
}

std::string SteamControllerDevice::serial () const
{
	return _serial;
}

void SteamControllerDevice::setTouchPadEventMode (bool report_single_axis, bool report_linked_axes)
{
	_report_single_axis = report_single_axis;
	_report_linked_axes = report_linked_axes;
}

void SteamControllerDevice::setSetting (uint8_t setting, uint16_t value)
{
	std::vector<uint8_t> params (3);
	params[0] = setting;
	writeLE (&params[1], value);
	_receiver->sendRequest (RequestConfigure, params);
}

void SteamControllerDevice::enableKeys ()
{
	_receiver->sendRequest (RequestEnableKeys, {});
}

void SteamControllerDevice::disableKeys ()
{
	_receiver->sendRequest (RequestDisableKeys, {});
}

void SteamControllerDevice::enableMouse ()
{
	_receiver->sendRequest (RequestEnableMouse, {});
}

void SteamControllerDevice::hapticFeedback (uint8_t actuator, uint16_t amplitude, uint16_t period, uint16_t count)
{
	std::vector<uint8_t> params (7);
	params[0] = actuator;
	writeLE (&params[1], amplitude);
	writeLE (&params[3], period);
	writeLE (&params[5], count);
	_receiver->sendRequest (RequestHapticFeedback, params);
}

std::string SteamControllerDevice::querySerial ()
{
	std::vector<uint8_t> params ({ControllerSerial}), results;
	//params[0] = ControllerSerial;
	do {
		try {
			_receiver->sendRequest (RequestGetSerial, params, &results);
		}
		catch (std::runtime_error e) {
			Log::warning () << "In " << __PRETTY_FUNCTION__ << ": "
					<< e.what () << std::endl;
		}
	} while (results.size () < 1 || results[0] != ControllerSerial);
	results.push_back (0); // Make sure the string is null-terminated
	return std::string (reinterpret_cast<char *> (&results[1]));
}

void SteamControllerDevice::setSounds (const std::vector<int8_t> &sounds)
{
	std::vector<uint8_t> params (sounds.begin (), sounds.end ());
	params.resize (16, -1);
	_receiver->sendRequest (RequestSetSounds, params);
}

void SteamControllerDevice::playSound (int id)
{
	std::vector<uint8_t> params (4);
	writeLE<int32_t> (&params[0], id);
	_receiver->sendRequest (RequestPlaySound, params);
}

void SteamControllerDevice::reset ()
{
	_receiver->sendRequest (RequestReset, {});
}

void SteamControllerDevice::shutdown ()
{
	_receiver->sendRequest (RequestShutdown, { 'o', 'f', 'f', '!' });
}

void SteamControllerDevice::calibrateTouchPads ()
{
	_receiver->sendRequest (RequestCalibrateTouchPads, {});
}

void SteamControllerDevice::calibrateSensors ()
{
	_receiver->sendRequest (RequestCalibrateSensors, {});
}

void SteamControllerDevice::calibrateJoystick ()
{
	_receiver->sendRequest (RequestCalibrateJoystick, {});
}

SteamControllerDevice::operator bool () const
{
	return _receiver->isConnected ();
}

const JSClass SteamControllerDevice::js_class = JS_HELPERS_CLASS("SteamControllerDevice", SteamControllerDevice);

const JSFunctionSpec SteamControllerDevice::js_fs[] = {
	JS_HELPERS_METHOD("setTouchPadEventMode", SteamControllerDevice::setTouchPadEventMode),
	JS_HELPERS_METHOD("setSetting", SteamControllerDevice::setSetting),
	JS_HELPERS_METHOD("enableKeys", SteamControllerDevice::enableKeys),
	JS_HELPERS_METHOD("disableKeys", SteamControllerDevice::disableKeys),
	JS_HELPERS_METHOD("enableMouse", SteamControllerDevice::enableMouse),
	JS_HELPERS_METHOD("hapticFeedback", SteamControllerDevice::hapticFeedback),
	JS_HELPERS_METHOD("setSounds", SteamControllerDevice::setSounds),
	JS_HELPERS_METHOD("playSound", SteamControllerDevice::playSound),
	JS_HELPERS_METHOD("reset", SteamControllerDevice::reset),
	JS_HELPERS_METHOD("shutdown", SteamControllerDevice::shutdown),
	JS_HELPERS_METHOD("calibrateTouchPads", SteamControllerDevice::calibrateTouchPads),
	JS_HELPERS_METHOD("calibrateSensors", SteamControllerDevice::calibrateSensors),
	JS_HELPERS_METHOD("calibrateJoystick", SteamControllerDevice::calibrateJoystick),
	JS_FS_END
};

#define DEFINE_CONSTANT(name) { #name, name }
const std::pair<std::string, int> SteamControllerDevice::js_int_const[] = {
	// enum HapticActuator
	DEFINE_CONSTANT (HapticLeft),
	DEFINE_CONSTANT (HapticRight),
	// enum Setting
	DEFINE_CONSTANT (SettingTrackBall),
	DEFINE_CONSTANT (SettingTrackBallInertia),
	DEFINE_CONSTANT (SettingLogoBrightness),
	DEFINE_CONSTANT (SettingOrientationSensors),
	DEFINE_CONSTANT (SettingSleepDelay),
	// enum TrackBallMode
	DEFINE_CONSTANT (TrackBallOn),
	DEFINE_CONSTANT (TrackBallOff),
	// enum OrientationSensor
	DEFINE_CONSTANT (OrientationTiltX),
	DEFINE_CONSTANT (OrientationTiltY),
	DEFINE_CONSTANT (OrientationAccel),
	DEFINE_CONSTANT (OrientationQuaternion),
	DEFINE_CONSTANT (OrientationGyro),
	// enum EventType
	DEFINE_CONSTANT (EventBtn),
	DEFINE_CONSTANT (EventAbs),
	DEFINE_CONSTANT (EventSensor),
	DEFINE_CONSTANT (EventTouchPad),
	DEFINE_CONSTANT (EventOrientation),
	// enum Button
	DEFINE_CONSTANT (BtnTriggerRight),
	DEFINE_CONSTANT (BtnTriggerLeft),
	DEFINE_CONSTANT (BtnShoulderRight),
	DEFINE_CONSTANT (BtnShoulderLeft),
	DEFINE_CONSTANT (BtnY),
	DEFINE_CONSTANT (BtnB),
	DEFINE_CONSTANT (BtnX),
	DEFINE_CONSTANT (BtnA),
	DEFINE_CONSTANT (BtnSelect),
	DEFINE_CONSTANT (BtnMode),
	DEFINE_CONSTANT (BtnStart),
	DEFINE_CONSTANT (BtnGripLeft),
	DEFINE_CONSTANT (BtnGripRight),
	DEFINE_CONSTANT (BtnClickLeft),
	DEFINE_CONSTANT (BtnClickRight),
	DEFINE_CONSTANT (BtnTouchLeft),
	DEFINE_CONSTANT (BtnTouchRight),
	// enum AbsAxis
	DEFINE_CONSTANT (AbsLeftX),
	DEFINE_CONSTANT (AbsLeftY),
	DEFINE_CONSTANT (AbsRightX),
	DEFINE_CONSTANT (AbsRightY),
	DEFINE_CONSTANT (AbsLeftTrigger),
	DEFINE_CONSTANT (AbsRightTrigger),
	// enum Sensor
	DEFINE_CONSTANT (SensorAccel),
	DEFINE_CONSTANT (SensorGyro),
	// enum TouchPad
	DEFINE_CONSTANT (TouchPadLeft),
	DEFINE_CONSTANT (TouchPadRight),
	{ "", 0 }
};

JSObject *SteamControllerDevice::makeJsObject (JSContext *cx, JS::HandleObject obj)
{
	InputDevice::JsClass input_class (cx, obj, JS::NullPtr ());
	SteamControllerDevice::JsClass sc_class (cx, obj, input_class.prototype ());
	return sc_class.newObjectFromPointer (this);
}

