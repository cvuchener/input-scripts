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

#ifndef EVENT_DEVICE_H
#define EVENT_DEVICE_H

#include "../InputDevice.h"

#include <libevdev/libevdev.h>

/**
 * This class manages a event device from linux ABI.
 *
 * Events from this device are standard linux input events with
 * "type", "code" and "value" entries.
 */
class EventDevice: public InputDevice
{
public:
	/**
	 * Create a device from the device node path.
	 *
	 * \param path A path to an event device node (usually /dev/input/eventX).
	 */
	EventDevice (const std::string &path);
	EventDevice (const EventDevice &) = delete;
	EventDevice (EventDevice &&);
	virtual ~EventDevice ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	/**
	 * Grab or ungrab the device.
	 *
	 * When grabbed other applications will not receive any event from
	 * this device.
	 *
	 * \param grab_mode true for grabbing, false for ungrabbing.
	 */
	void grab (bool grab_mode);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	typedef JsHelpers::AbstractClass<EventDevice> JsClass;

	JSObject *makeJsObject (const JsHelpers::Thread *thread) override;

private:
	void readEvents ();

	int _fd, _pipe[2];
	std::thread _thread;
	struct libevdev *_dev;

	static bool _registered;
};

#endif
