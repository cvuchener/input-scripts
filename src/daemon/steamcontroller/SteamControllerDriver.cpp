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

#include "SteamControllerDriver.h"

#include "SteamControllerReceiver.h"
#include "SteamControllerDevice.h"

#include "../Log.h"

extern "C" {
#include <libudev.h>
}

SteamControllerDriver::SteamControllerDriver ()
{
}

SteamControllerDriver::~SteamControllerDriver ()
{
	for (auto &pair: _receivers) {
		pair.second.first->disconnected.clear ();
		pair.second.first->stop ();
		pair.second.second.join ();
		delete pair.second.first;
	}
}

void SteamControllerDriver::addDevice (udev_device *dev)
{
	SteamControllerReceiver *receiver;
	try {
		receiver = new SteamControllerReceiver (udev_device_get_devnode (dev));
	}
	catch (SteamControllerReceiver::InvalidDeviceError) {
		Log::info () << "Ignoring invalid Steam Controller device "
			     << udev_device_get_syspath (dev)
			     << std::endl;
		return;
	}
	receiver->connected.connect ([this, receiver] () {
		inputDeviceAdded (receiver->device ());
	});
	receiver->disconnected.connect ([this, receiver] () {
		inputDeviceRemoved (receiver->device ());
	});
	_receivers.emplace (
		udev_device_get_syspath (dev),
		std::pair<SteamControllerReceiver *, std::thread> (
			receiver,
			std::thread (&SteamControllerReceiver::monitor, receiver)
		)
	);
}

void SteamControllerDriver::removeDevice (udev_device *dev)
{
	auto it = _receivers.find (udev_device_get_syspath (dev));
	if (it == _receivers.end ()) {
		Log::warning () << "Trying to remove unknown device." << std::endl;
		return;
	}
	it->second.first->stop ();
	it->second.second.join ();
	delete it->second.first;
	_receivers.erase (it);
}

bool SteamControllerDriver::_registered = Driver::registerDriver ("steamcontroller", new SteamControllerDriver ());
