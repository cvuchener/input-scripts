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

#include "jstpl/Thread.h"
#include "dbus/ScriptInterfaceAdaptor.h"

#include <string>
#include "InputDevice.h"


class Script:
	public jstpl::Thread,
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
	static bool connectSignalWrapper (JSContext *cx, unsigned int argc, JS::Value *vp);
	bool connectSignal (JSContext *cx, unsigned int argc, JS::Value *vp);
	static bool disconnectSignalWrapper (JSContext *cx, unsigned int argc, JS::Value *vp);
	bool disconnectSignal (JSContext *cx, unsigned int argc, JS::Value *vp);

	std::string _filename;
	InputDevice *_device;

	std::map<int, sigc::connection> _signal_connections;
	int _next_signal_connection_index;
};

#endif
