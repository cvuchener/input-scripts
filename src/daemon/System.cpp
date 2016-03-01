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

#include "System.h"

#include "Log.h"
#include "JsHelpers/Thread.h"

System::System (JsHelpers::Thread *js_thread):
	_js_thread (js_thread)
{
}

System::~System ()
{
}

void System::setTimeout (MTFunction<void ()> callback, unsigned int delay)
{
	std::unique_lock<std::mutex> lock (_timers_mutex);
	_timers.emplace_back (std::chrono::milliseconds (delay));
	auto it = std::prev (_timers.end ());
	lock.unlock ();

	it->start ([this, callback, it] () {
		callback ();
		// The timer must be deleted asynchronously since the
		// destructor will wait for this call to end. And it must
		// be deleted on the JS thread since it may contains some
		// JS values.
		_js_thread->execOnJsThreadAsync ([this, it] () {
			_timers.erase (it);
		});
		return false;
	});
}

static std::string toString (JSContext *cx, JS::HandleValue value)
{
	if (value.isNull ()) {
		return "null";
	}
	else if (value.isUndefined ()) {
		return "undefined";
	}
	else if (value.isBoolean ()) {
		return (value.toBoolean () ? "true" : "false");
	}
	else if (value.isNumber ()) {
		return std::to_string (value.toNumber ());
	}
	else if (value.isString ()) {
		JSString *str = value.toString ();
		std::vector<char> buffer (JS_GetStringLength (str));
		JS_EncodeStringToBuffer (cx, str, &buffer[0], buffer.size ());
		return std::string (buffer.begin (), buffer.end ());
	}
	else if (value.isObject ()) {
		return "object";
	}
	else {
		return "type not implemented";
	}
}

bool System::print (JSContext *cx, JS::CallArgs &args)
{
	for (unsigned int i = 0; i < args.length (); ++i) {
		if (i != 0)
			Log::info () << ", ";
		Log::info () << toString (cx, args.get (i));
	}
	Log::info () << std::endl;
	args.rval ().setNull ();
	return true;
}

static void print_r_impl (std::ostream &out, JSContext *cx, JS::HandleValue value, unsigned int indent = 0)
{
	if (JS_IsArrayObject (cx, value)) {
		JS::RootedObject array (cx, value.toObjectOrNull ());
		unsigned int len;
		JS_GetArrayLength (cx, array, &len);
		out << "[" << std::endl;
		for (unsigned int i = 0; i < len; ++i) {
			JS::RootedValue elem (cx);
			JS_GetElement (cx, array, i, &elem);
			for (unsigned int j = 0; j < indent+1; ++j)
				out << "    ";
			print_r_impl (out, cx, elem, indent+1);
			if (i != len-1)
				out << ", ";
			out << std::endl;
		}
		for (unsigned int j = 0; j < indent; ++j)
			out << "    ";
		out << "]";
	}
	else if (value.isObject ()) {
		JS::RootedObject obj (cx, value.toObjectOrNull ());
		JSIdArray *ids = JS_Enumerate (cx, obj);
		unsigned int len = JS_IdArrayLength (cx, ids);
		out << "{" << std::endl;
		for (unsigned int i = 0; i < len; ++i) {
			JS::RootedId id (cx, JS_IdArrayGet (cx, ids, i));
			JS::RootedValue key (cx);
			JS_IdToValue (cx, id, &key);
			JS::RootedValue prop (cx);
			JS_GetPropertyById (cx, obj, id, &prop);
			for (unsigned int j = 0; j < indent+1; ++j)
				out << "    ";
			out << toString (cx, key) << ": ";
			print_r_impl (out, cx, prop, indent+1);
			if (i != len-1)
				out << ", ";
			out << std::endl;
		}
		for (unsigned int j = 0; j < indent; ++j)
			out << "    ";
		out << "}";
		JS_DestroyIdArray (cx, ids);
	}
	else if (value.isString ()) {
		out << '"' << toString (cx, value) << '"';
	}
	else {
		out << toString (cx, value);
	}
}

bool System::print_r (JSContext *cx, JS::CallArgs &args)
{
	for (unsigned int i = 0; i < args.length (); ++i) {
		print_r_impl (Log::info (), cx, args.get (i));
		Log::info () << std::endl;
	}
	args.rval ().setNull ();
	return true;
}

const JSClass System::js_class = JS_HELPERS_CLASS("System", System);

const JSFunctionSpec System::js_fs[] = {
	JS_HELPERS_METHOD("setTimeout", System::setTimeout),
	{
		"print",
		&JsHelpers::LLMethodWrapper<System, &System::print>,
		0, 0
	},
	{
		"print_r",
		&JsHelpers::LLMethodWrapper<System, &System::print_r>,
		0, 0
	},
	JS_FS_END
};

