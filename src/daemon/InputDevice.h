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

#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <cstdint>
#include <functional>

#include "JsHelpers/JsHelpers.h"

/**
 * This is the base class for every input device class used by drivers.
 */
class InputDevice
{
public:
	/**
	 * Event contains the data for an input event.
	 *
	 * It must have a "type" entry. Other entries may vary depending on
	 * the driver and type.
	 *
	 * A event whose type is a valid EV_* type from linux input events
	 * must have "code" and "value" entries.
	 */
	typedef std::map<std::string, int> Event;

	virtual ~InputDevice ();

	/**
	 * Start reading events
	 */
	virtual void start () = 0;
	/**
	 * Stop reading events
	 */
	virtual void stop () = 0;

	/**
	 * Signal sent when a fatal error happens.
	 *
	 * No more events are read, but stop must still be called.
	 */
	sigc::signal<void ()> error;

	/**
	 * Get the current value/state for the given event
	 *
	 * For example, for a standard linux input event, the "value" entry
	 * will be filled with current value for the given type and code.
	 */
	virtual Event getEvent (Event) = 0;
	/**
	 * Get the current value/state for the given event by type and code.
	 */
	virtual int32_t getSimpleEvent (uint16_t type, uint16_t code) = 0;
	/**
	 * Get the current state of the key \p code.
	 *
	 * \param code A valid code for an event with type EV_KEY.
	 *
	 * \returns if the key is pressed.
	 */
	bool keyPressed (uint16_t code);
	/**
	 * Get the current state of the absolute axis \p code.
	 *
	 * \param code A valid code for an event with type EV_ABS.
	 *
	 * \returns the current value of the axis.
	 */
	int32_t getAxisValue (uint16_t code);

	/**
	 * Get the name of the driver used by this device.
	 */
	virtual std::string driver () const = 0;
	/**
	 * Get the name of the device.
	 */
	virtual std::string name () const = 0;
	/**
	 * Get the serial of the device.
	 */
	virtual std::string serial () const = 0;

	/**
	 * Signals for input events.
	 */
	sigc::signal<void (Event)> event;
	sigc::signal<void (uint16_t, uint16_t, int32_t)> simpleEvent;

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const JsHelpers::SignalMap js_signals;
	typedef JsHelpers::AbstractClass<InputDevice> JsClass;

	/**
	 * Create a JS object from this device.
	 *
	 * \param cx JS context
	 * \param obj Global object where the class is created.
	 */
	virtual JSObject *makeJsObject (const JsHelpers::Thread *) = 0;

protected:
	void eventRead (const Event &);
	void simpleEventRead (uint16_t type, uint16_t code, int32_t value);

private:
	static bool _registered;
};

#endif
