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

#ifndef STEAM_CONTROLLER_RECEIVER_H
#define STEAM_CONTROLLER_RECEIVER_H

#include <functional>
#include <array>
#include <vector>
#include <mutex>
#include <cstdint>

class SteamControllerDevice;

class SteamControllerReceiver
{
public:
	class InvalidDeviceError { };

	SteamControllerReceiver (const std::string &path);
	~SteamControllerReceiver ();

	void monitor ();
	void stop ();

	std::string name () const;

	bool isConnected () const;
	SteamControllerDevice *device ();

	void sendRequest (uint8_t id, const std::vector<uint8_t> &params, std::vector<uint8_t> *result = nullptr);

	std::function<void (void)> connected;
	std::function<void (void)> disconnected;
	std::function<void (const std::array<uint8_t, 64> &)> inputReport;

private:
	void parseReport (const std::array<uint8_t, 64> &report);

	int _fd, _pipe[2];
	std::string _name;
	bool _connected;
	std::mutex _request_mutex;
	SteamControllerDevice *_device;
};


#endif
