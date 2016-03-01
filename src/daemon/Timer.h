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

#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <thread>
#include <condition_variable>
#include <chrono>

template <class Rep, class Period = std::ratio<1>>
class Timer
{
public:
	Timer (std::chrono::duration<Rep, Period> interval):
		_interval (interval)
	{
	}

	~Timer ()
	{
		stop ();
	}

	void start (std::function<bool (void)> func, bool repeat = false)
	{
		_state = Running;
		_thread = std::thread (&Timer<Rep, Period>::run, this, func, repeat);
	}

	void stop ()
	{
		std::unique_lock<std::mutex> lock (_mutex);
		if (isRunning ()) {
			_state = Stopping;
			lock.unlock ();
			_condvar.notify_one ();
		}
		else
			lock.unlock ();
		_thread.join ();
	}

	bool isRunning () const
	{
		return _state == Running;
	}

private:
	void run (std::function<bool (void)> func, bool repeat)
	{
		std::unique_lock<std::mutex> lock (_mutex);
		while (isRunning ()) {
			if (_condvar.wait_for (lock, _interval,
					   [this] () { return !isRunning (); }))
				break;
			if (func () || !repeat)
				break;
		}
		_state = Stopped;
	}

	std::chrono::duration<Rep, Period> _interval;
	enum State {
		Running,
		Stopping,
		Stopped,
	} _state;
	std::thread _thread;
	std::mutex _mutex;
	std::condition_variable _condvar;
};

#endif

