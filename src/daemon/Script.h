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

#ifndef SCRIPT_H
#define SCRIPT_H

#include "JsHelpers/Thread.h"
#include "dbus/ScriptInterfaceAdaptor.h"

#include <string>
#include "InputDevice.h"


class Script:
	public JsHelpers::Thread,
	public com::github::cvuchener::InputScripts::Script_adaptor,
	public DBus::IntrospectableAdaptor,
	public DBus::PropertiesAdaptor,
	public DBus::ObjectAdaptor
{
public:
	Script (DBus::Connection &dbus_connection, std::string path, InputDevice *device);
	virtual ~Script ();

protected:
	virtual void run (JSContext *cx);
	virtual void on_set_property (DBus::InterfaceAdaptor &interface, const std::string &property, const DBus::Variant &value);

private:
	void readInputEvents (JSContext *, JS::HandleObject);

	std::string _filename;
	InputDevice *_device;
};

#endif
