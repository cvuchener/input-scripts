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

#ifndef OBJECT_MANAGER_H
#define OBJECT_MANAGER_H

#include "dbus/ObjectManagerInterfaceProxy.h"

class ObjectManager:
	public org::freedesktop::DBus::ObjectManager_proxy,
	public DBus::IntrospectableProxy,
	public DBus::ObjectProxy
{
public:
	ObjectManager (DBus::Connection &connection, const char *path, const char *name);

	virtual void InterfacesAdded (const DBus::Path &object_path, const std::map<std::string, std::map<std::string, DBus::Variant>> &interfaces_and_properties);
	virtual void InterfacesRemoved (const DBus::Path &object_path, const std::vector<std::string> &interfaces);
};

#endif
