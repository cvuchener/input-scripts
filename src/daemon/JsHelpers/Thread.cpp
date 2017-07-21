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

#include "Thread.h"

#include "../Log.h"

using JsHelpers::Thread;

Thread::~Thread ()
{
	stop ();
}

void Thread::start ()
{
	_stopping = false;
	_thread = std::thread (static_cast<void (Thread::*) ()> (&Thread::run), this);
}

void Thread::stop ()
{
	if (_thread.joinable ()) {
		_stopping = true;
		_task_queue.push ([] () {});
		_thread.join ();
	}
}

JSContext *Thread::getContext ()
{
	return _cx;
}

void Thread::execOnJsThreadAsync (std::function<void (void)> f)
{
	_task_queue.push (f);
}

void Thread::exec ()
{
	while (!_stopping) {
		auto opt = _task_queue.pop ();
		if (opt)
			opt.value () ();
	}
}

static void errorReporter (JSContext *cx, const char *message, JSErrorReport *report)
{
	Log::error () << (report->filename ? report->filename : "-" ) << ":"
		      << report->lineno << ": "
		      << message << std::endl;
}

void Thread::run ()
{
	JSRuntime *rt = JS_NewRuntime (RuntimeMaxBytes, JS::DefaultNurseryBytes, _main_rt);
	if (!rt) {
		throw std::runtime_error ("JS_NewRuntime failed");
	}

	JSContext *cx = JS_NewContext (rt, 8192);
	if (!cx) {
		throw std::runtime_error ("JS_NewContext failed");
	}
	JS_SetContextPrivate (cx, this);

	JS_SetErrorReporter (rt, errorReporter);

	try {
		run (cx);
	}
	catch (std::exception &e) {
		Log::error () << "Script failed: " << e.what () << std::endl;
	}

	JS_DestroyContext (cx);
	JS_DestroyRuntime (rt);
}

void Thread::init ()
{
	if (!JS_Init ())
		throw std::runtime_error ("JS_Init failed");

	if (!(_main_rt = JS_NewRuntime (RuntimeMaxBytes)))
		throw std::runtime_error("Main JS_NewRuntime failed");

	if (!(_main_cx = JS_NewContext (_main_rt, 8192)))
		throw std::runtime_error ("Main JS_NewContext failed");
}

void Thread::shutdown ()
{
	JS_DestroyContext (_main_cx);
	JS_DestroyRuntime (_main_rt);
	JS_ShutDown ();
}

JSRuntime *Thread::_main_rt = nullptr;
JSContext *Thread::_main_cx = nullptr;
