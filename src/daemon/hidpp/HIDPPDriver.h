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

#ifndef HIDPP_DRIVER_H
#define HIDPP_DRIVER_H

#include "../Driver.h"

#include <map>
#include <thread>

#include <hidpp/DispatcherThread.h>

class HIDPPDriver: public Driver
{
public:
	HIDPPDriver ();
	virtual ~HIDPPDriver ();

	virtual void addDevice (udev_device *);
	virtual void removeDevice (udev_device *);

private:
	struct Node
	{
		std::unique_ptr<HIDPP::DispatcherThread> dispatcher;
		std::map<HIDPP::DeviceIndex, std::unique_ptr<InputDevice>> devices;
		std::thread thread;
	};
	std::map<std::string, Node> _nodes;
	void addDevice (Node *node, HIDPP::DeviceIndex index, const std::vector<std::string> &paths = std::vector<std::string> ());
	void removeDevice (Node *node, HIDPP::DeviceIndex index);
	void dispatcherRun (Node *node);
	bool receiverEvent (Node *node, const HIDPP::Report &report);

	static bool _registered;
};

#endif
