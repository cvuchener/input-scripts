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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "jstpl/jstpl.h"
#include "Timer.h"
#include <list>

namespace jstpl {
class Thread;
}

class System
{
public:
	System (jstpl::Thread *js_thread);
	~System ();

	void setTimeout (std::function<void ()> callback, unsigned int delay);
	void exec (std::string filename, std::vector<std::string> args);
	bool print (JSContext *cx, JS::CallArgs &args);
	bool print_r (JSContext *cx, JS::CallArgs &args);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];

	typedef jstpl::AbstractClass<System> JsClass;

private:
	jstpl::Thread *_js_thread;
	std::mutex _timers_mutex;
	std::list<Timer<int, std::milli>> _timers;
};

#endif
