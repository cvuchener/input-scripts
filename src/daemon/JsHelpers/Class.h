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

#ifndef JS_HELPERS_CLASS_H
#define JS_HELPERS_CLASS_H

#include "Functions.h"

#include <cassert>
#include <experimental/type_traits>

#define STATIC_ARRAY_POINTER_OR_NULL(name) \
template<typename T> \
using has_##name##_member_t = typename std::remove_extent<decltype (T::name)>::type; \
template<typename T, typename Class> \
std::enable_if_t<std::is_same<T, std::experimental::detected_t<has_##name##_member_t, Class>>::value, T *> \
static_array_pointer_or_null_##name () { return Class::name; }; \
template<typename T, typename Class> \
std::enable_if_t<!std::is_same<T, std::experimental::detected_t<has_##name##_member_t, Class>>::value, T *> \
static_array_pointer_or_null_##name () { return nullptr; };

namespace JsHelpers
{

STATIC_ARRAY_POINTER_OR_NULL(js_ps)
STATIC_ARRAY_POINTER_OR_NULL(js_fs)
STATIC_ARRAY_POINTER_OR_NULL(js_static_ps)
STATIC_ARRAY_POINTER_OR_NULL(js_static_fs)
typedef std::pair<std::string, int> IntConstantSpec;
STATIC_ARRAY_POINTER_OR_NULL(js_int_const)

class BaseClass
{
public:
	template<typename T>
	class Type { };

	template<typename T>
	BaseClass (Type<T> &&, JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto, JSNative constructor, unsigned int nargs):
		_cx (cx),
		_class (&T::js_class),
		_proto (cx)
	{
		Log::debug () << "Initializing class " << T::js_class.name << std::endl;
		_proto = JS_InitClass (cx, obj, parent_proto, &T::js_class,
			constructor, nargs,
			static_array_pointer_or_null_js_ps<const JSPropertySpec, T> (),
			static_array_pointer_or_null_js_fs<const JSFunctionSpec, T> (),
			static_array_pointer_or_null_js_static_ps<const JSPropertySpec, T> (),
			static_array_pointer_or_null_js_static_fs<const JSFunctionSpec, T> ());
		JS::RootedObject js_constructor (cx, JS_GetConstructor (cx, _proto));
		const IntConstantSpec *int_const = static_array_pointer_or_null_js_int_const<const IntConstantSpec, T> ();
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

private:
	JSContext *_cx;
	const JSClass *_class;
	JS::RootedObject _proto;
};

template<typename T>
class AbstractClass: public BaseClass
{
public:
	AbstractClass (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto):
		BaseClass (BaseClass::Type<T> (), cx, obj, parent_proto, &constructor, 0)
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
	Class (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto):
		BaseClass (BaseClass::Type<T> (), cx, obj, parent_proto,
			&constructorWrapper<&T::js_class, T, ConstructorArgs...>,
			sizeof... (ConstructorArgs))
	{
	}
};

}

#endif
