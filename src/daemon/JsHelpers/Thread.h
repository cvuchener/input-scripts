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

#ifndef JS_HELPERS_THREAD_H
#define JS_HELPERS_THREAD_H

#include <jsapi.h>
#include <functional>
#include <thread>
#include <future>
#include "../MTQueue.h"

namespace JsHelpers
{

class Thread
{
public:
	virtual ~Thread ();

	void start ();
	void stop ();

	JSContext *getContext ();

	template <typename R>
	R execOnJsThreadSync (std::function<R ()> f)
	{
		std::promise<R> promise;
		std::future<R> future = promise.get_future ();
		execOnJsThreadAsync ([&promise, &f] () {
			promise.set_value (f ());
		});
		return future.get ();
	}

	void execOnJsThreadAsync (std::function<void (void)> f);

protected:
	void exec ();
	virtual void run (JSContext *cx) = 0;

	bool _stopping;

private:
	void run ();

	JSContext *_cx;
	MTQueue<std::function<void (void)>> _task_queue;
	std::thread _thread;
};

}

#endif

