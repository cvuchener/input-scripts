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

#ifndef JSTPL_TYPE_CONVERSION_H
#define JSTPL_TYPE_CONVERSION_H

#include <jsapi.h>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <type_traits>
#include "detector.h"
#include "Thread.h"

#include "../Log.h"

namespace jstpl
{

template <typename T>
using is_js_class_t = decltype (T::js_class);

// Assigning C++ values to JS::Value
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::string &str);
template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, T value);
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, double value);
template <typename T, typename = std::enable_if_t<is_detected_exact_v<const JSClass, is_js_class_t, T>>>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, T *object);
template <typename T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::vector<T> &array);
template <typename T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::map<std::string, T> &properties);
template <typename... T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::variant<T...> &variant);

// Converting JS::Value to C++ types
inline void readJSValue (JSContext *cx, std::string &var, JS::HandleValue value);
template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline void readJSValue (JSContext *cx, T &var, JS::HandleValue value);
inline void readJSValue (JSContext *cx, double &var, JS::HandleValue value);
inline void readJSValue (JSContext *cx, bool &var, JS::HandleValue value);
inline void readJSValue (JSContext *cx, std::vector<bool>::reference var, JS::HandleValue value);
template <typename T>
inline void readJSValue (JSContext *cx, std::vector<T> &var, JS::HandleValue value);
template <typename T>
inline void readJSValue (JSContext *cx, std::map<std::string, T> &var, JS::HandleValue value);
template <typename R, typename... Args>
inline void readJSValue (JSContext *cx, std::function<R (Args...)> &var, JS::HandleValue value);
template <typename... Args>
inline void readJSValue (JSContext *cx, std::function<void (Args...)> &var, JS::HandleValue value);
template <typename T, typename = std::enable_if_t<is_detected_exact_v<const JSClass, is_js_class_t, T>>>
void readJSValue (JSContext *cx, T *&var, JS::HandleValue value);
template <typename... T>
void readJSValue (JSContext *cx, std::variant<T...> &var, JS::HandleValue value);

}

#include "ClassManager.h"

// Define templates
namespace jstpl
{

// Assigning C++ values to JS::Value
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::string &str)
{
	var.setString (JS_NewStringCopyZ (cx, str.c_str ()));
}

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, T value)
{
	var.setInt32 (value);
}

inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, double value)
{
	var.setNumber (value);
}

template <typename T, typename = std::enable_if_t<is_detected_exact_v<const JSClass, is_js_class_t, T>>>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, T *object)
{
	Thread *thread = static_cast<Thread *> (JS_GetContextPrivate (cx));
	auto p = thread->getClass (T::js_class.name);
	if (!p)
		throw std::runtime_error ("Class not found");
	var.setObject (*p->newObjectFromPointer (object));
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

template <typename... T>
inline void setJSValue (JSContext *cx, JS::MutableHandleValue var, const std::variant<T...> &variant)
{
	std::visit ([cx, var] (const auto &value) { setJSValue (cx, var, value); }, variant);
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

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline void readJSValue (JSContext *cx, T &var, JS::HandleValue value)
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

inline void readJSValue (JSContext *cx, std::vector<bool>::reference var, JS::HandleValue value)
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
inline void readJSValue (JSContext *cx, std::map<std::string, T> &var, JS::HandleValue value)
{
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

namespace detail {
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
}

template <typename R, typename... Args>
inline void readJSValue (JSContext *cx, std::function<R (Args...)> &var, JS::HandleValue value)
{
	auto fun = std::make_shared<JS::PersistentRootedValue> (cx, value);
	Thread *thread = static_cast<Thread *> (JS_GetContextPrivate (cx));
	var = std::function<R (Args...)> ([cx, thread, fun] (Args... args) {
		return thread->execOnJsThreadSync<R> ([cx, fun, args...] () {
			JS::AutoValueVector jsargs (cx);
			jsargs.resize (sizeof... (Args));
			detail::ArgumentVector<0, Args...>::pack (cx, jsargs, args...);
			JS::RootedValue rval (cx);
			JS_CallFunctionValue (cx, JS::NullPtr (), *fun, jsargs, &rval);
			R ret;
			readJSValue (cx, ret, rval);
			return ret;
		});
	});
}

template <typename... Args>
inline void readJSValue (JSContext *cx, std::function<void (Args...)> &var, JS::HandleValue value)
{
	auto fun = std::make_shared<JS::PersistentRootedValue> (cx, value);
	Thread *thread = static_cast<Thread *> (JS_GetContextPrivate (cx));
	var = std::function<void (Args...)> ([cx, thread, fun] (Args... args) {
		thread->execOnJsThreadSync<int> ([cx, fun, args...] () {
			JS::AutoValueVector jsargs (cx);
			jsargs.resize (sizeof... (Args));
			detail::ArgumentVector<0, Args...>::pack (cx, jsargs, args...);
			JS::RootedValue rval (cx);
			JS_CallFunctionValue (cx, JS::NullPtr (), *fun, jsargs, &rval);
			return 0;
		});
	});
}

namespace detail {
	template <typename Variant, typename... T>
	struct VariantAlternative;

	template <typename Variant, typename T, typename... Rest>
	struct VariantAlternative<Variant, T, Rest...>
	{
		inline static Variant readJS (JSContext *cx, JS::HandleValue value)
		{
			try {
				T var;
				readJSValue (cx, var, value);
				return var;
			}
			catch (std::exception &e) {
				return VariantAlternative<Variant, Rest...>::readJS (cx, value);
			}
		}
	};

	template <typename Variant>
	struct VariantAlternative<Variant>
	{
		inline static Variant readJS (JSContext *, JS::HandleValue)
		{
			throw std::invalid_argument ("not convertible to any variant alternative");
		}
	};
}

template <typename... T>
void readJSValue (JSContext *cx, std::variant<T...> &var, JS::HandleValue value)
{
	var = detail::VariantAlternative<std::variant<T...>, T...>::readJS (cx, value);
}

template <typename T, typename = std::enable_if_t<is_detected_exact_v<const JSClass, is_js_class_t, T>>>
inline void readJSValue (JSContext *cx, T *&var, JS::HandleValue value)
{
	if (!value.isObject ()) {
		throw std::invalid_argument ("must be an object");
	}
	JS::RootedObject obj (cx, value.toObjectOrNull ());
	const JSClass *cls = JS_GetClass (obj);
	if (!ClassManager::inherits (cls->name, T::js_class.name)) {
		Log::debug () << cls->name << " is not convertible to " << T::js_class.name << std::endl;
		throw std::runtime_error ("object type mismatch");
	}
	auto data = static_cast<std::pair<bool, T *> *> (JS_GetPrivate (obj));
	var = data->second;
}

} // namespace jstpl

#endif
