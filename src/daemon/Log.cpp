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

#include "Log.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>

Log::Level Log::_level = Log::Error;
Log Log::_null;
Log Log::_error ("error");
Log Log::_warning ("warning");
Log Log::_info ("info");
Log Log::_debug ("debug");

Log::Log ():
	_buf ("null")
{
}

Log::Log (const std::string &prefix):
	std::ostream (&_buf),
	_buf (prefix)
{
}

void Log::setLevel (Log::Level level)
{
	_level = level;
}

Log::Level Log::level ()
{
	return _level;
}

Log &Log::log (Log::Level level)
{
	if (level <= _level) {
		switch (level) {
		case Error:
			return _error;
		case Warning:
			return _warning;
		case Info:
			return _info;
		case Debug:
			return _debug;
		}
	}
	return _null;
}

void Log::printf (const char *format, ...)
{
	char *str;
	int len;
	if (!*this)
		return;
	va_list args;
	va_start (args, format);
	len = vsnprintf (nullptr, 0, format, args);
	str = new char[len];
	vsnprintf (str, len, format, args);
	*this << std::string (str, len);
	delete str;
	va_end (args);
}

Log::LogBuf::LogBuf (const std::string &prefix):
	_prefix (prefix)
{
}

int Log::LogBuf::sync ()
{
	std::cerr << "[" << _prefix << "] " << str ();
	str ("");
	return 0;
}
