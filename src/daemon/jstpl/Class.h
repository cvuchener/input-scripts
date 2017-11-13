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

#ifndef JSTPL_CLASS_H
#define JSTPL_CLASS_H

#include "Functions.h"

#include <sigc++/signal.h>
#include <cassert>
#include "detector.h"

#define STATIC_ARRAY_POINTER_OR_NULL(expected, name) \
template<typename T> \
using has_##name##_array_t = std::enable_if_t<std::is_array_v<decltype (T::name)>, std::remove_extent_t<decltype (T::name)>>; \
template<typename T> \
std::enable_if_t<is_detected_exact_v<expected, has_##name##_array_t, T>, expected *> \
static_array_pointer_or_null_##name () { return T::name; }; \
template<typename T> \
std::enable_if_t<!is_detected_exact_v<expected, has_##name##_array_t, T>, expected *> \
static_array_pointer_or_null_##name () { return nullptr; };

#define STATIC_MEMBER_POINTER_OR_NULL(expected, name) \
template<typename T> \
using has_##name##_member_t = decltype (T::name); \
template<typename T> \
std::enable_if_t<is_detected_exact_v<expected, has_##name##_member_t, T>, expected *> \
static_member_pointer_or_null_##name () { return &T::name; }; \
template<typename T> \
std::enable_if_t<!is_detected_exact_v<expected, has_##name##_member_t, T>, expected *> \
static_member_pointer_or_null_##name () { return nullptr; };

namespace jstpl
{

using IntConstantSpec = std::pair<std::string, int>;
using SignalConnector = std::function<sigc::connection (JSContext *, JS::HandleValue obj, JS::HandleValue)>;
using SignalMap = std::map<std::string, SignalConnector>;

STATIC_ARRAY_POINTER_OR_NULL(const JSPropertySpec, js_ps)
STATIC_ARRAY_POINTER_OR_NULL(const JSFunctionSpec, js_fs)
STATIC_ARRAY_POINTER_OR_NULL(const JSPropertySpec, js_static_ps)
STATIC_ARRAY_POINTER_OR_NULL(const JSFunctionSpec, js_static_fs)
STATIC_ARRAY_POINTER_OR_NULL(const IntConstantSpec, js_int_const)

STATIC_MEMBER_POINTER_OR_NULL(const SignalMap, js_signals)

class BaseClass
{
public:
	template<typename T>
	class Type { };

	template<typename T>
	BaseClass (Type<T> &&, JSContext *cx, JS::HandleObject obj, const BaseClass *parent, JSNative constructor, unsigned int nargs):
		_cx (cx),
		_class (&T::js_class),
		_parent (parent),
		_proto (cx),
		_signals (static_member_pointer_or_null_js_signals<T> ())
	{
		Log::debug () << "Initializing class " << T::js_class.name << std::endl;
		JS::RootedObject parent_proto (cx, parent ? parent->prototype () : JS::NullPtr ());
		_proto = JS_InitClass (cx, obj, parent_proto, &T::js_class,
			constructor, nargs,
			static_array_pointer_or_null_js_ps<T> (),
			static_array_pointer_or_null_js_fs<T> (),
			static_array_pointer_or_null_js_static_ps<T> (),
			static_array_pointer_or_null_js_static_fs<T> ());
		JS::RootedObject js_constructor (cx, JS_GetConstructor (cx, _proto));
		auto int_const = static_array_pointer_or_null_js_int_const<T> ();
		if (int_const) {
			unsigned int i = 0;
			while (!int_const[i].first.empty ()) {
				JS::RootedValue value (cx);
				setJSValue (cx, &value, int_const[i].second);
				JS_DefineProperty (cx, js_constructor,
					int_const[i].first.c_str (), value,
					JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
				++i;
			}
		}
	}

	template <typename T>
	JSObject *newObjectFromPointer (T *ptr) const
	{
		assert (&T::js_class == _class);
		JSObject *obj = JS_NewObjectWithGivenProto (_cx, &T::js_class, _proto);
		auto data = new std::pair<bool, T*> (false, ptr);
		JS_SetPrivate (obj, data);
		return obj;
	}

	JS::HandleObject prototype () const
	{
		return _proto;
	}

	const char *name () const
	{
		return _class->name;
	}

	sigc::connection connect (JS::HandleValue obj, const std::string &signal_name, JS::HandleValue callback) const
	{
		if (_signals) {
			auto it = _signals->find (signal_name);
			if (it != _signals->end ())
				return it->second (_cx, obj, callback);
		}
		if (_parent)
			return _parent->connect (obj, signal_name, callback);
		else
			throw std::invalid_argument ("Unknown signal name");
	}

private:
	JSContext *_cx;
	const JSClass *_class;
	const BaseClass *_parent;
	JS::RootedObject _proto;
	const std::map<std::string, SignalConnector> *_signals;
};

template<typename T>
class AbstractClass: public BaseClass
{
public:
	typedef T type;

	AbstractClass (JSContext *cx, JS::HandleObject obj, const BaseClass *parent):
		BaseClass (BaseClass::Type<T> (), cx, obj, parent, &constructor, 0)
	{
	}

private:
	static bool constructor (JSContext *cx, unsigned int argc, jsval *vp) {
		JS_ReportError (cx, "Class %s is abstract", T::js_class.name);
		return false;
	}
};

template<typename T, typename... ConstructorArgs>
class Class: public BaseClass
{
public:
	typedef T type;

	Class (JSContext *cx, JS::HandleObject obj, const BaseClass *parent):
		BaseClass (BaseClass::Type<T> (), cx, obj, parent,
			&constructorWrapper<&T::js_class, T, ConstructorArgs...>,
			sizeof... (ConstructorArgs))
	{
	}
};

}

#endif
