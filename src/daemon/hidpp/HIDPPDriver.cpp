/*
 * Copyright 2017 Clément Vuchener
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

#include "HIDPPDriver.h"

#include "HIDPP20Device.h"

#include <hidpp10/Device.h>
#include <hidpp10/IReceiver.h>
#include <hidpp10/defs.h>

#include "../Log.h"

extern "C" {
#include <libudev.h>
}

using namespace std::literals::chrono_literals;

HIDPPDriver::HIDPPDriver ()
{
}

HIDPPDriver::~HIDPPDriver ()
{
	for (auto &pair: _nodes) {
		pair.second.dispatcher->stop ();
		pair.second.thread.join ();
	}
}

static bool isReceiver (HIDPP::Dispatcher *dispatcher)
{
	try {
		HIDPP10::Device dev (dispatcher);
		HIDPP10::IReceiver (&dev).getDeviceInformation (0, nullptr, nullptr, nullptr, nullptr);
		return true;
	}
	catch (std::exception &e) {
		return false;
	}
}

void HIDPPDriver::addDevice (udev_device *dev)
{
	const char *syspath = udev_device_get_syspath (dev);
	const char *devnode = udev_device_get_devnode (dev);
	decltype (_nodes)::iterator it;
	Node *node = nullptr;

	// Open device
	try {
		auto ret = _nodes.emplace (syspath, Node {});
		if (!ret.second) {
			Log::error () << "HIDPP device " << syspath
				      << " already opened." << std::endl;
			return;
		}
		it = ret.first;
		node = &it->second;
		node->dispatcher = std::make_unique<HIDPP::DispatcherThread> (devnode);
		node->thread = std::thread (&HIDPPDriver::dispatcherRun, this, node);
	}
	catch (std::exception &e) {
		Log::error () << "Failed to open HIDPP device " << syspath
			      << ": " << e.what () << std::endl;
		_nodes.erase (it);
		return;
	}

	if (isReceiver (node->dispatcher.get ())) {
		// Try as a HID++ receiver
		for (int i = 1; i <= 6; ++i) {
			auto index = static_cast<HIDPP::DeviceIndex> (i);
			node->dispatcher->registerEventHandler (
				index, HIDPP10::DeviceDisconnection,
				std::bind (&HIDPPDriver::receiverEvent, this, node, std::placeholders::_1));
			node->dispatcher->registerEventHandler (
				index, HIDPP10::DeviceConnection,
				std::bind (&HIDPPDriver::receiverEvent, this, node, std::placeholders::_1));
			// Try adding wireless devices
			addDevice (node, static_cast<HIDPP::DeviceIndex> (i));
		}
	}
	else {
		// Try both corded device index
		for (auto index: { HIDPP::DefaultDevice, HIDPP::CordedDevice }) {
			addDevice (node, index);
		}
	}
}

void HIDPPDriver::removeDevice (udev_device *dev)
{
	auto it = _nodes.find (udev_device_get_syspath (dev));
	if (it == _nodes.end ()) {
		Log::warning () << "Trying to remove unknown device." << std::endl;
		return;
	}
	it->second.dispatcher->stop ();
	it->second.thread.join ();
	_nodes.erase (it);
}

void HIDPPDriver::addDevice (Node *node, HIDPP::DeviceIndex index)
{
	std::unique_ptr<InputDevice> device;
	try {
		HIDPP::Device dev (node->dispatcher.get (), index);
		unsigned int major, minor;
		std::tie (major, minor) = dev.protocolVersion ();
		if (major >= 2) {
			device = std::make_unique<HIDPP20Device> (std::move (dev));
		}
	}
	catch (std::exception &e) {
		Log::debug () << "Ignoring HID++ device ("
			      << "name = \"" << node->dispatcher->name ()
			      << "\", index = " << static_cast<int> (index)
			      << "): " << e.what () << std::endl;
		return;
	}
	if (device) {
		inputDeviceAdded (device.get ());
		node->devices.emplace (index, std::move (device));
	}
}

void HIDPPDriver::removeDevice (Node *node, HIDPP::DeviceIndex index)
{
	auto it = node->devices.find (index);
	if (it != node->devices.end ()) {
		inputDeviceRemoved (it->second.get ());
		node->devices.erase (it);
	}
}

void HIDPPDriver::dispatcherRun (Node *node)
{
	node->dispatcher->run ();
	for (auto &p: node->devices)
		inputDeviceRemoved (p.second.get ());
	node->devices.clear ();
}

bool HIDPPDriver::receiverEvent (Node *node, const HIDPP::Report &report)
{
	auto params = report.parameterBegin ();
	switch (report.subID ()) {
	case HIDPP10::DeviceConnection:
		if (params[0] & (1<<6))
			removeDevice (node, report.deviceIndex ());
		else {
			std::this_thread::sleep_for (10ms);
			addDevice (node, report.deviceIndex ());
		}
		break;
	case HIDPP10::DeviceDisconnection:
		removeDevice (node, report.deviceIndex ());
		break;
	default:
		break;
	}
	return true;
}

bool HIDPPDriver::_registered = Driver::registerDriver ("hidpp", new HIDPPDriver ());
