/*
 * Copyright 2017 Clément Vuchener
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

#ifndef JS_HELPERS_SIGNAL_H
#define JS_HELPERS_SIGNAL_H

#include <sigc++/signal.h>

#include "Types.h"
#include "Class.h"

namespace JsHelpers
{

template<typename T, typename Ret, typename... Args>
constexpr SignalConnector make_signal_connector (sigc::signal<Ret (Args...)> T::*signal)
{
	return [signal] (JSContext *cx, JS::HandleValue obj, JS::HandleValue callback) {
		T *ptr;
		readJSValue (cx, ptr, obj);
		std::function<Ret (Args...)> fn;
		readJSValue (cx, fn, callback);
		return (ptr->*signal).connect (fn);
	};
}

}

#endif
