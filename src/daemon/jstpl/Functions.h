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

#ifndef JSTPL_WRAPPERS_H
#define JSTPL_WRAPPERS_H

#include "Types.h"

#include <typeinfo>
#include <tuple>

namespace jstpl
{

/*
 * Convert JS args to C++ args
 */
template <typename R, typename... Args>
struct ArgConvert
{
	template <R (*function) (JSContext *, JS::CallArgs &, Args...), typename... Converted>
	struct Wrapper
	{
		static inline R call (JSContext *cx, JS::CallArgs &jsargs, Converted... args)
		{
			constexpr std::size_t n = sizeof...(Converted);
			typedef typename std::tuple_element<n, std::tuple<Args...>>::type type;
			typedef typename std::remove_cv<typename std::remove_reference<type>::type>::type base_type;
			typedef typename ArgConvert<R, Args...>::template Wrapper<function, Converted..., type> wrapper;
			base_type arg;
			try {
				readJSValue (cx, arg, jsargs.get (n));
			}
			catch (std::invalid_argument e) {
				std::string what  = "Argument " + std::to_string (n);
				throw std::invalid_argument (what + " " + e.what ());
			}
			return wrapper::call (cx, jsargs, args..., arg);
		}
	};


	template <R (*function) (JSContext *, JS::CallArgs &, Args...)>
	struct Wrapper<function, Args...>
	{
		static inline R call (JSContext *cx, JS::CallArgs &jsargs, Args... args)
		{
			return function (cx, jsargs, args...);
		}
	};
};


/*
 * Template functions for using with ArgConvert::Wrapper
 */
template <typename R, typename... Args>
struct functionCall
{
	template <R (*function) (Args...)>
	static R call (JSContext *cx, JS::CallArgs &jsargs, Args... args)
	{
		return function (args...);
	}
};

template <typename T, typename R, typename... Args>
struct methodCall
{
	template <R (T::*method) (Args...)>
	static R call (JSContext *cx, JS::CallArgs &jsargs, Args... args)
	{
		JSObject *obj = jsargs.thisv ().toObjectOrNull ();
		auto data = static_cast<std::pair<bool, T *> *> (JS_GetPrivate (obj));
		T *ptr = data->second;
		return (ptr->*method) (args...);
	}
};

template <typename T, typename R, typename... Args>
struct constMethodCall
{
	template <R (T::*method) (Args...) const>
	static R call (JSContext *cx, JS::CallArgs &jsargs, Args... args)
	{
		JSObject *obj = jsargs.thisv ().toObjectOrNull ();
		auto data = static_cast<std::pair<bool, T *> *> (JS_GetPrivate (obj));
		const T *ptr = data->second;
		return (ptr->*method) (args...);
	}
};

template <typename T, typename... Args>
T *constructorCall (JSContext *cx, JS::CallArgs &jsargs, Args... args)
{
	return new T (args...);
}

/*
 * C++ method wrappers
 */

template <typename Sig>
struct MethodWrap;

// Make method wrapper with return value
template <typename T, typename R, typename... Args>
struct MethodWrap<R (T::*) (Args...)>
{
	template <R (T::*method) (Args...)>
	static bool wrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
	{
		JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
		if (jsargs.length () < sizeof...(Args)) {
			JS_ReportError (cx, "Too few arguments (%d required)", sizeof...(Args));
			return false;
		}
		typedef typename ArgConvert<R, Args...>::template Wrapper<methodCall<T, R, Args...>::template call<method>> wrapper;
		R ret;
		try {
			ret = wrapper::call (cx, jsargs);
		}
		catch (std::exception &e) {
			JS_ReportError (cx, "C++ exception: %s", e.what ());
			return false;
		}
		setJSValue (cx, jsargs.rval (), ret);
		return true;
	}
};

// Make method wrapper without return value
template <typename T, typename... Args>
struct MethodWrap<void (T::*) (Args...)>
{
	template <void (T::*method) (Args...)>
	static bool wrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
	{
		JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
		if (jsargs.length () < sizeof...(Args)) {
			JS_ReportError (cx, "Too few arguments (%d required)", sizeof...(Args));
			return false;
		}
		typedef typename ArgConvert<void, Args...>::template Wrapper<methodCall<T, void, Args...>::template call<method>> wrapper;
		try {
			wrapper::call (cx, jsargs);
		}
		catch (std::exception &e) {
			JS_ReportError (cx, "C++ exception: %s", e.what ());
			return false;
		}
		jsargs.rval ().setNull ();
		return true;
	}
};

// Make const method wrapper with return value
template <typename T, typename R, typename... Args>
struct MethodWrap<R (T::*) (Args...) const>
{
	template <R (T::*method) (Args...) const>
	static bool wrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
	{
		JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
		if (jsargs.length () < sizeof...(Args)) {
			JS_ReportError (cx, "Too few arguments (%d required)", sizeof...(Args));
			return false;
		}
		typedef typename ArgConvert<R, Args...>::template Wrapper<constMethodCall<T, R, Args...>::template call<method>> wrapper;
		R ret;
		try {
			ret = wrapper::call (cx, jsargs);
		}
		catch (std::exception &e) {
			JS_ReportError (cx, "C++ exception: %s", e.what ());
			return false;
		}
		setJSValue (cx, jsargs.rval (), ret);
		return true;
	}
};

// Make const method wrapper without return value
template <typename T, typename... Args>
struct MethodWrap<void (T::*) (Args...) const>
{
	template <void (T::*method) (Args...) const>
	static bool wrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
	{
		JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
		if (jsargs.length () < sizeof...(Args)) {
			JS_ReportError (cx, "Too few arguments (%d required)", sizeof...(Args));
			return false;
		}
		typedef typename ArgConvert<void, Args...>::template Wrapper<constMethodCall<T, void, Args...>::template call<method>> wrapper;
		try {
			wrapper::call (cx, jsargs);
		}
		catch (std::exception &e) {
			JS_ReportError (cx, "C++ exception: %s", e.what ());
			return false;
		}
		jsargs.rval ().setNull ();
		return true;
	}
};

template <typename T, bool (T::*method) (JSContext *, JS::CallArgs &)>
bool LLMethodWrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
	JSObject *obj = jsargs.thisv ().toObjectOrNull ();
	auto data = static_cast<std::pair<bool, T *> *> (JS_GetPrivate (obj));
	T *ptr = data->second;
	return (ptr->*method) (cx, jsargs);
}

// Make constructor wrapper
template <const JSClass *object_class, typename T, typename... Args>
bool constructorWrapper (JSContext *cx, unsigned int argc, JS::Value *vp)
{
	JS::CallArgs jsargs = JS::CallArgsFromVp (argc, vp);
	if (jsargs.length () < sizeof...(Args)) {
		JS_ReportError (cx, "Too few arguments (%d required)", sizeof...(Args));
		return false;
	}

	JSObject *obj = JS_NewObjectForConstructor (cx, object_class, jsargs);
	if (!obj) {
		return false;
	}

	typedef typename ArgConvert<T *, Args...>::template Wrapper<constructorCall<T, Args...>> wrapper;
	T *ptr;
	try {
		ptr = wrapper::call (cx, jsargs);
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "C++ exception: %s", e.what ());
		return false;
	}
	if (!ptr) {
		JS_ReportOutOfMemory (cx);
		return false;
	}
	auto data = new std::pair<bool, T *> (true, ptr);
	JS_SetPrivate (obj, data);

	jsargs.rval ().setObject (*obj);
	return true;
}

template <typename T>
void finalizeWrapper (JSFreeOp *, JSObject *obj)
{
	auto data = static_cast<std::pair<bool, T *> *> (JS_GetPrivate (obj));
	if (!data)
		return;
	if (data->first)
		delete data->second;
	delete data;
}

template <typename T>
constexpr JSClass make_class (const char *name)
{
	return {
		name, JSCLASS_HAS_PRIVATE,
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, &finalizeWrapper<T>
	};
}

template <auto method>
constexpr JSNativeWrapper make_method_wrapper ()
{
	return { &MethodWrap<decltype(method)>::template wrapper<method>, nullptr };
}

template <auto method>
constexpr JSFunctionSpec make_method (const char *name)
{
	return {
		name,
		make_method_wrapper<method> (),
		0, 0
	};
}

template <auto getter, auto setter>
constexpr JSPropertySpec make_property (const char *name)
{
	return {
		name, JSPROP_ENUMERATE | JSPROP_SHARED,
		{ make_method_wrapper<getter> () },
		{ make_method_wrapper<setter> () },
	};
}

}

#endif
