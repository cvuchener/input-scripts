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

/**
 * This class manages a steam controller.
 *
 * This device may send different types of events:
 *  - Standard events (EV_KEY, EV_ABS, EV_SYN) but with special codes
 *    (from Button for EV_KEY/EventBtn, from AbsAxis from EV_ABS/EventAbs).
 *  - Special events for sensors, orientation or "touch point" (linked
 *    axes value per touchpad/stick).
 *
 * Left touchpad and stick use the same axis values (AbsLeftX, AbsLeftY and
 * TouchPadLeft). They can be discrimated by looking at the value of
 * BtnTouchLeft.
 *
 * \ingroup InputDevices
 */
class SteamControllerDevice: public InputDevice
{
public:
	SteamControllerDevice (SteamControllerReceiver *receiver);
	virtual ~SteamControllerDevice ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;
	int32_t getSimpleEvent (uint16_t type, uint16_t code) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	/**
	 * Set what kind of events will be sent for the touchpads and the stick.
	 *
	 * \param report_single_axis If true, EventAbs (EV_ABS) events will be
	 *                           sent for each axis.
	 * \param report_linked_axes If true, EventTouchPad events will be sent
	 *                           with both x and y values.
	 */
	void setTouchPadEventMode (bool report_single_axis, bool report_linked_axes);

	/**
	 * Change a hardware setting on the controller.
	 *
	 * \param setting The setting to change.
	 * \param value New value for the setting.
	 *
	 * \sa Setting
	 */
	void setSetting (uint8_t setting, uint16_t value);
	/**
	 * Enable the default hardware key bindings.
	 *
	 * Events for these are sent through the standard keyboard/mouse
	 * HID interface.
	 */
	void enableKeys ();
	/**
	 * Disable the default hardware key bindings.
	 */
	void disableKeys ();
	/**
	 * Enable the default hardware mouse (trackball emulation).
	 */
	void enableMouse ();
	/**
	 * Query the serial from the device.
	 */
	std::string querySerial ();
	/**
	 * Configure the hardware sounds for this controller.
	 *
	 * \param sounds Array of sound IDs. -1 for disabled.
	 *
	 * Some indices have special roles. Known roles are:
	 *  - 0: turn on
	 *  - 1: turn off
	 * others may be used by steam (identify, ...).
	 *
	 * Valid sound IDs are from 0 to 16.
	 *
	 * Sounds can then be played by calling playSound.
	 *
	 * \sa playSound
	 */
	void setSounds (const std::vector<int8_t> &sounds);
	/**
	 * Play a sound stored on the hardware.
	 *
	 * \param id The index of the sound in the array from setSounds.
	 *
	 * \sa setSounds
	 */
	void playSound (int id);
	/**
	 * Reset the controller.
	 *
	 * This will disconnect the device and, thus, destroy this device object.
	 * Then, the controller reconnects automatically.
	 */
	void reset ();
	/**
	 * Shutdown the controller.
	 *
	 * This will disconnect the device and, thus, destroy this device object.
	 */
	void shutdown ();
	/**
	 * Calibrate the touch pads.
	 */
	void calibrateTouchPads ();
	/**
	 * Calibrate the sensors.
	 */
	void calibrateSensors ();
	/**
	 * Calibrate the joystick.
	 */
	void calibrateJoystick ();

	enum HapticActuator: uint8_t {
		HapticLeft = 0x01,
		HapticRight = 0x00
	};

	/**
	 * Play a haptic feedback effect.
	 *
	 * \param actuator The actuator that will play the effect.
	 * \param amplitude The amplitude of the effect.
	 * \param period The period between two "clicks". The period may actually be longer depending on the amplitude.
	 * \param count The total number "clicks" before the effect ends.
	 */
	void hapticFeedback (uint8_t actuator, uint16_t amplitude, uint16_t period, uint16_t count);

	/**
	 * Settings that can be changed with setSetting.
	 *
	 * \sa setSetting
	 */
	enum Setting: uint8_t {
		/**
		 * Enable/disable the trackball emulation on the right touch pad.
		 *
		 * Value must be one from TrackBallMode.
		 * \sa TrackBallMode
		 */
		SettingTrackBall = 0x08,
		/** Change the trackball emulation inertia. */
		SettingTrackBallInertia = 0x18,
		/** Change the Steam logo brightness (maximum value is 100). */
		SettingLogoBrightness = 0x2d,
		/**
		 * Enable/disable orientation sensors.
		 *
		 * Value must be a combination of flags from OrientationSensors.
		 * \sa OrientationSensor
		 */
		SettingOrientationSensors = 0x30,
		/** Change the delay before a wireless controller goes to sleep (in seconds). */
		SettingSleepDelay = 0x32,
	};

	enum TrackBallMode: uint16_t {
		TrackBallOn = 0x0000,
		TrackBallOff = 0x0007,
	};

	enum OrientationSensor: uint16_t {
		/** If set, Left X axis will be replaced with tilt value. */
		OrientationTiltX = 0x0001,
		/** If set, Left Y axis will be replaced with tilt value. */
		OrientationTiltY = 0x0002,
		/** Enables accelerometer data. */
		OrientationAccel = 0x0004,
		/** Enables orientation quaternion data */
		OrientationQuaternion = 0x0008,
		/** Enables gyroscope data */
		OrientationGyro = 0x0010,
	};

	enum EventType {
		/**
		 * Same as EV_KEY but with special button code.
		 * \sa Button
		 */
		EventBtn = EV_KEY,
		/**
		 * Same as EV_ABS but with special button code.
		 * \sa Button
		 */
		EventAbs = EV_ABS,
		/**
		 * 3 axis sensor data.
		 *
		 * code is one from Sensor.
		 * Sensor data is given in "x", "y" and "z".
		 * \sa Sensor
		 */
		EventSensor = EV_MAX+1,
		/**
		 * 2 axis data from touchpads or stick
		 *
		 * code is one from TouchPad.
		 * Touch/stick position is given in "x" and "y".
		 * \sa TouchPad
		 */
		EventTouchPad,
		/**
		 * Orientation of the controller as a quaternion.
		 *
		 * There is no "code". THe quaternion is given in
		 * "w", "x", "y" and "z".
		 */
		EventOrientation
	};

	enum Button {
		BtnTriggerRight = 0,
		BtnTriggerLeft = 1,
		BtnShoulderRight = 2,
		BtnShoulderLeft = 3,
		BtnY = 4,
		BtnB = 5,
		BtnX = 6,
		BtnA = 7,
		BtnSelect = 12,
		BtnMode = 13,
		BtnStart = 14,
		BtnGripLeft = 15,
		BtnGripRight = 16,
		BtnClickLeft = 17,
		BtnClickRight = 18,
		BtnTouchLeft = 19,
		BtnTouchRight = 20,
		BtnCount = 24
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
	static const char *js_name;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef jstpl::AbstractClass<SteamControllerDevice> JsClass;

	JSObject *makeJsObject (const jstpl::Thread *thread) override;

private:
	void readEvent (const std::array<uint8_t, 64> &report);

	SteamControllerReceiver *_receiver;
	sigc::connection _input_report_conn;
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

	static bool _registered;
};

#endif
