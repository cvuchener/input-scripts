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

#ifndef DBUS_PROXY_H
#define DBUS_PROXY_H

#include <dbus-c++/dbus.h>
#include "../JsHelpers/JsHelpers.h"

class DBusProxy:
	public DBus::InterfaceProxy,
	public DBus::ObjectProxy
{
public:
	DBusProxy (int bus, std::string service, std::string path, std::string interface);
	~DBusProxy ();

	bool call (JSContext *cx, JS::CallArgs &args);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::Class<DBusProxy, int, std::string, std::string, std::string> JsClass;

private:
	static bool _registered;
};

#endif
