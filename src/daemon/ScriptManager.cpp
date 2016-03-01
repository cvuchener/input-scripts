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

ScriptManager::ScriptManager (DBus::Connection &dbus_connection, const std::map<std::string, Driver *> *drivers):
	DBus::ObjectAdaptor (dbus_connection, DBusObjectPath),
	_dbus_connection (dbus_connection),
	_drivers (drivers)
{
	for (auto pair: *_drivers) {
		Driver *driver = pair.second;
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
	script->start ();
}

void ScriptManager::removeDevice (InputDevice *device)
{
	Log::info () << "Remove device script for "
		     << device->driver () << "/"
		     << device->name () << "/"
		     << device->serial () << std::endl;
	Script *script = _scripts[device];
	script->stop ();
	delete script;
	_scripts.erase (device);
}

std::map<DBus::Path, std::map<std::string, std::map<std::string, DBus::Variant>>> ScriptManager::GetManagedObjects ()
{
	using com::github::cvuchener::InputScripts::Script_adaptor;
	std::map<DBus::Path, std::map<std::string, std::map<std::string, DBus::Variant>>> objects;
	for (auto pair: _scripts) {
		Script *script = pair.second;
		Log::debug () << "Object = " << script->path () << std::endl;
		auto &object = objects[script->path ()];
		DBus::IntrospectedInterface *script_interface = script->Script_adaptor::introspect ();
		Log::debug () << "Interface = " << script_interface->name << std::endl;
		auto &interface = object[script_interface->name];
		const DBus::IntrospectedProperty *property = script_interface->properties;
		for (; property->name; ++property) {
			//if (std::string (property->name) == "file")
			//	continue;
			//Log::debug () << "Property: " << property->name << " = " << script->Script_adaptor::get_property (property->name)->operator std::string () << std::endl;
			Log::debug () << "Property: " << property->name << " (" << property->type << ")" << std::endl;
			DBus::MessageIter m = interface[property->name].writer ();
			//DBus::MessageIter var = m.new_variant (property->type);
			DBus::Variant *oldvar = script->Script_adaptor::get_property (property->name);
			m << *oldvar;
			//oldvar->reader ().copy_data (var);
			//var.append_string (oldvar->reader ().get_string ());
			//m.close_container (var);
		}
	}
	return objects;
}

