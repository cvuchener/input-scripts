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

#ifndef JS_HELPERS_TYPE_CONVERSION_H
#define JS_HELPERS_TYPE_CONVERSION_H

#include <jsapi.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "Thread.h"

#include "../Log.h"

template <typename Sig>
class MTFunction: public std::function<Sig>
{
public:
	MTFunction () { }

	template <typename F>
	MTFunction (F f): std::function<Sig> (f) { }
};

namespace JsHelpers
{

// Assigning C++ values to JS::Value
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::string &str)
{
	var.setString (JS_NewStringCopyZ (cx, str.c_str ()));
}

template <typename T>
inline typename std::enable_if<std::is_integral<T>::value>::type
setJSValue (JSContext *cx, JS::MutableHandleValue var, T value)
{
	var.setInt32 (value);
}

inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, double value)
{
	var.setNumber (value);
}

template <typename T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::vector<T> &array)
{
	JS::RootedObject obj (cx, JS_NewArrayObject (cx, array.size ()));
	for (unsigned i = 0; i < array.size (); ++i) {
		JS::RootedValue elem (cx);
		setJSValue (cx, &elem, array.at (i));
		JS_SetElement (cx, obj, i, elem);
	}
	var.setObject (*obj);
}

template <typename T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::map<std::string, T> &properties)
{
	JS::RootedObject obj (cx, JS_NewObject (cx, nullptr));
	for (const auto &pair: properties) {
		JS::RootedValue value (cx);
		setJSValue (cx, &value, pair.second);
		JS_DefineProperty (cx, obj, pair.first.c_str (), value, JSPROP_ENUMERATE);
	}
	var.setObject (*obj);
}

// Converting JS::Value to C++ types
inline void readJSValue (JSContext *cx, std::string &var, JS::HandleValue value)
{
	if (!value.isString ()) {
		throw std::invalid_argument ("must be a string");
	}
	JSString *jsstr = value.toString ();
	std::vector<char> buffer (JS_GetStringLength (jsstr));
	JS_EncodeStringToBuffer (cx, jsstr, &buffer[0], buffer.size ());
	var.assign (buffer.begin (), buffer.end ());
}

template <typename T>
inline typename std::enable_if<std::is_integral<T>::value>::type
readJSValue (JSContext *cx, T &var, JS::HandleValue value)
{
	if (!value.isNumber ()) {
		throw std::invalid_argument ("must be a number");
	}
	double v = value.toNumber ();
	if (v < std::numeric_limits<T>::min () || v > std::numeric_limits<T>::max ()) {
		throw std::invalid_argument ("is out of range");
	}
	var = static_cast<T> (v);
}

inline void readJSValue (JSContext *cx, double &var, JS::HandleValue value)
{
	if (!value.isNumber ()) {
		throw std::invalid_argument ("must be a number");
	}
	var = value.toNumber ();
}

inline void readJSValue (JSContext *cx, bool &var, JS::HandleValue value)
{
	if (!value.isBoolean ()) {
		throw std::invalid_argument ("must be an boolean");
	}
	var = value.toBoolean ();
}

template <typename T>
inline void readJSValue (JSContext *cx, std::vector<T> &var, JS::HandleValue value)
{
	if (!JS_IsArrayObject (cx, value)) {
		throw std::invalid_argument ("must be an array");
	}
	JS::RootedObject array (cx, value.toObjectOrNull ());
	uint32_t length;
	JS_GetArrayLength (cx, array, &length);
	var.resize (length);
	for (unsigned i = 0; i < length; ++i) {
		JS::RootedValue elem (cx);
		JS_GetElement (cx, array, i, &elem);
		readJSValue (cx, var[i], elem);
	}
}

template <typename T>
inline void readJSValue (JSContext *cx, std::map<std::string, T> &var, JS::HandleValue value) {
	if (!value.isObject ()) {
		throw std::invalid_argument ("must be an object");
	}
	JS::RootedObject obj (cx, value.toObjectOrNull ());
	JSIdArray *ids = JS_Enumerate (cx, obj);
	unsigned int len = JS_IdArrayLength (cx, ids);
	for (unsigned int i = 0; i < len; ++i) {
		JS::RootedId id (cx, JS_IdArrayGet (cx, ids, i));
		JS::RootedValue js_key (cx);
		JS_IdToValue (cx, id, &js_key);
		std::string key;
		readJSValue (cx, key, js_key);

		JS::RootedValue prop (cx);
		JS_GetPropertyById (cx, obj, id, &prop);
		T value;
		readJSValue (cx, value, prop);

		var.emplace (key, value);
	}
	JS_DestroyIdArray (cx, ids);
}

template <int n, typename... Args>
struct ArgumentVector;

template <int n, typename First, typename... Rest>
struct ArgumentVector<n, First, Rest...>
{
	inline static void pack (JSContext *cx, JS::AutoValueVector &jsargs, First first, Rest... rest)
	{
		setJSValue (cx, jsargs[n], first);
		ArgumentVector<n+1, Rest...>::pack (cx, jsargs, rest...);
	}
};

template <int n>
struct ArgumentVector<n>
{
	inline static void pack (JSContext *cx, JS::AutoValueVector &jsargs)
	{
	}
};

template <typename R, typename... Args>
inline void readJSValue (JSContext *cx, MTFunction<R (Args...)> &var, JS::HandleValue value)
{
	std::shared_ptr<JS::PersistentRootedValue> fun (new JS::PersistentRootedValue (cx, value));
	Thread *thread = static_cast<Thread *> (JS_GetContextPrivate (cx));
	var = std::function<R (Args...)> ([cx, thread, fun] (Args... args) {
		return thread->execOnJsThreadSync<R> ([cx, fun, args...] () {
			JS::AutoValueVector jsargs (cx);
			jsargs.resize (sizeof... (Args));
			ArgumentVector<0, Args...>::pack (cx, jsargs, args...);
			JS::RootedValue rval (cx);
			JS_CallFunctionValue (cx, JS::NullPtr (), *fun, jsargs, &rval);
			R ret;
			readJSValue (cx, ret, rval);
			return ret;
		});
	});
}

template <typename... Args>
inline void readJSValue (JSContext *cx, MTFunction<void (Args...)> &var, JS::HandleValue value)
{
	std::shared_ptr<JS::PersistentRootedValue> fun (new JS::PersistentRootedValue (cx, value));
	Thread *thread = static_cast<Thread *> (JS_GetContextPrivate (cx));
	var = std::function<void (Args...)> ([cx, thread, fun] (Args... args) {
		thread->execOnJsThreadSync<int> ([cx, fun, args...] () {
			JS::AutoValueVector jsargs (cx);
			jsargs.resize (sizeof... (Args));
			ArgumentVector<0, Args...>::pack (cx, jsargs, args...);
			JS::RootedValue rval (cx);
			JS_CallFunctionValue (cx, JS::NullPtr (), *fun, jsargs, &rval);
			return 0;
		});
	});
}

}

#endif
