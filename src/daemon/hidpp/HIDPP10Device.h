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

#ifndef HIDPP10_DEVICE_H
#define HIDPP10_DEVICE_H

#include "../InputDevice.h"
#include "../event/EventDevice.h"

#include <hidpp10/Device.h>
#include <hidpp/Dispatcher.h>
#include <hidpp/Setting.h>

namespace HIDPP10
{
struct MouseInfo;
}

/**
 * Manages devices using Logitech's HID++ 1.0 protocol.
 *
 * No event are sent currently. But associated event devices can be obtained
 * with getEventDevices.
 *
 * \ingroup InputDevices
 */
class HIDPP10Device: public InputDevice
{
public:
	HIDPP10Device (HIDPP::Device &&device, const std::vector<std::string> &paths);
	virtual ~HIDPP10Device ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;
	int32_t getSimpleEvent (uint16_t type, uint16_t code) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	/**
	 * Get the associated event devices.
	 *
	 * Events may be split accross several interfaces.
	 *
	 * For example, the G500 has 2 devices:
	 *  - Mouse movement, buttons and wheels.
	 *  - Keyboard and consumer controls.
	 */
	std::vector<EventDevice *> getEventDevices ();

	/**
	 * Low-level write access to a register.
	 */
	std::vector<uint8_t> setRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size);
	/**
	 * Low-level read access to a register.
	 */
	std::vector<uint8_t> getRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size);

	/**
	 * Get individual feature flags.
	 *
	 * See libhidpp documentation for flags list.
	 *
	 * \see setIndividualFeatureFlags
	 */
	unsigned int getIndividualFeatureFlags ();
	/**
	 * Set individual feature flags.
	 *
	 * See libhidpp documentation for flags list.
	 *
	 * \see getIndividualFeatureFlags
	 */
	void setIndividualFeatureFlags (unsigned int flags);

	/**
	 * Get the current mouse DPI.
	 *
	 * There may be 1 or 2 returned values depending on the mouse
	 * supporting separate axis DPI.
	 *
	 * libhidpp must know the mouse format for converting internal values to DPI.
	 */
	std::vector<unsigned int> getCurrentResolution ();
	/**
	 * Set the current mouse DPI.
	 *
	 * If the mouse use separate axes and only one value is provided.
	 * Vertical resolution is set to the same value as horizontal
	 * resolution.
	 *
	 * libhidpp must know the mouse format for converting DPI to internal values.
	 */
	void setCurrentResolution (const std::vector<unsigned int> &dpi);

	enum class Modifier {
		LeftControl = 0,
		LeftShift,
		LeftAlt,
		LeftMeta,
		RightControl,
		RightShift,
		RightAlt,
		RightMeta,
	};

	enum class SpecialAction: uint16_t {
		WheelLeft = 0x0001,
		WheelRight = 0x0002,
		BatteryLevel = 0x0003,
		ResolutionNext = 0x0004,
		ResolutionCycleNext = 0x0005,
		ResolutionPrev = 0x0008,
		ResolutionCyclePrev = 0x0009,
		ProfileNext = 0x0010,
		ProfileCycleNext = 0x0011,
		ProfilePrev = 0x0020,
		ProfileCyclePrev = 0x0021,
		ProfileSwitch0 = 0x0040,
		ProfileSwitch1 = 0x0140,
		ProfileSwitch2 = 0x0240,
		ProfileSwitch3 = 0x0340,
		ProfileSwitch4 = 0x0440,
	};

	using setting_t = std::variant<bool, int, std::string, HIDPP::LEDVector>;
	using button_t = std::variant<int, std::vector<int>>;
	/**
	 * Upload a temporary profile in RAM and use it.
	 *
	 * \param settings	Map general profile setting names to values.
	 * \param buttons	List of buttons. A button type depends on the properties:
	 *			 - "mouse-buttons": a mouse button index (from 0) or an array of buttons.
	 *			 - "key": a key HID usage code (can be used with "modifiers").
	 *			 - "modifiers": a modifier index or array (can be used with "key"). (see Modifier enum)
	 *			 - "special": a special action (see SpecialAction).
	 *			 - "consumer-control": a consumer control HID usage code.
	 *			If there is no property, the button is disabled.
	 * \param modes		An array of setting maps for each mode.
	 *
	 * See libhidpp for available settings.
	 */
	void loadTemporaryProfile (const std::map<std::string, setting_t> &settings,
				   const std::vector<std::map<std::string, button_t>> &buttons,
				   const std::vector<std::map<std::string, setting_t>> &modes);
	/**
	 * Load the persistent profile with given index.
	 */
	void loadProfileFromIndex (unsigned int index);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef jstpl::AbstractClass<HIDPP10Device> JsClass;

	virtual JSObject *makeJsObject (const jstpl::Thread *thread);

private:
	bool eventHandler (const HIDPP::Report &report);

	HIDPP10::Device _device;
	std::vector<EventDevice> _evdev;
	const HIDPP10::MouseInfo *_mouse_info;

	std::vector<HIDPP::Dispatcher::listener_iterator> _listener_iterators;
	MTQueue<HIDPP::Report> _report_queue;

	static bool _registered;
};

#endif
