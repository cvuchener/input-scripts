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

#ifndef DRIVER_H
#define DRIVER_H

#include <functional>

struct udev_device;
class InputDevice;

class Driver
{
public:
	Driver ();
	virtual ~Driver ();

	virtual void addDevice (udev_device *) = 0;
	virtual void removeDevice (udev_device *) = 0;

	std::function<void (InputDevice *)> inputDeviceAdded;
	std::function<void (InputDevice *)> inputDeviceRemoved;
};

#endif
