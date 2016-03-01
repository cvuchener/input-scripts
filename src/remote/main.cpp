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

#include "ObjectManager.h"
#include "Script.h"

#include <iostream>
#include <cstdio>
#include <cstring>

extern "C" {
#include <unistd.h>
#include <getopt.h>
}

static constexpr char usage[] = R"***(Usage: %s [--system|--session] [device options] command [args...]

DBus connection:
    --system	Use system bus connection
    --session	Use session bus connection
Default is system bus for root (uid = 0) and session bus for other users.

Device options:
Select devices by their DBus path (several paths can be passed):
    -p|--path objectpath	Add objectpath
If no path is given, select device by driver/name/serial (missing option matches all devices):
    -d|--driver drivername	Match device driver
    -n|--name devicename	Match device name
    -s|--serial deviceserial	Match device serial

Commands:
list:
    Print every matching device object path and properties.
set-file filename:
    Set the script file for every matching device.

)***";

static constexpr char ServiceName[] = "com.github.cvuchener.InputScripts";
static constexpr char ScriptManagerPath[] = "/com/github/cvuchener/InputScripts/ScriptManager";
static constexpr char ScriptInterface[] = "com.github.cvuchener.InputScripts.Script";

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

using com::github::cvuchener::InputScripts::Script_proxy;

int main (int argc, char *argv[])
{
	enum Options {
		SessionOpt = 256,
		SystemOpt,
		PathOpt,
		DriverOpt,
		NameOpt,
		SerialOpt,
		HelpOpt
	};
	static const struct option longopts[] = {
		{ "session", no_argument, nullptr, SessionOpt },
		{ "system", no_argument, nullptr, SystemOpt },
		{ "path", required_argument, nullptr, PathOpt },
		{ "driver", required_argument, nullptr, DriverOpt },
		{ "name", required_argument, nullptr, NameOpt },
		{ "serial", required_argument, nullptr, SerialOpt },
		{ "help", no_argument, nullptr, HelpOpt },
		{ nullptr, 0, nullptr, 0 }
	};
	Bus bus = Auto;
	std::vector<DBus::Path> paths;
	std::string driver, name, serial;

	int opt;
	while (-1 != (opt = getopt_long (argc, argv, "p:d:n:s:h", longopts, nullptr))) {
		switch (opt) {
		case SessionOpt:
			bus = Session;
			break;

		case SystemOpt:
			bus = System;
			break;

		case 'p':
		case PathOpt:
			paths.push_back (optarg);
			break;

		case 'd':
		case DriverOpt:
			driver.assign (optarg);
			break;

		case 'n':
		case NameOpt:
			name.assign (optarg);
			break;

		case 's':
		case SerialOpt:
			serial.assign (optarg);
			break;

		case 'h':
		case HelpOpt:
			fprintf (stderr, usage, argv[0]);
			return EXIT_SUCCESS;
		}
	}

	if (optind >= argc) {
		fprintf (stderr, "Missing command\n");
		fprintf (stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}
	std::string command = argv[optind];

	DBus::BusDispatcher dispatcher;
	DBus::default_dispatcher = &dispatcher;
	DBus::Connection connection = getConnection (bus);
	if (paths.empty ()) {
		ObjectManager object_manager (connection, ScriptManagerPath, ServiceName);
		auto scripts = object_manager.GetManagedObjects ();
		for (const auto &pair: scripts) {
			const std::string &path = pair.first;
			auto interface = pair.second.find (ScriptInterface);
			if (interface == pair.second.end ()) {
				std::cerr << path << ": Missing Script interface" << std::endl;
				continue;
			}
			bool match = true;
			for (const auto &property: interface->second) {
				std::string propname = property.first;
				DBus::MessageIter reader = property.second.reader ().recurse ();
				if (strcmp ("s", reader.signature ()) != 0) {
					std::cerr << path << ": Property " << propname << " is not a string" << std::endl;
					continue;
				}
				std::string value = reader.get_string ();
				if (propname == "driver") {
					if (!driver.empty () && driver != value) {
						match = false;
						break;
					}
				}
				else if (propname == "name") {
					if (!name.empty () && name != value) {
						match = false;
						break;
					}
				}
				else if (propname == "serial") {
					if (!serial.empty () && serial != value) {
						match = false;
						break;
					}
				}
			}
			if (match)
				paths.push_back (path);
		}
	}

	if (command == "list") {
		for (const auto &path: paths) {
			Script script (connection, path.c_str (), ServiceName);
			std::cout << path << std::endl;
			std::cout << "Driver = " << script.Script_proxy::driver () << std::endl;
			std::cout << "Name = " << script.Script_proxy::name () << std::endl;
			std::cout << "Serial = " << script.Script_proxy::serial () << std::endl;
			std::cout << "File = " << script.Script_proxy::file () << std::endl;
		}
	}
	else if (command == "set-file") {
		if (optind+1 >= argc) {
			std::cerr << "Missing file name" << std::endl;
			return EXIT_FAILURE;
		}
		std::string file = argv[optind+1];
		for (const auto &path: paths) {
			Script script (connection, path.c_str (), ServiceName);
			script.Script_proxy::file (file);
		}
	}
	else {
		std::cerr << "Unknown command" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
