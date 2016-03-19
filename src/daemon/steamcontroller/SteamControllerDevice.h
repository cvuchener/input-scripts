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

extern "C" {
#include <linux/input.h>
}

class SteamControllerReceiver;

class SteamControllerDevice: public InputDevice
{
public:
	SteamControllerDevice (SteamControllerReceiver *receiver);
	virtual ~SteamControllerDevice ();

	virtual void interrupt ();
	virtual void readEvents ();

	virtual InputDevice::Event getEvent (InputDevice::Event event);

	virtual std::string driver () const;
	virtual std::string name () const;
	virtual std::string serial () const;

	void setTouchPadEventMode (bool report_single_axis, bool report_linked_axes);

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
		OrientationTiltX = 0x0001,
		OrientationTiltY = 0x0002,
		OrientationAccel = 0x0004,
		OrientationQuaternion = 0x0008,
		OrientationGyro = 0x0010,
	};

	enum EventType {
		EventBtn = EV_KEY,
		EventAbs = EV_ABS,
		EventSensor = EV_MAX+1,
		EventTouchPad,
		EventOrientation
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
	};

	enum Sensor {
		SensorAccel,
		SensorGyro,
	};

	enum TouchPad {
		TouchPadLeft,
		TouchPadRight,
	};

	static constexpr int AxisFuzz = 16;
	static constexpr int TriggerFuzz = 4;
	static constexpr int SensorFuzz = 16;

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::AbstractClass<SteamControllerDevice> JsClass;

	virtual JSObject *makeJsObject (JSContext *cx, JS::HandleObject obj);

private:
	SteamControllerReceiver *_receiver;
	bool _stop;
	MTQueue<std::array<uint8_t, 64>> _report_queue;
	bool _report_single_axis, _report_linked_axes;
	struct {
		uint32_t buttons;
		int16_t touchpad[2][2];
		int8_t triggers[2];
		int16_t accel[3];
		int16_t gyro[3];
		int16_t quaternion[4];
	} _state;
	std::string _serial;
};

#endif
