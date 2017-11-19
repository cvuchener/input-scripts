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

#ifndef HIDPP20_DEVICE_H
#define HIDPP20_DEVICE_H

#include "../InputDevice.h"

#include <hidpp20/Device.h>
#include <hidpp/Dispatcher.h>

extern "C" {
#include <linux/input.h>
}

namespace HIDPP20
{
class IMouseButtonSpy;
class IOnboardProfiles;
class IReprogControlsV4;
}

/**
 * Manages devices using Logitech's HID++ 2.0 or later protocol.
 *
 * Events from every device supported feature are received and are sent as
 * \ref EventRawHIDPP events if activated with \ref sendRawEvents.
 *
 * Features supported by this class are also sent as parsed events. Currently
 * supported features are:
 *  - MouseButtonSpy (sent as EV_KEY, BTN_MOUSE+i events),
 *  - OnboardProfiles (EventOnboardProfilesCurrentProfile, and EventOnboardProfilesCurrentDPIIndex events),
 *  - ReprogControlsV4 (ReprogControlsV4Button, and ReprogControlsV4RawXY events).
 *
 * \ingroup InputDevices
 */
class HIDPP20Device: public InputDevice
{
public:
	HIDPP20Device (HIDPP::Device &&device);
	virtual ~HIDPP20Device ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;
	int32_t getSimpleEvent (uint16_t type, uint16_t code) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	/**
	 * Test if feature given by ID is available on this device.
	 */
	bool hasFeature (uint16_t feature_id) const;
	/**
	 * Low-level function call.
	 */
	std::vector<uint8_t> callFunction (uint16_t id, unsigned int function, const std::vector<uint8_t> &params);

	/**
	 * Enables sending raw events for every feature.
	 */
	void sendRawEvents (bool enable);

	enum EventType {
		/**
		 * Raw HID++ events.
		 *
		 *  - "feature": the feature ID.
		 *  - "function": the event index.
		 *  - "dataN": *N* th byte of raw data.
		 */
		EventRawHIDPP = EV_MAX+1,
		/**
		 * Current on-board profile was changed.
		 *
		 *  - "rom": memory type (0: from flash, 1: from ROM)
		 *  - "index": the profile index.
		 */
		EventOnboardProfilesCurrentProfile,
		/**
		 * Current DPI mode was changed.
		 *
		 *  - "index": the DPI mode index.
		 */
		EventOnboardProfilesCurrentDPIIndex,
		/**
		 * A diverted reprogrammable button was pressed or released.
		 *
		 * Similar to a EV_KEY event but "code" is a control ID.
		 */
		EventReprogControlsV4Button,
		/**
		 * A diverted axis was moved.
		 *
		 * Data in "x" and "y".
		 */
		EventReprogControlsV4RawXY,
	};

	/**
	 * \name MouseButtonSpy
	 * Mouse button spying interface.
	 *
	 * Remap and get button events for the mouse.
	 *
	 * Remapping requires OnboardProfiles to be in host mode. A mapping is
	 * a array of integers up to 16 buttons. Valid button values are 1
	 * to 16, 0 disable the button. The mapping changes the HID events but
	 * MouseButtonSpy events still reports every button with its normal
	 * index.
	 *
	 * For example <tt>{ 2, 1, 3, 4, 5 }</tt>, will invert the two main
	 * buttons and silents the buttons after the first five so they can
	 * only be received by this driver through HID++ events.
	 *
	 * MouseButtonSpy events are \c EV_KEY events with \c BTN_MOUSE+i codes.
	 */
	///\{
	/**
	 * Get the number of buttons supported by this interface.
	 */
	unsigned int MouseButtonSpy_GetButtonCount ();
	/**
	 * Start sending button events.
	 */
	void MouseButtonSpy_Start ();
	/**
	 * Stop sending button events.
	 */
	void MouseButtonSpy_Stop ();
	/**
	 * Get the current mouse button mapping.
	 */
	std::vector<uint8_t> MouseButtonSpy_GetMapping ();
	/**
	 * Remap the mouse buttons.
	 */
	void MouseButtonSpy_SetMapping (const std::vector<uint8_t> &button_mapping);
	///\}

	/**
	 * \name OnboardProfiles
	 * Manage on-board profiles.
	 *
	 * On-board profiles are used when the device is *not* in host mode.
	 *
	 * \see EventOnboardProfilesCurrentProfile, EventOnboardProfilesCurrentDPIIndex.
	 */
	///\{
	/**
	 * Get if the device is currently in host mode.
	 */
	bool OnboardProfiles_GetHostMode ();
	/**
	 * Enter or exit host mode.
	 */
	void OnboardProfiles_SetHostMode (bool host_mode);
	/**
	 * Get the current profile.
	 *
	 * \returns a object with two properties: "rom" is 1 if the profile is
	 * from ROM, "index" is the profile index.
	 */
	std::map<std::string, unsigned int> OnboardProfiles_GetCurrentProfile ();
	/**
	 * Set the current profile.
	 *
	 * \param rom use a profile from ROM.
	 * \param index the new profile index.
	 */
	void OnboardProfiles_SetCurrentProfile (bool rom, unsigned int index);
	/**
	 * Get the current DPI mode index.
	 */
	unsigned int OnboardProfiles_GetCurrentDPIIndex ();
	/**
	 * Set the current DPI mode index.
	 */
	void OnboardProfiles_SetCurrentDPIIndex (unsigned int index);
	///\}

	/**
	 * \name ReprogControlsV4
	 * Manages reprogrammable controls.
	 *
	 * \see EventReprogControlsV4Button, EventReprogControlsV4RawXY.
	 */
	///\{
	/**
	 * Get the number of reprogrammable controls on this device.
	 */
	unsigned int ReprogControlsV4_GetControlCount ();
	/**
	 * Get information about a reprogrammable control.
	 *
	 * \param index Control index from 0 to ReprogControlsV4_GetControlCount (excluded).
	 *
	 * \returns properties describing the control:
	 *  - "control_id": The control ID, used in to refer to this control in other methods.
	 *  - "task_id": Recommended usage for this control (what is printed on the key).
	 *  - "mouse_button": the control is a mouse button.
	 *  - "fkey": the control is F-key.
	 *  - "hotkey": the control is a hot-key.
	 *  - "fntoggle": the control is toggled when pressing the Fn key.
	 *  - "reprog": the control can be remapped to another control ID.
	 *  - "divert": the control can be diverted to a \ref EventReprogControlsV4Button
	 *              event (instead of normal or remapped event).
	 *  - "persist": the control can be persistently diverted.
	 *  - "vitual": the control does not physically exists (used only as a remapping target).
	 *  - "rawxy": the control has axes than can be diverted to
	 *             \ref EventReprogControlsV4RawXY events.
	 *  - "pos": for F key, the position of the key.
	 *  - "group": the control group (from 1 to 8), 0 if not in a group
	 *  - "group_mask": bit mask giving the list of group the control can be remapped to.
	 */
	std::map<std::string, int> ReprogControlsV4_GetControlInfo (unsigned int index);
	/**
	 * Get the current remapping for the given control.
	 *
	 * \returns remapped control ID or 0 if not remapped.
	 */
	uint16_t ReprogControlsV4_GetRemap (uint16_t control_id);
	/**
	 * Remap a control.
	 *
	 * Remap a control to itself to disabled remapping.
	 */
	void ReprogControlsV4_SetRemap (uint16_t control_id, uint16_t remap);
	/**
	 * Check if the control is diverted.
	 */
	bool ReprogControlsV4_GetDivert (uint16_t control_id);
	/**
	 * Divert the control.
	 */
	void ReprogControlsV4_SetDivert (uint16_t control_id, bool divert);
	/**
	 * Check if the control is persistently diverted.
	 */
	bool ReprogControlsV4_GetPersistentDivert (uint16_t control_id);
	/**
	 * Persistently divert the control.
	 */
	void ReprogControlsV4_SetPersistentDivert (uint16_t control_id, bool divert);
	/**
	 * Check if raw axes are diverted.
	 */
	bool ReprogControlsV4_GetRawXYDivert (uint16_t control_id);
	/**
	 * Divert raw axis data.
	 */
	void ReprogControlsV4_SetRawXYDivert (uint16_t control_id, bool divert);
	///\}

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef jstpl::AbstractClass<HIDPP20Device> JsClass;

	JSObject *makeJsObject (const jstpl::Thread *thread) override;

private:
	bool eventHandler (const HIDPP::Report &report);

	HIDPP20::Device _device;
	std::map<uint16_t, uint8_t> _features_id_index;
	std::map<uint8_t, uint16_t> _features_index_id;

	bool _send_raw_events;

	struct MouseButtonSpy;
	std::unique_ptr<MouseButtonSpy> _mbs;

	struct OnboardProfiles;
	std::unique_ptr<OnboardProfiles> _op;

	struct ReprogControlsV4;
	std::unique_ptr<ReprogControlsV4> _rc4;

	std::vector<HIDPP::Dispatcher::listener_iterator> _listener_iterators;

	static bool _registered;
};

#endif
