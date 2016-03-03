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

#include "Script.h"

#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "Log.h"
#include "Config.h"

#include "classes/UInput.h"

#include "System.h"

extern "C" {
#include <libevdev/libevdev.h>
}

using com::github::cvuchener::InputScripts::Script_adaptor;

Script::Script (DBus::Connection &dbus_connection, std::string path, InputDevice *device):
	DBus::ObjectAdaptor (dbus_connection, path),
	_device (device)
{
	for (const auto &script: Config::config.default_scripts) {
		bool match = true;
		for (const auto &rule: script.rules) {
			if (rule.first == "driver") {
				if (rule.second != device->driver ()) {
					match = false;
					break;
				}
			}
			else if (rule.first == "name") {
				if (rule.second != device->name ()) {
					match = false;
					break;
				}
			}
			else if (rule.first == "serial") {
				if (rule.second != device->serial ()) {
					match =false;
					break;
				}
			}
		}
		if (match) {
			_filename = script.script_file;
			break;
		}
	}

	Script_adaptor::driver = device->driver ();
	Script_adaptor::name = device->name ();
	Script_adaptor::serial = device->serial ();
	Script_adaptor::file = _filename;
}

Script::~Script ()
{
}

static const JSClass global_class = {
	"global",
	JSCLASS_GLOBAL_FLAGS
};

void Script::readInputEvents (JSContext *cx, JS::HandleObject script_object)
{
	_device->eventRead = [this, cx, script_object] (uint16_t type, uint16_t code, int32_t value) {
		execOnJsThreadAsync ([cx, script_object, type, code, value] () {
			JS::AutoValueArray<3> args (cx);
			args[0].setNumber (static_cast<uint32_t> (type));
			args[1].setNumber (static_cast<uint32_t> (code));
			args[2].setNumber (static_cast<double> (value));
			JS::RootedValue rval (cx);
			JS_CallFunctionName (cx, script_object, "event", args, &rval);
		});
	};

	while (!_stopping && *_device) {
		_device->readEvents ();
	}
	_stopping = true;
	execOnJsThreadAsync ([] () {}); // Wake up the script thread
}

static JSObject *getScriptObject (JSContext *cx, std::string filename)
{
	std::string filepath;
	std::string script;
	filepath = Config::getConfigFilePath (filename);
	std::ifstream file (filepath);
	file.seekg (0, std::ios::end);
	script.reserve (file.tellg ());
	file.seekg (0, std::ios::beg);
	script.assign (std::istreambuf_iterator<char> (file),
		       std::istreambuf_iterator<char> ());

	JS::CompileOptions compileOptions (cx);
	compileOptions.setFile (filepath.c_str ());

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16cvt;
	std::u16string utf16 = utf16cvt.from_bytes (script.data ());
	JS::SourceBufferHolder src_buf (utf16.data (), utf16.size (), JS::SourceBufferHolder::NoOwnership);

	JS::AutoObjectVector scope_chain (cx);
	scope_chain.append (JS_NewObject (cx, nullptr));
	JS::RootedValue rval (cx);
	if (!JS::Evaluate (cx, scope_chain, compileOptions, src_buf, &rval)) {
		throw std::runtime_error ("Script evaluation failed");
	}
	return scope_chain.popCopy ();
}

void Script::run (JSContext *cx)
{
	JSAutoRequest ar (cx);

	JS::RootedObject global (cx);
	global = JS_NewGlobalObject (cx, &global_class, nullptr, JS::DontFireOnNewGlobalHook);
	if (!global) {
		throw std::runtime_error ("JS_NewGlobalObject failed");
	}

	JSAutoCompartment ac (cx, global);

	// Init classes

	// Standard
	if (!JS_InitStandardClasses (cx, global)) {
		throw std::runtime_error ("JS_InitStandardClasses");
	}

	// C++ classes
	UInput::JsClass uinput_class (cx, global, JS::NullPtr ());

	// System object
	System system (this);
	System::JsClass system_class (cx, global, JS::NullPtr ());
	JS::RootedObject system_object (cx);
	system_object = system_class.newObjectFromPointer (&system);
	JS_DefineProperty (cx, global, "system", system_object, JSPROP_ENUMERATE);

	// Input constants
	for (uint16_t type = 0; type <= EV_MAX; ++type) {
		const char *name = libevdev_event_type_get_name (type);
		if (!name)
			continue;
		JS::RootedValue value (cx);
		value.setNumber (static_cast<uint32_t> (type));
		JS_DefineProperty (cx, global, name, value, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
		int max = libevdev_event_type_get_max (type);
		if (max == -1)
			continue;
		for (uint16_t code = 0; code <= max; ++code) {
			const char *name = libevdev_event_code_get_name (type, code);
			if (!name)
				continue;
			value.setNumber (static_cast<uint32_t> (code));
			JS_DefineProperty (cx, global, name, value, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
		}
	}

	// Library scripts
	for (auto pair: Config::config.library_scripts) {
		try {
			JS::RootedObject lib (cx, getScriptObject (cx, pair.second));
			JS_DefineProperty (cx, global, pair.first.c_str (), lib, JSPROP_ENUMERATE);
		}
		catch (std::exception &e) {
			Log::error () << "Loading library script "
				      << pair.first << " (" << pair.second
				      << ") failed: " << e.what () << std::endl;
		}
	}

	// Create InputEvent JS object
	JS::RootedObject input_object (cx);
	input_object = _device->makeJsObject (cx, global);
	JS_DefineProperty (cx, global, "input", input_object, JSPROP_ENUMERATE);

	// Execute user script and retrieve the prototype
	JS::RootedObject script_proto (cx);
	try {
		script_proto = getScriptObject (cx, _filename);
	}
	catch (std::exception &e) {
		Log::error () << "Failed to load script " << _filename
			      << ": " << e.what () << std::endl;
		throw std::runtime_error ("invalid script file");
	}

	// Create object from the script prototype
	JS::RootedObject script_object (cx);
	script_object = JS_NewObjectWithGivenProto (cx, nullptr, script_proto);

	// Call init function
	JS::AutoValueVector args (cx);
	JS::RootedValue rval (cx);
	if (!JS_CallFunctionName (cx, script_object, "init", args, &rval))
		throw std::runtime_error ("init function failed");

	// Start reading inputs
	std::thread input_thread (&Script::readInputEvents, this, cx, JS::HandleObject (script_object));

	// Perform tasks
	exec ();

	// Stop inputs
	_device->interrupt ();
	input_thread.join ();

	// Call finalize function
	if (!JS_CallFunctionName (cx, script_object, "finalize", args, &rval))
		throw std::runtime_error ("finalize function failed");
}

void Script::on_set_property (DBus::InterfaceAdaptor &interface, const std::string &property, const DBus::Variant &value)
{
	if (property == "file") {
		stop ();
		_filename = value.operator std::string ();
		Log::info () << "Script set to " << _filename << std::endl;
		start ();
	}
}
