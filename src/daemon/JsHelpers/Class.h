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

#define STATIC_MEMBER_DETECTOR(member_type, name)                          \
template <typename T>                                                      \
class has_static_member_##name                                             \
{                                                                          \
	template <typename U, U *> struct check_t;                         \
                                                                           \
	template <typename U>                                              \
	static std::true_type test (check_t<member_type, &U::name> *);     \
                                                                           \
	template <typename U>                                              \
	static std::false_type test (...);                                 \
                                                                           \
public:                                                                    \
	static constexpr bool value = decltype (test<T> (0))::value;       \
};

#define STATIC_ARRAY_MEMBER_DETECTOR(member_type, name)                    \
template <typename T>                                                      \
class has_static_member_##name                                             \
{                                                                          \
	template <typename U, U> struct check_t;                           \
                                                                           \
	template <typename U>                                              \
	static std::true_type test (check_t<member_type, U::name> *);      \
                                                                           \
	template <typename U>                                              \
	static std::false_type test (...);                                 \
                                                                           \
public:                                                                    \
	static constexpr bool value = decltype (test<T> (0))::value;       \
};

#define STATIC_MEMBER_POINTER_OR_NULL(member_type, name)                        \
template <typename T>                                                           \
typename std::enable_if<has_static_member_##name<T>::value, member_type>::type  \
static_member_pointer_or_null_##name ()                                         \
{ return T::name; }                                                             \
template <typename T>                                                           \
typename std::enable_if<!has_static_member_##name<T>::value, member_type>::type \
static_member_pointer_or_null_##name ()                                         \
{ return nullptr; }

namespace JsHelpers
{

STATIC_ARRAY_MEMBER_DETECTOR(const JSPropertySpec *, js_ps)
STATIC_MEMBER_POINTER_OR_NULL(const JSPropertySpec *, js_ps)
STATIC_ARRAY_MEMBER_DETECTOR(const JSFunctionSpec *, js_fs)
STATIC_MEMBER_POINTER_OR_NULL(const JSFunctionSpec *, js_fs)
STATIC_ARRAY_MEMBER_DETECTOR(const JSPropertySpec *, js_static_ps)
STATIC_MEMBER_POINTER_OR_NULL(const JSPropertySpec *, js_static_ps)
STATIC_ARRAY_MEMBER_DETECTOR(const JSFunctionSpec *, js_static_fs)
STATIC_MEMBER_POINTER_OR_NULL(const JSFunctionSpec *, js_static_fs)
typedef std::pair<std::string, int> IntConstantSpec;
STATIC_ARRAY_MEMBER_DETECTOR(const IntConstantSpec *, js_int_const)
STATIC_MEMBER_POINTER_OR_NULL(const IntConstantSpec *, js_int_const)

template<typename T>
class BaseClass
{
public:
	BaseClass (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto, JSNative constructor, unsigned int nargs):
		_cx (cx),
		_proto (cx)
	{
		Log::debug () << "Initializing class " << T::js_class.name << std::endl;
		_proto = JS_InitClass (cx, obj, parent_proto, &T::js_class,
			constructor, nargs,
			static_member_pointer_or_null_js_ps<T> (),
			static_member_pointer_or_null_js_fs<T> (),
			static_member_pointer_or_null_js_static_ps<T> (),
			static_member_pointer_or_null_js_static_fs<T> ());
		JS::RootedObject js_constructor (cx, JS_GetConstructor (cx, _proto));
		const std::pair<std::string, int > *int_const = static_member_pointer_or_null_js_int_const<T> ();
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

	JSObject *newObjectFromPointer (T *ptr) const
	{
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
	JS::RootedObject _proto;
};

template<typename T>
class AbstractClass: public BaseClass<T>
{
public:
	AbstractClass (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto):
		BaseClass<T> (cx, obj, parent_proto, &constructor, 0)
	{
	}

private:
	static bool constructor (JSContext *cx, unsigned int argc, jsval *vp) {
		JS_ReportError (cx, "Class %s is abstract", T::js_class.name);
		return false;
	}
};

template<typename T, typename... ConstructorArgs>
class Class: public BaseClass<T>
{
public:
	Class (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto):
		BaseClass<T> (cx, obj, parent_proto,
			&constructorWrapper<&T::js_class, T, ConstructorArgs...>,
			sizeof... (ConstructorArgs))
	{
	}
};

}

#endif
