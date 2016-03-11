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

#include "../Log.h"

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
}

SteamControllerDevice::SteamControllerDevice (SteamControllerReceiver *receiver):
	_receiver (receiver)
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

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, T>::type
readLE (uint8_t *buffer) {
	T value = 0;
	for (unsigned int i = 0; i < sizeof (T); ++i) {
		value |= buffer[i] << (8*i);
	}
	return value;
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value>::type
writeLE (uint8_t *buffer, T value) {
	for (unsigned int i = 0; i < sizeof (T); ++i) {
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
		uint32_t buttons = readLE<uint32_t> (&report[7]);
		uint8_t triggers[2] = { report[11], report[12] };
		int16_t left[2] = {
			readLE<int16_t> (&report[16]),
			readLE<int16_t> (&report[18])
		};
		int16_t right[2] = {
			readLE<int16_t> (&report[20]),
			readLE<int16_t> (&report[22])
		};
		//int16_t triggers_16[2] = {
		//	readLE<int16_t> (&report[24]),
		//	readLE<int16_t> (&report[26])
		//};
		int16_t accel[3] = {
			readLE<int16_t> (&report[28]),
			readLE<int16_t> (&report[30]),
			readLE<int16_t> (&report[32])
		};
		int16_t gyro_r[3] = {
			readLE<int16_t> (&report[34]),
			readLE<int16_t> (&report[36]),
			readLE<int16_t> (&report[38])
		};
		int16_t gyro_q[4] = {
			readLE<int16_t> (&report[40]),
			readLE<int16_t> (&report[42]),
			readLE<int16_t> (&report[44]),
			readLE<int16_t> (&report[46])
		};

		uint32_t buttons_diff = buttons ^ _state.buttons;
		_state.buttons = buttons;
		bool changed = false;
		for (unsigned int i = 0; i < 2; ++i) {
			if (std::abs (_state.left[i]-left[i]) > AxisFuzz) {
				_state.left[i] = left[i];
				eventRead (EV_ABS, AbsLeftX+i, left[i]);
				changed = true;
			}
			if (std::abs (_state.right[i]-right[i]) > AxisFuzz) {
				_state.right[i] = right[i];
				eventRead (EV_ABS, AbsRightX+i, right[i]);
				changed = true;
			}
			if (std::abs (_state.triggers[i]-triggers[i]) > TriggerFuzz) {
				_state.triggers[i] = triggers[i];
				eventRead (EV_ABS, AbsLeftTrigger+i, triggers[i]);
				changed = true;
			}
		}
		for (unsigned int i = 0; i < 3; ++i) {
			if (std::abs (_state.accel[i]-accel[i]) > AccelFuzz) {
				_state.accel[i] = accel[i];
				eventRead (EV_ABS, AbsAccelX+i, accel[i]);
				changed = true;
			}
			if (gyro_r[i] != 0) {
				eventRead (EV_REL, RelGyroRX+i, gyro_r[i]);
				changed = true;
			}
		}
		for (unsigned int i = 0; i < 4; ++i) {
			if (std::abs (_state.gyro_q[i]-gyro_q[i]) > GyroFuzz) {
				_state.gyro_q[i] = gyro_q[i];
				eventRead (EV_ABS, AbsGyroQW+i, gyro_q[i]);
				changed = true;
			}
		}
		for (unsigned int i = 0; i < 32; ++i) {
			if (buttons_diff & (1<<i)) {
				eventRead (EV_KEY, i, (buttons & (1<<i) ? 1 : 0));
				changed = true;
			}
		}
		if (changed)
			eventRead (EV_SYN, SYN_REPORT, 0);
	}
}

int32_t SteamControllerDevice::getValue (uint16_t type, uint16_t code)
{
	switch (type) {
	case EV_KEY:
		if (code >= 32)
			throw std::invalid_argument ("button code too high");
		return (_state.buttons & (1<<code)) >> code;

	case EV_ABS:
		switch (code) {
		case AbsLeftX:
			return _state.left[0];
		case AbsLeftY:
			return _state.left[1];
		case AbsRightX:
			return _state.right[0];
		case AbsRightY:
			return _state.right[1];
		case AbsLeftTrigger:
			return _state.triggers[0];
		case AbsRightTrigger:
			return _state.triggers[1];
		case AbsAccelX:
			return _state.accel[0];
		case AbsAccelY:
			return _state.accel[1];
		case AbsAccelZ:
			return _state.accel[2];
		case AbsGyroQW:
			return _state.gyro_q[0];
		case AbsGyroQX:
			return _state.gyro_q[1];
		case AbsGyroQY:
			return _state.gyro_q[2];
		case AbsGyroQZ:
			return _state.gyro_q[3];
		default:
			throw std::invalid_argument ("invalid abs axis code");
		}

	default:
		throw std::invalid_argument ("event type not supported");
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

void SteamControllerDevice::setSetting (uint8_t setting, uint16_t value)
{
	std::vector<uint8_t> params (3);
	params[0] = setting;
	writeLE (&params[1], value);
	_receiver->sendRequest (0x87, params);
}

void SteamControllerDevice::enableKeys ()
{
	std::vector<uint8_t> params (0);
	_receiver->sendRequest (0x85, params);
}

void SteamControllerDevice::disableKeys ()
{
	std::vector<uint8_t> params (0);
	_receiver->sendRequest (0x81, params);
}

void SteamControllerDevice::hapticFeedback (uint8_t actuator, uint16_t amplitude, uint16_t period, uint16_t count)
{
	std::vector<uint8_t> params (7);
	params[0] = actuator;
	writeLE (&params[1], amplitude);
	writeLE (&params[3], period);
	writeLE (&params[5], count);
	_receiver->sendRequest (0x8f, params);
}

std::string SteamControllerDevice::querySerial ()
{
	std::vector<uint8_t> params ({1}), results;
	//params[0] = 1;
	do {
		try {
			_receiver->sendRequest (0xae, params, &results);
		}
		catch (std::runtime_error e) {
			Log::warning () << "In " << __PRETTY_FUNCTION__ << ": "
					<< e.what () << std::endl;
		}
	} while (results.size () < 1 || results[0] != 1);
	results.push_back (0); // Make sure the string is null-terminated
	return std::string (reinterpret_cast<char *> (&results[1]));
}

void SteamControllerDevice::setSounds (const std::vector<int8_t> &sounds)
{
	std::vector<uint8_t> params (sounds.begin (), sounds.end ());
	params.resize (16, -1);
	_receiver->sendRequest (0xc1, params);
}

void SteamControllerDevice::playSound (int id)
{
	std::vector<uint8_t> params (4);
	writeLE<int32_t> (&params[0], id);
	_receiver->sendRequest (0xb6, params);
}

void SteamControllerDevice::shutdown ()
{
	_receiver->sendRequest (0x9f, { 'o', 'f', 'f', '!' });
}

SteamControllerDevice::operator bool () const
{
	return _receiver->isConnected ();
}

const JSClass SteamControllerDevice::js_class = JS_HELPERS_CLASS("SteamControllerDevice", SteamControllerDevice);

const JSFunctionSpec SteamControllerDevice::js_fs[] = {
	JS_HELPERS_METHOD("setSetting", SteamControllerDevice::setSetting),
	JS_HELPERS_METHOD("enableKeys", SteamControllerDevice::enableKeys),
	JS_HELPERS_METHOD("disableKeys", SteamControllerDevice::disableKeys),
	JS_HELPERS_METHOD("hapticFeedback", SteamControllerDevice::hapticFeedback),
	JS_HELPERS_METHOD("setSounds", SteamControllerDevice::setSounds),
	JS_HELPERS_METHOD("playSound", SteamControllerDevice::playSound),
	JS_HELPERS_METHOD("shutdown", SteamControllerDevice::shutdown),
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
	DEFINE_CONSTANT (SensorTiltX),
	DEFINE_CONSTANT (SensorTiltY),
	DEFINE_CONSTANT (SensorAccel),
	DEFINE_CONSTANT (SensorGyroQ),
	DEFINE_CONSTANT (SensorGyroR),
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
	DEFINE_CONSTANT (AbsAccelX),
	DEFINE_CONSTANT (AbsAccelY),
	DEFINE_CONSTANT (AbsAccelZ),
	DEFINE_CONSTANT (AbsGyroQW),
	DEFINE_CONSTANT (AbsGyroQX),
	DEFINE_CONSTANT (AbsGyroQY),
	DEFINE_CONSTANT (AbsGyroQZ),
	// enum RelAxis
	DEFINE_CONSTANT (RelGyroRX),
	DEFINE_CONSTANT (RelGyroRY),
	DEFINE_CONSTANT (RelGyroRZ),
	{ "", 0 }
};

JSObject *SteamControllerDevice::makeJsObject (JSContext *cx, JS::HandleObject obj)
{
	InputDevice::JsClass input_class (cx, obj, JS::NullPtr ());
	SteamControllerDevice::JsClass sc_class (cx, obj, input_class.prototype ());
	return sc_class.newObjectFromPointer (this);
}

