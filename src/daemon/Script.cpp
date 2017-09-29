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

#include "ClassManager.h"

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
			Log::info () << "Default script is " << _filename << std::endl;
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
	JSCLASS_GLOBAL_FLAGS,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	JS_GlobalObjectTraceHook
};

void Script::readInputEvents (JSContext *cx, JS::HandleObject script_object)
{
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

static bool importJSScript (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
	std::string filename;
	try {
		JsHelpers::readJSValue (cx, filename, jsargs.get (0));
	}
	catch (std::invalid_argument &e) {
		JS_ReportError (cx, "Invalid argument: %s", e.what ());
		return false;
	}
	try {
		jsargs.rval ().setObject (*getScriptObject (cx, filename));
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "Script error : %s", e.what ());
		return false;
	}
	return true;
}

bool Script::connectSignalWrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	Script *script = static_cast<Script *> (JS_GetContextPrivate (cx));
	return script->connectSignal (cx, argc, vp);
}

bool Script::connectSignal (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);

	auto ptr = jsargs.get (0).toObjectOrNull ();
	if (!ptr) {
		JS_ReportError (cx, "Argument 0 must be a valid object");
		return false;
	}
	JS::RootedObject obj (cx, ptr);
	const JSClass *cls = JS_GetClass (obj);
	auto p = getClass (cls->name);
	if (!p) {
		JS_ReportError (cx, "Unknown class: %s", cls->name);
		return false;
	}

	std::string signal_name;
	try {
		JsHelpers::readJSValue (cx, signal_name, jsargs.get (1));
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "Invalid argument 1: %s", e.what ());
		return false;
	}

	try {
		auto c = p->connect (jsargs.get (0), signal_name, jsargs.get (2));
		auto it = _signal_connections.lower_bound (_next_signal_connection_index);
		while (it != _signal_connections.end () &&
		       it->first == _next_signal_connection_index)
			++it, ++_next_signal_connection_index;
		_signal_connections.emplace_hint (it, _next_signal_connection_index, c);
		JsHelpers::setJSValue (cx, jsargs.rval (), _next_signal_connection_index);
		++_next_signal_connection_index;
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "Failed to connect signal: %s", e.what ());
		return false;
	}
}

bool Script::disconnectSignalWrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	Script *script = static_cast<Script *> (JS_GetContextPrivate (cx));
	return script->disconnectSignal (cx, argc, vp);
}

bool Script::disconnectSignal (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);

	try {
		int index;
		JsHelpers::readJSValue (cx, index, jsargs.get (0));
		auto it = _signal_connections.find (index);
		if (it == _signal_connections.end ()) {
			JS_ReportError (cx, "Signal index not found");
			return false;
		}
		it->second.disconnect ();
		_signal_connections.erase (it);
		return true;
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "Could not disconnect signal: %s", e.what ());
		return false;
	}
}

void Script::run (JSContext *cx)
{
	JSAutoRequest ar (cx);

	JS::RootedObject global (cx);
	global = JS_NewGlobalObject (cx, &global_class, nullptr, JS::FireOnNewGlobalHook);
	if (!global) {
		throw std::runtime_error ("JS_NewGlobalObject failed");
	}

	JSAutoCompartment ac (cx, global);

	// Init classes

	// Standard
	if (!JS_InitStandardClasses (cx, global)) {
		throw std::runtime_error ("JS_InitStandardClasses");
	}

	// import function
	JS_DefineFunction (cx, global, "importScript", importJSScript, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	// signal functions
	JS_DefineFunction (cx, global, "connect", connectSignalWrapper, 3, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction (cx, global, "disconnect", disconnectSignalWrapper, 3, JSPROP_READONLY | JSPROP_PERMANENT);

	// C++ classes
	addClasses (ClassManager::initClasses (cx, global));

	// System object
	System system (this);
	System::JsClass system_class (cx, global, nullptr);
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

	// Create InputEvent JS object
	JS::RootedObject input_object (cx);
	input_object = _device->makeJsObject (this);
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

	// disconnect all remaining signals
	for (auto &p: _signal_connections)
		p.second.disconnect ();
	_signal_connections.clear ();

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
