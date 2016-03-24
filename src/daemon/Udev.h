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

#ifndef UDEV_H
#define UDEV_H

#include <map>
#include <string>

class Udev
{
public:
	Udev ();
	~Udev ();

	void interrupt ();
	void exec ();

private:
	enum {
		Running,
		Stopping,
		Stopped,
	} _state;
	int _pipe[2];
};

#endif
