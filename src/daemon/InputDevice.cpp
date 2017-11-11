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

#include "InputDevice.h"

#include "ClassManager.h"
#include "JsHelpers/Signal.h"

extern "C" {
#include <linux/input.h>
}

InputDevice::~InputDevice ()
{
}

bool InputDevice::keyPressed (uint16_t code)
{
	return getEvent ({
		{ "type", EV_KEY },
		{ "code", code }
	})["value"] > 0;
}

int32_t InputDevice::getAxisValue (uint16_t code)
{
	return getEvent ({
		{ "type", EV_ABS },
		{ "code", code }
	})["value"];
}

void InputDevice::eventRead (const Event &e)
{
	event.emit (e);
}

void InputDevice::simpleEventRead (uint16_t type, uint16_t code, int32_t value)
{
	simpleEvent.emit (type, code, value);
	eventRead ({
		{ "type", type },
		{ "code", code },
		{ "value", value },
	});
}

const JSClass InputDevice::js_class = JS_HELPERS_CLASS("InputDevice", InputDevice);

const JSFunctionSpec InputDevice::js_fs[] = {
	JS_HELPERS_METHOD("getEvent", InputDevice::getEvent),
	JS_HELPERS_METHOD("getSimpleEvent", InputDevice::getSimpleEvent),
	JS_HELPERS_METHOD("keyPressed", InputDevice::keyPressed),
	JS_HELPERS_METHOD("getAxisValue", InputDevice::getAxisValue),
	JS_FS_END
};

const JsHelpers::SignalMap InputDevice::js_signals = {
	{ "event", JsHelpers::make_signal_connector (&InputDevice::event) },
	{ "simpleEvent", JsHelpers::make_signal_connector (&InputDevice::simpleEvent) },
};

bool InputDevice::_registered = ClassManager::registerClass<InputDevice::JsClass> ();
