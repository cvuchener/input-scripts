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

#include <jsapi.h>
#include <iostream>
#include <csignal>
#include <dbus-c++/dbus.h>

#include "event/EventDriver.h"
#include "steamcontroller/SteamControllerDriver.h"
#include "ScriptManager.h"
#include "Udev.h"
#include "Log.h"
#include "Config.h"

extern "C" {
#include <unistd.h>
#include <getopt.h>
}

static constexpr char usage[] = R"***(Usage: %s [--system|--session] [options]

DBus connection:
    --system	Use system bus connection
    --session	Use session bus connection
Default is system bus for root (uid = 0) and session bus for other users.

Options:
    -c|--config configfile	Set the configuration file (default is "config.json")
    -v|--verbose [level]	Set verbosity level

)***";

static constexpr char ServiceName[] = "com.github.cvuchener.InputScripts";

enum Bus {
	Auto,
	Session,
	System
};
static DBus::Connection getConnection (Bus bus)
{
	switch (bus) {
	case Auto:
		if (getuid () == 0)
			return DBus::Connection::SystemBus ();
		else
			return DBus::Connection::SessionBus ();

	case Session:
		return DBus::Connection::SessionBus ();

	case System:
	default:
		return DBus::Connection::SystemBus ();
	}
}

static std::map<std::string, Driver *> drivers;
static ScriptManager *manager;
static DBus::BusDispatcher dispatcher;

static void sigint (int signal)
{
	dispatcher.leave ();
}

int main (int argc, char *argv[])
{
	enum Options {
		SessionOpt = 256,
		SystemOpt,
		ConfigOpt,
		VerboseOpt,
		HelpOpt
	};
	static const struct option longopts[] = {
		{ "session", no_argument, nullptr, SessionOpt },
		{ "system", no_argument, nullptr, SystemOpt },
		{ "config", required_argument, nullptr, ConfigOpt },
		{ "verbose", optional_argument, nullptr, VerboseOpt },
		{ "help", no_argument, nullptr, HelpOpt },
		{ nullptr, 0, nullptr, 0 }
	};
	Bus bus = Auto;
	std::string config_file ("config.json");
	Log::Level log_level = Log::Warning;

	int opt;
	while (-1 != (opt = getopt_long (argc, argv, "c:v::h", longopts, nullptr))) {
		switch (opt) {
		case SessionOpt:
			bus = Session;
			break;

		case SystemOpt:
			bus = System;
			break;

		case 'c':
		case ConfigOpt:
			config_file = optarg;
			break;

		case 'v':
		case VerboseOpt:
			if (!optarg) {
				log_level = Log::Info;
			}
			else {
				std::string str (optarg);
				if (str == "error")
					log_level = Log::Error;
				else if (str == "warning")
					log_level = Log::Warning;
				else if (str == "info")
					log_level = Log::Info;
				else if (str == "debug")
					log_level = Log::Debug;
				else
					std::cerr << "Unknown verbose level: " << str << std::endl;
			}
			break;

		case 'h':
		case HelpOpt:
			fprintf (stderr, usage, argv[0]);
			return EXIT_SUCCESS;
		}
	}

	Log::setLevel (log_level);
	Config::config.loadConfig (config_file);

	if (!JS_Init ()) {
		Log::debug () << "JS_Init failed" << std::endl;
		return -1;
	}

	DBus::default_dispatcher = &dispatcher;
	DBus::Connection dbus_connection = getConnection (bus);
	dbus_connection.request_name (ServiceName);

	drivers.emplace ("event", new EventDriver ());
	drivers.emplace ("steamcontroller", new SteamControllerDriver ());

	manager = new ScriptManager (dbus_connection, &drivers);

	std::signal (SIGINT, sigint);

	Udev udev (&drivers);
	std::thread udev_thread (&Udev::exec, &udev);

	dispatcher.enter ();
	udev.interrupt ();

	udev_thread.join ();
	delete manager;

	for (auto pair: drivers)
		delete pair.second;

	JS_ShutDown ();
	return 0;
}
