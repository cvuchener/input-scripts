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

#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <cstdint>
#include <functional>

#include "JsHelpers/JsHelpers.h"

class InputDevice
{
public:
	virtual ~InputDevice ();

	virtual void interrupt () = 0;
	virtual void readEvents () = 0;

	virtual int32_t getValue (uint16_t type, uint16_t code) = 0;

	virtual std::string driver () const = 0;
	virtual std::string name () const = 0;
	virtual std::string serial () const = 0;

	virtual operator bool () const = 0;

	std::function<void (uint16_t, uint16_t, int32_t)> eventRead;

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	typedef JsHelpers::AbstractClass<InputDevice> JsClass;

	virtual JSObject *makeJsObject (JSContext *cx, JS::HandleObject obj) = 0;
};

#endif
