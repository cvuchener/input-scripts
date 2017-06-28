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

#include "EventDriver.h"

#include "EventDevice.h"

extern "C" {
#include <libudev.h>
}

EventDriver::EventDriver ()
{
}

EventDriver::~EventDriver ()
{
	for (auto pair: _devices) {
		delete pair.second;
	}
}

void EventDriver::addDevice (udev_device *dev)
{
	EventDevice *evdev = new EventDevice (udev_device_get_devnode (dev));
	_devices.emplace (udev_device_get_syspath (dev), evdev);
	inputDeviceAdded (evdev);
}

void EventDriver::removeDevice (udev_device *dev)
{
	auto it = _devices.find (udev_device_get_syspath (dev));
	if (it == _devices.end ()) {
		return;
	}
	inputDeviceRemoved (it->second);
	delete it->second;
	_devices.erase (it);
}

bool EventDriver::_registered = Driver::registerDriver ("event", new EventDriver ());
