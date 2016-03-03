Input Scripts
=============

`input-scripts` is a daemon for managing JavaScript scripts for processing input events from devices. Different drivers may be used, providing specific APIs depending on the device. New scripts can be loaded at runtime in order to change the device behavior.


Compilation
-----------

Compilation requires CMake and g++, and has the following dependencies:
 - libevdev
 - mozjs-38 (SpiderMonkey)
 - libudev
 - dbus-c++
 - jsoncpp

Run:

```
mkdir build
cd build
cmake ..
make
```


Configuration
-------------

Configuration files can be passed with absolute or relative paths. Relative paths are searched in `$XDG_CONFIG_HOME/input-scripts` (or `$HOME/.config/input-scripts`) and then in `/etc/input-scripts/`.

`input-scripts` configuration is a JSON file with two properties:
 - `library_scripts` is a dictionary of helpers scripts that will always be loaded.
 - `default_scripts` is an array of rules for loading the default script when a new device is added. Each rule must have a `file` property with the script file name and optional matching rules: `driver`, `name`, `serial`.

Here is an example configuration file loading three helper scripts and setting `scripts/sc-default.js` as the default script for Steam Controller ans `scripts/catch-all.js` for all others.

```
{
	"library_scripts": {
		"Remapper": "scripts/remapper.js",
		"ButtonMap": "scripts/button-map.js",
		"SteamControllerFF": "scripts/sc-ff.js"
	},
	"default_scripts": [
		{
			"driver": "steamcontroller",
			"file": "scripts/sc-default.js"
		},
		{
			"file": "scripts/catch-all.js"
		}
	]
}
```

See `config-example` for more configuration and scripts examples.


### Udev

`input-scripts` relies on udev tags and properties to recognize devices. Add the `input_scripts` tag and set the `INPUT_SCRIPTS_DRIVER` property in udev rules for each device.

For example, for adding WiiU Pro Controllers (as an event device) and Steam Controllers:

```
# WiiU Pro Controller
KERNEL=="event*", SUBSYSTEM=="input", ATTRS{name}=="Nintendo Wii Remote Pro Controller", TAG+="input_scripts", ENV{INPUT_SCRIPTS_DRIVER}="event"

# Valve Steam Controller
ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1102", GOTO="valve_sc"
ATTRS{idVendor}=="28de", ATTRS{idProduct}=="1142", GOTO="valve_sc"
GOTO="valve_sc_end"

LABEL="valve_sc"
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", TAG+="input_scripts", ENV{INPUT_SCRIPTS_DRIVER}="steamcontroller", MODE="0666"

LABEL="valve_sc_end"
```

Note that the Steam Controller need read/write access to the hidraw nodes.


### Scripts

Every script must contains at least three methods:
 - `init ()`: called when the script is loaded
 - `event (type, code, value)`: called for each input event
 - `finalize ()`: called when the script is unloaded

The scope object from the script is used as a prototype to create a script object (the functions are called as this object's methods).

Some global variables are created:
 - `input`: this is the input device object, its class may vary depending on the driver.
 - `system`: provides some useful functions for interacting with the system.

See `config-example/scripts` for script examples.


Usage
-----

### input-scripts

`input-scripts` is the daemon that will manage the scripts. It will load the configuration in `config.json` by default. The daemon exposes DBus interfaces for controlling the device scripts.

Options are:
 - `--session`: use the DBus session bus (default for non-root users).
 - `--system`: use the DBus system bus (default for root).
 - `-c configfile` or `--config configfile`: load `configfile` instead of `config.json`
 - `-v [level]` or `--verbose [level]`: print more message during execution. `level` can be `error`, `warning`, `info`, `debug`. Default value is `warning` without this option, or `info` with this option but no specified level.


### input-scripts-remote

`input-scripts-remote` interacts with the daemon using DBus calls.

Options `--session` or `--system` may be used to choose the DBus bus to use.

Devices can be chosen by DBus paths by using one or more `--path /object/path` options. If no `--path` options are given, all devices that match the optional options `--driver`, `--name`, `--serial`, `-d`, `-n`, `-s` will be used.

Commands are:
 - `list`: print path and informations about all matched devices.
 - `set-file filename`: set the current script of all matched devices to `filename`.

Examples:
 - List all devices: `input-scripts-remote list`
 - Set every Steam Controller in xpad emulation mode: `input-scripts-remote --driver=steamcontroller set-file scripts/sc-x360.js`
 - Set a specific Steam Controller (with a known serial number) in xpad emulation mode: `input-scripts-remote --driver=steamcontroller --serial=1234567890 set-file scripts/sc-x360.js`


Drivers
-------

Currently supported devices are:
 - Generic Linux event devices
 - Valve's Steam Controller


### Event devices

This driver name is `event`.

TODO: API documentation (see src/daemon/event/EventDevice.h).


### Steam controllers

This driver name is `steamcontroller`.

TODO: API documentation (see src/daemon/steamcontroller/SteamControllerDevice.h).


Known issues
------------

 - uinput force feedback is not very robust and it may cause Kernel Oops and unkillable processes. Removing a uinput device when force feedback is in use may cause this.
 - dbus-c++ need the fix from [Steffen Kieß's fix-copy-containers branch](https://github.com/steffen-kiess/dbus-cplusplus/tree/fix-copy-containers) or it may crash when calling org.freedesktop.DBus.ObjectManager.GetManagedObjects.

