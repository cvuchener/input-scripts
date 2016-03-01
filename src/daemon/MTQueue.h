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

#include <condition_variable>
#include <queue>

#include <iostream>

template <typename Item>
class MTQueue
{
public:
	MTQueue ()
	{
	}

	~MTQueue ()
	{
	}

	void push (Item item)
	{
		std::unique_lock<std::mutex> lock (_mutex);
		_queue.push (item);
		lock.unlock ();
		_condvar.notify_all ();
	}

	Item pop ()
	{
		std::unique_lock<std::mutex> lock (_mutex);
		while (_queue.empty ()) {
			_condvar.wait (lock);
		}
		Item item = _queue.front ();
		_queue.pop ();
		return item;
	}

private:
	std::mutex _mutex;
	std::condition_variable _condvar;
	std::queue<Item> _queue;
};

#endif
