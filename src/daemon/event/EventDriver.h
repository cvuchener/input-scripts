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

#ifndef EVENT_DRIVER_H
#define EVENT_DRIVER_H

#include "../Driver.h"

#include <map>

class EventDevice;

class EventDriver: public Driver
{
public:
	EventDriver ();
	virtual ~EventDriver ();

	virtual void addDevice (udev_device *);
	virtual void removeDevice (udev_device *);

private:
	std::map<std::string, EventDevice *> _devices;

	static bool _registered;
};

#endif
