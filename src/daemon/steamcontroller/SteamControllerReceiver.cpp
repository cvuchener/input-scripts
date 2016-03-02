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

#include "SteamControllerReceiver.h"

#include "SteamControllerDevice.h"

#include "../Log.h"

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
}

static const std::array<uint8_t, 33> raw_report_desc = {
	0x06, 0x00, 0xFF,	/* Usage Page (FF00 - Vendor) */
	0x09, 0x01,		/* Usage (0001 - Vendor) */
	0xA1, 0x01,		/* Collection (Application) */
	0x15, 0x00,		/*   Logical Minimum (0) */
	0x26, 0xFF, 0x00,	/*   Logical Maximum (255) */
	0x75, 0x08,		/*   Report Size (8) */
	0x95, 0x40,		/*   Report Count (64) */
	0x09, 0x01,		/*   Usage (0001 - Vendor) */
	0x81, 0x02,		/*   Input (Data, Variable, Absolute) */
	0x95, 0x40,		/*   Report Count (64) */
	0x09, 0x01,		/*   Usage (0001 - Vendor) */
	0x91, 0x02,		/*   Output (Data, Variable, Absolute) */
	0x95, 0x40,		/*   Report Count (64) */
	0x09, 0x01,		/*   Usage (0001 - Vendor) */
	0xB1, 0x02,		/*   Feature (Data, Variable, Absolute) */
	0xC0,			/* End Collection */
};

SteamControllerReceiver::SteamControllerReceiver (const std::string &path)
{
	int ret;
	_fd = open (path.c_str (), O_RDWR);
	if (_fd == -1)
		throw std::system_error (errno, std::system_category (), "open");
	ret = pipe (_pipe);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "pipe");

	struct hidraw_devinfo di;
	ret = ioctl (_fd, HIDIOCGRAWINFO, &di);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCGRAWINFO");

	if (di.vendor != 0x28de)
		throw InvalidDeviceError ();

	struct hidraw_report_descriptor rdesc;
	ret = ioctl (_fd, HIDIOCGRDESCSIZE, &rdesc.size);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCGRDESCSIZE");
	ret = ioctl (_fd, HIDIOCGRDESC, &rdesc);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCGRDESC");
	if (rdesc.size != raw_report_desc.size ())
		throw InvalidDeviceError ();
	if (!std::equal (raw_report_desc.begin (), raw_report_desc.end (), rdesc.value))
		throw InvalidDeviceError ();

	char name[64];
	ret = ioctl (_fd, HIDIOCGRAWNAME(sizeof (name)), name);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCGRAWNAME");
	_name.assign (name, ret);

	switch (di.product) {
	case 0x1102: // Wired controller
		_connected = true;
		_device = new SteamControllerDevice (this);
		break;

	case 0x1142: // Wireless receiver
		_connected = false;
		sendRequest (0xb4, {}, nullptr);
		break;

	default:
		throw InvalidDeviceError ();
	}
}

SteamControllerReceiver::~SteamControllerReceiver ()
{
	if (_connected) {
		_connected = false;
		disconnected ();
		delete _device;
	}
	close (_fd);
	close (_pipe[0]);
	close (_pipe[1]);
}

void SteamControllerReceiver::monitor ()
{
	// Send connected signal for already connected devices
	if (_connected)
		connected ();

	int ret;
	int nfds = std::max (_fd, _pipe[0]) + 1;
	fd_set fds;
	std::array<uint8_t, 64> report;
	try {
		while (true) {
			FD_ZERO (&fds);
			FD_SET (_fd, &fds);
			FD_SET (_pipe[0], &fds);

			ret = select (nfds, &fds, nullptr, nullptr, nullptr);
			if (ret == -1) {
				if (errno == EINTR)
					continue;
				throw std::system_error (errno, std::system_category (), "SteamControllerReceiver monitor select");
			}

			if (FD_ISSET (_fd, &fds)) {
				ret = read (_fd, report.data (), 64);
				if (ret == -1)
					throw std::system_error (errno, std::system_category (), "SteamControllerReceiver monitor read");
				if (ret != 64)
					Log::error () << "Invalid report size: " << ret << std::endl;
				parseReport (report);
			}
			if (FD_ISSET (_pipe[0], &fds)) {
				char c;
				read (_pipe[0], &c, sizeof (char));
				break;
			}
		}
	}
	catch (std::exception &e) {
		Log::error () << "Steam Controller monitoring failed: " << e.what () << std::endl;
	}
}


void SteamControllerReceiver::parseReport (const std::array<uint8_t, 64> &report)
{
	uint8_t type = report[2];
	uint8_t length = report[3];

	switch (type) {
	case 0x03:
		if (length != 1) {
			Log::error () << "Steam Controller event with invalid length" << std::endl;
			break;
		}

		switch (report[4]) {
		case 0x01: // Disconnected event
			if (_connected) {
				_connected = false;
				disconnected ();
				delete _device;
				_device = nullptr;
			}
			break;

		case 0x02: // Connected event
			if (!_connected) {
				_connected = true;
				_device = new SteamControllerDevice (this);
				connected ();
			}
			break;

		case 0x03: // Paired
			Log::debug () << "Steam Controller Paired event" << std::endl;
			break;

		default:
			Log::warning ().printf ("Unknown Steam Controller connection event: %02hhx\n", report[4]);
		}
		break;

	case 0x01:
		if (_connected) {
			inputReport (report);
		}
		break;
	}
}

void SteamControllerReceiver::stop ()
{
	char c = 0;
	write (_pipe[1], &c, sizeof (char));
}

std::string SteamControllerReceiver::name () const
{
	return _name;
}

bool SteamControllerReceiver::isConnected () const
{
	return _connected;
}

SteamControllerDevice *SteamControllerReceiver::device ()
{
	return _device;
}

void SteamControllerReceiver::sendRequest (uint8_t id, const std::vector<uint8_t> &params, std::vector<uint8_t> *result)
{
	std::unique_lock<std::mutex> (_request_mutex);
	int ret;
	std::array<uint8_t, 65> report = { 0 };

	if (params.size () > 62)
		throw std::invalid_argument ("Steam controller request parameters too long");
	report[0] = 0;
	report[1] = id;
	report[2] = params.size ();
	std::copy (params.begin (), params.end (), &report[3]);
	ret = ioctl (_fd, HIDIOCSFEATURE(report.size ()), report.data ());
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCSFEATURE");

	if (!result)
		return;

	report[0] = 0;
	ret = ioctl (_fd, HIDIOCGFEATURE(report.size ()), report.data ());
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "ioctl HIDIOCGFEATURE");

	if (report[1] != id)
		throw std::runtime_error ("Invalid request id in Steam Controller answer");
	result->assign (&report[3], &report[3] + report[2]);
}
