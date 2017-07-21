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

#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <map>
#include <memory>
#include <condition_variable>
#include "dbus/ObjectManagerInterfaceAdaptor.h"

class InputDevice;
class Script;

class ScriptManager:
	public org::freedesktop::DBus::ObjectManager_adaptor,
	public DBus::IntrospectableAdaptor,
	public DBus::ObjectAdaptor
{
public:
	ScriptManager (DBus::Connection &dbus_connection);
	virtual ~ScriptManager ();

	void addDevice (InputDevice *);
	void removeDevice (InputDevice *);

	virtual std::map<DBus::Path, std::map<std::string, std::map<std::string, DBus::Variant>>> GetManagedObjects ();
	static constexpr char DBusObjectPath[] = "/com/github/cvuchener/InputScripts/ScriptManager";

private:
	DBus::Connection &_dbus_connection;
	std::mutex _mutex;
	std::map<InputDevice *, std::unique_ptr<Script>> _scripts;
};

#endif

