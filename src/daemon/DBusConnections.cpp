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

#include "DBusConnections.h"

extern "C" {
#include <unistd.h>
}

DBus::Connection *DBusConnections::_session = nullptr;
DBus::Connection *DBusConnections::_system = nullptr;

DBus::Connection &DBusConnections::sessionBus ()
{
	if (!_session)
		_session = new DBus::Connection (DBus::Connection::SessionBus ());
	return *_session;
}

DBus::Connection &DBusConnections::systemBus ()
{
	if (!_system)
		_system = new DBus::Connection (DBus::Connection::SystemBus ());
	return *_system;
}

DBus::Connection &DBusConnections::getBus (Bus bus)
{
	switch (bus) {
	case Auto:
		if (getuid () == 0)
			return systemBus ();
		else
			return sessionBus ();

	case SessionBus:
		return sessionBus ();

	case SystemBus:
	default:
		return systemBus ();
	}
}
