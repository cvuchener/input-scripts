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

#include "EventDevice.h"

#include "../ClassManager.h"

#include <iostream>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
}

EventDevice::EventDevice (const std::string &path)
{
	int ret;

	Log::info () << "Opening event node: " << path << std::endl;
	_fd = open (path.c_str (), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (!_fd) {
		throw std::runtime_error ("Cannot open device");
	}
	ret = libevdev_new_from_fd (_fd, &_dev);
	if (ret != 0) {
		close (_fd);
		throw std::runtime_error ("libevdev_new_from_fd failed");
	}
	if (-1 == pipe2 (_pipe, O_CLOEXEC)) {
		libevdev_free (_dev);
		close (_fd);
		throw std::runtime_error ("Failed to create pipe");
	}
}

EventDevice::EventDevice (EventDevice &&other):
	_thread (std::move (other._thread))
{
	_fd = other._fd;
	other._fd = -1;
	for (int i = 0; i < 2; ++i) {
		_pipe[i] = other._pipe[i];
		other._pipe[i] = -1;
	}
	_dev = other._dev;
	other._dev = nullptr;
}

EventDevice::~EventDevice ()
{
	if (_thread.joinable ())
		stop ();
	if (_dev)
		libevdev_free (_dev);
	if (_fd != -1)
		close (_fd);
	for (int i = 0; i < 2; ++i)
		if (_pipe[i] != -1)
			close (_pipe[i]);
}

void EventDevice::start ()
{
	assert (!_thread.joinable ());
	_thread = std::thread ([this] () {
		try {
			readEvents ();
		}
		catch (std::exception &e) {
			Log::error () << "Event device failed: " << e.what () << std::endl;
			error.emit ();
		}
	});
}

void EventDevice::stop ()
{
	assert (_thread.joinable ());
	char c = 0;
	write (_pipe[1], &c, sizeof (char));
	_thread.join ();
}

void EventDevice::readEvents ()
{
	int ret;
	fd_set set;
	int nfds = std::max (_fd, _pipe[0]) + 1;

	do {
		FD_ZERO (&set);
		FD_SET (_fd, &set);
		FD_SET (_pipe[0], &set);

		while (-1 == select (nfds, &set, nullptr, nullptr, nullptr)) {
			if (errno != EINTR)
				throw std::system_error (errno, std::system_category (), "select");
		}

		if (FD_ISSET (_fd, &set)) {
			struct input_event ev;
			do {
				ret = libevdev_next_event (_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

				if (ret == LIBEVDEV_READ_STATUS_SUCCESS) {
					simpleEventRead (ev.type, ev.code, ev.value);
				}
				while (ret == LIBEVDEV_READ_STATUS_SYNC) {
					simpleEventRead (ev.type, ev.code, ev.value);
					ret = libevdev_next_event (_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
				}
			} while (ret >= 0);
		}
		if (FD_ISSET (_pipe[0], &set)) {
			char c;
			read (_pipe[0], &c, sizeof (char));
			return;
		}
	} while (!(ret < 0 && ret != -EAGAIN));
	Log::error () << "libevdev_next_event: " << strerror (-ret) << std::endl;
	error.emit ();
}

std::string EventDevice::driver () const
{
	return "event";
}

std::string EventDevice::name () const
{
	const char *name = libevdev_get_name (_dev);
	if (name)
		return std::string (name);
	else
		return std::string ();
}

std::string EventDevice::serial () const
{
	const char *uniq = libevdev_get_uniq (_dev);
	if (uniq)
		return std::string (uniq);
	else
		return std::string ();
}

void EventDevice::grab (bool grab_mode)
{
	int ret;
	ret = libevdev_grab (_dev, grab_mode ? LIBEVDEV_GRAB : LIBEVDEV_UNGRAB );
	if (ret < 0) {
		throw std::runtime_error ("Cannot set grab mode");
	}
}

InputDevice::Event EventDevice::getEvent (InputDevice::Event event)
{
	event["value"] = getSimpleEvent (event["type"], event["code"]);
	return event;
}

int32_t EventDevice::getSimpleEvent (uint16_t type, uint16_t code)
{
	int32_t value;
	if (!libevdev_fetch_event_value (_dev, type, code, &value)) {
		throw std::invalid_argument ("Event type or code does not exist on this device");
	}
	return value;
}

const JSClass EventDevice::js_class = JS_HELPERS_CLASS("EventDevice", EventDevice);

const JSFunctionSpec EventDevice::js_fs[] = {
	JS_HELPERS_METHOD("grab", EventDevice::grab),
	JS_FS_END
};

JSObject *EventDevice::makeJsObject (const JsHelpers::Thread *thread)
{
	return thread->makeJsObject (this);
}

bool EventDevice::_registered = ClassManager::registerClass<EventDevice::JsClass> ("InputDevice");
