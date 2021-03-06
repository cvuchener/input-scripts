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

#include "Driver.h"

Driver::Driver ()
{
}

Driver::~Driver ()
{
}

void Driver::changeDevice (udev_device *)
{
}

std::map<std::string, std::unique_ptr<Driver>> Driver::_drivers;

Driver *Driver::findDriver (std::string name)
{
	auto it = _drivers.find (name);
	if (it == _drivers.end ())
		return nullptr;
	return it->second.get ();
}

Driver::const_iterator Driver::begin ()
{
	return _drivers.begin ();
}

Driver::const_iterator Driver::end ()
{
	return _drivers.end ();
}

bool Driver::registerDriver (std::string name, Driver *driver)
{
	return _drivers.emplace (name, std::unique_ptr<Driver> (driver)).second;
}
