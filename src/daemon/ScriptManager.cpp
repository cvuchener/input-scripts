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

#include "ScriptManager.h"

#include "Driver.h"
#include "InputDevice.h"
#include "Script.h"
#include "Log.h"

constexpr char ScriptManager::DBusObjectPath[];

using com::github::cvuchener::InputScripts::Script_adaptor;

ScriptManager::ScriptManager (DBus::Connection &dbus_connection):
	DBus::ObjectAdaptor (dbus_connection, DBusObjectPath),
	_dbus_connection (dbus_connection)
{
	for (auto it = Driver::begin (); it != Driver::end (); ++it) {
		Driver *driver = it->second.get ();
		driver->inputDeviceAdded = std::bind (&ScriptManager::addDevice, this, std::placeholders::_1);
		driver->inputDeviceRemoved = std::bind (&ScriptManager::removeDevice, this, std::placeholders::_1);
	}
}

ScriptManager::~ScriptManager ()
{
	for (auto pair: _scripts) {
		pair.second->stop ();
		delete pair.second;
	}
}

static std::map<std::string, std::map<std::string, DBus::Variant>> getScriptProperties (Script *script)
{
	DBus::IntrospectedInterface *script_interface = script->Script_adaptor::introspect ();

	std::map<std::string, std::map<std::string, DBus::Variant>> object_properties;

	auto &interface = object_properties[script_interface->name];
	for (const auto *property = script_interface->properties; property->name; ++property) {
		DBus::MessageIter m = interface[property->name].writer ();
		DBus::Variant *variant = script->Script_adaptor::get_property (property->name);
		m << *variant;
	}
	return object_properties;
}

void ScriptManager::addDevice (InputDevice *device)
{
	static int next_device = 0;
	std::stringstream name;
	name << "Device" << (next_device++);
	Log::info () << "Add new device script for "
		     << device->driver () << "/"
		     << device->name () << "/"
		     << device->serial () << std::endl;
	std::stringstream path;
	path << DBusObjectPath << "/" << name.str ();
	Script *script = new Script (_dbus_connection, path.str (), device);
	_scripts.emplace (device, script);

	InterfacesAdded (path.str (), getScriptProperties (script));

	script->start ();
}

void ScriptManager::removeDevice (InputDevice *device)
{
	Log::info () << "Remove device script for "
		     << device->driver () << "/"
		     << device->name () << "/"
		     << device->serial () << std::endl;
	Script *script = _scripts[device];

	DBus::Path path = script->path ();
	std::string interface_name = script->Script_adaptor::introspect ()->name;

	script->stop ();
	delete script;
	_scripts.erase (device);

	InterfacesRemoved (path, { interface_name });
}

std::map<DBus::Path, std::map<std::string, std::map<std::string, DBus::Variant>>> ScriptManager::GetManagedObjects ()
{
	std::map<DBus::Path, std::map<std::string, std::map<std::string, DBus::Variant>>> objects;
	for (auto pair: _scripts) {
		Script *script = pair.second;
		objects.emplace (script->path (), getScriptProperties (script));
	}
	return objects;
}

