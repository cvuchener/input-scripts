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

#include "WiimoteDriver.h"

#include "WiimoteDevice.h"

extern "C" {
#include <libudev.h>
}

WiimoteDriver::WiimoteDriver ()
{
}

WiimoteDriver::~WiimoteDriver ()
{
	for (auto pair: _devices) {
		delete pair.second;
	}
}

void WiimoteDriver::addDevice (udev_device *dev)
{
	const char *syspath = udev_device_get_syspath (dev);
	WiimoteDevice *wiidev;
	try {
		wiidev = new WiimoteDevice (syspath);
	}
	catch (WiimoteDevice::UnknownDeviceError) {
		return;
	}
	_devices.emplace (syspath, wiidev);
	inputDeviceAdded (wiidev);
}

void WiimoteDriver::changeDevice (udev_device *dev)
{
	const char *syspath = udev_device_get_syspath (dev);
	if (_devices.find (syspath) != _devices.end ())
		return;
	WiimoteDevice *wiidev;
	try {
		wiidev = new WiimoteDevice (syspath);
	}
	catch (WiimoteDevice::UnknownDeviceError) {
		Log::warning () << "Wiimote devtype is unknown after change event." << std::endl;
		return;
	}
	_devices.emplace (syspath, wiidev);
	inputDeviceAdded (wiidev);
}

void WiimoteDriver::removeDevice (udev_device *dev)
{
	auto it = _devices.find (udev_device_get_syspath (dev));
	if (it == _devices.end ()) {
		return;
	}
	inputDeviceRemoved (it->second);
	delete it->second;
	_devices.erase (it);
}

bool WiimoteDriver::_registered = Driver::registerDriver ("wiimote", new WiimoteDriver ());
