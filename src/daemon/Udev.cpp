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

#include "Udev.h"

#include "Log.h"
#include "Driver.h"

#include <iostream>

extern "C" {
#include <libudev.h>
#include <unistd.h>
#include <fcntl.h>
}

#define INPUT_SCRIPTS_UDEV_TAG	"input_scripts"
#define INPUT_SCRIPTS_UDEV_DRIVER_PROP	"INPUT_SCRIPTS_DRIVER"

Udev::Udev (const std::map<std::string, Driver *> *drivers):
	_drivers (drivers),
	_state (Stopped)
{
	if (-1 == pipe2 (_pipe, O_CLOEXEC))
		throw std::runtime_error ("pipe error");
}

Udev::~Udev ()
{
}

void Udev::interrupt ()
{
	char c = 0;
	if (_state != Running)
		return;
	_state = Stopping;
	write (_pipe[1], &c, sizeof (char));
}

void Udev::exec () {
	int ret;

	_state = Running;

	struct udev *ctx = udev_new ();
	if (!ctx)
		throw std::runtime_error ("udev_new failed");

	struct udev_monitor *monitor = udev_monitor_new_from_netlink (ctx, "udev");
	if (!monitor)
		throw std::runtime_error ("udev_monutor_new_from_netlink failed");
	ret = udev_monitor_filter_add_match_tag (monitor, INPUT_SCRIPTS_UDEV_TAG);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "udev_monitor_file_add_match_tag");
	ret = udev_monitor_enable_receiving (monitor);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "udev_monitor_enable_receiving");

	struct udev_enumerate *enumerate = udev_enumerate_new (ctx);
	if (!enumerate)
		throw std::runtime_error ("udev_enumerate_new failed");
	ret = udev_enumerate_add_match_tag (enumerate, INPUT_SCRIPTS_UDEV_TAG);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "udev_enumerate_add_match_tag");
	ret = udev_enumerate_scan_devices (enumerate);

	struct udev_list_entry *current;
	udev_list_entry_foreach (current, udev_enumerate_get_list_entry (enumerate)) {
		struct udev_device *device = udev_device_new_from_syspath (ctx, udev_list_entry_get_name (current));
		std::string driver = udev_device_get_property_value (device, INPUT_SCRIPTS_UDEV_DRIVER_PROP);
		Log::info () << "Found device " << udev_device_get_syspath (device) << " with driver " << driver << std::endl;
		if (driver.empty ()) {
			Log::error () << "Missing " INPUT_SCRIPTS_UDEV_DRIVER_PROP " property" << std::endl;
		}
		else {
			auto it = _drivers->find (driver);
			if (it == _drivers->end ())
				Log::error () << "Unknown driver " << driver << std::endl;
			else
				it->second->addDevice (device);
		}
		udev_device_unref (device);
	}
	udev_enumerate_unref (enumerate);

	int fd = udev_monitor_get_fd (monitor);
	while (_state == Running) {
		int nfds = std::max (fd, _pipe[0]) + 1;
		fd_set fds;
		FD_ZERO (&fds);
		FD_SET (fd, &fds);
		FD_SET (_pipe[0], &fds);
		if (-1 == select (nfds, &fds, nullptr, nullptr, nullptr)) {
			if (errno == EINTR)
				continue;
			throw std::system_error (errno, std::system_category (), "select");
		}
		if (FD_ISSET (fd, &fds)) {
			struct udev_device *device = udev_monitor_receive_device (monitor);
			std::string action = udev_device_get_action (device);
			std::string driver = udev_device_get_property_value (device, INPUT_SCRIPTS_UDEV_DRIVER_PROP);
			Log::info () << action << " device " << udev_device_get_syspath (device) << " with driver " << driver << std::endl;
			if (driver.empty ()) {
				Log::error () << "Missing " INPUT_SCRIPTS_UDEV_DRIVER_PROP " property" << std::endl;
			}
			else {
				auto it = _drivers->find (driver);
				if (it == _drivers->end ()) {
					Log::error () << "Unknown driver " << driver << std::endl;
				}
				else {
					if (action == "add")
						it->second->addDevice (device);
					else if (action == "remove")
						it->second->removeDevice (device);
				}
			}
			udev_device_unref (device);
		}
		if (FD_ISSET (_pipe[0], &fds)) {
			char c;
			read (_pipe[0], &c, sizeof (char));
		}
	}

	udev_monitor_unref (monitor);
	udev_unref (ctx);
	_state = Stopped;
}

