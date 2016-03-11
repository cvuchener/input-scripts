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

#ifndef STEAM_CONTROLLER_DEVICE_H
#define STEAM_CONTROLLER_DEVICE_H

#include "../InputDevice.h"

#include <array>
#include <cstdint>
#include "../MTQueue.h"

class SteamControllerReceiver;

class SteamControllerDevice: public InputDevice
{
public:
	SteamControllerDevice (SteamControllerReceiver *receiver);
	virtual ~SteamControllerDevice ();

	virtual void interrupt ();
	virtual void readEvents ();

	virtual int32_t getValue (uint16_t type, uint16_t code);

	virtual std::string driver () const;
	virtual std::string name () const;
	virtual std::string serial () const;

	void setSetting (uint8_t setting, uint16_t value);
	void enableKeys ();
	void disableKeys ();
	std::string querySerial ();
	void setSounds (const std::vector<int8_t> &sounds);
	void playSound (int id);
	void shutdown ();

	enum HapticActuator: uint8_t {
		HapticLeft = 0x01,
		HapticRight = 0x00
	};

	void hapticFeedback (uint8_t actuator, uint16_t amplitude, uint16_t period, uint16_t count);

	virtual operator bool () const;

	enum Setting: uint8_t {
		SettingTrackBall = 0x08,
		SettingTrackBallInertia = 0x18,
		SettingLogoBrightness = 0x2d,
		SettingOrientationSensors = 0x30,
		SettingSleepDelay = 0x32,
	};

	enum TrackBallMode: uint16_t {
		TrackBallOn = 0x0000,
		TrackBallOff = 0x0007,
	};

	enum OrientationSensor: uint16_t {
		SensorTiltX = 0x0001,
		SensorTiltY = 0x0002,
		SensorAccel = 0x0004,
		SensorGyroQ = 0x0008,
		SensorGyroR = 0x0010,
	};

	enum Button {
		BtnTriggerRight = 8,
		BtnTriggerLeft = 9,
		BtnShoulderRight = 10,
		BtnShoulderLeft = 11,
		BtnY = 12,
		BtnB = 13,
		BtnX = 14,
		BtnA = 15,
		BtnSelect = 20,
		BtnMode = 21,
		BtnStart = 22,
		BtnGripLeft = 23,
		BtnGripRight = 24,
		BtnClickLeft = 25,
		BtnClickRight = 26,
		BtnTouchLeft = 27,
		BtnTouchRight = 28
	};

	enum AbsAxis {
		AbsLeftX,
		AbsLeftY,
		AbsRightX,
		AbsRightY,
		AbsLeftTrigger,
		AbsRightTrigger,
		AbsAccelX,
		AbsAccelY,
		AbsAccelZ,
		AbsGyroQW,
		AbsGyroQX,
		AbsGyroQY,
		AbsGyroQZ,
	};

	enum RelAxis {
		RelGyroRX,
		RelGyroRY,
		RelGyroRZ,
	};

	static constexpr int AxisFuzz = 16;
	static constexpr int TriggerFuzz = 4;
	static constexpr int AccelFuzz = 16;
	static constexpr int GyroFuzz = 16;

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::AbstractClass<SteamControllerDevice> JsClass;

	virtual JSObject *makeJsObject (JSContext *cx, JS::HandleObject obj);

private:
	SteamControllerReceiver *_receiver;
	bool _stop;
	MTQueue<std::array<uint8_t, 64>> _report_queue;
	struct {
		uint32_t buttons;
		int16_t left[2];
		int16_t right[2];
		int8_t triggers[2];
		int16_t accel[3];
		int16_t gyro_q[4];
	} _state;
	std::string _serial;
};

#endif
