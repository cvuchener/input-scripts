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

#ifndef MT_QUEUE_H
#define MT_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * Interruptible queue for transfering data across different thread.
 */
template <typename T>
class MTQueue
{
public:
	MTQueue ():
		_interrupted (false)
	{
	}

	~MTQueue ()
	{
	}

	/**
	 * Push an item in the queue.
	 */
	void push (const T &item)
	{
		std::unique_lock<std::mutex> lock (_mutex);
		_queue.push (item);
		lock.unlock ();
		_condvar.notify_one ();
	}

	/**
	 * Pop an item from the queue.
	 *
	 * This method will block until an item
	 * is available unless it is interrupted.
	 *
	 * \see interrupt()
	 */
	std::optional<T> pop ()
	{
		std::unique_lock<std::mutex> lock (_mutex);
		std::optional<T> ret;
		while (_queue.empty () && !_interrupted)
			_condvar.wait (lock);
		if (!_interrupted) {
			ret = std::move (_queue.front ());
			_queue.pop ();
		}
		return ret;
	}

	/**
	 * Try to pop an item from the queue.
	 *
	 * If the queue is empty, an empty optional is returned.
	 */
	std::optional<T> try_pop ()
	{
		std::unique_lock<std::mutex> lock (_mutex);
		std::optional<T> ret;
		if(!_queue.empty ()) {
			ret = std::move (_queue.front ());
			_queue.pop ();
		}
		return ret;
	}

	/**
	 * Make the current and future calls to pop() return
	 * immediately with an empty value.
	 *
	 * \see resetInterruption()
	 */
	void interrupt ()
	{
		_interrupted = true;
		_condvar.notify_all ();
	}

	/**
	 * Cancel the effect of interrupt().
	 *
	 * Future calls to pop(), will block again.
	 */
	void resetInterruption ()
	{
		_interrupted = false;
	}

private:
	std::mutex _mutex;
	std::condition_variable _condvar;
	std::queue<T> _queue;
	bool _interrupted;
};

#endif
