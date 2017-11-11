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

#ifndef WIIMOTE_DEVICE_H
#define WIIMOTE_DEVICE_H

#include "../InputDevice.h"

extern "C" {
#include <xwiimote.h>
}

class WiimoteDevice: public InputDevice
{
public:
	class UnknownDeviceError { };

	WiimoteDevice (const std::string &path);
	virtual ~WiimoteDevice ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;
	int32_t getSimpleEvent (uint16_t type, uint16_t code) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	void open (unsigned ifaces);
	void close (unsigned ifaces);
	void rumble (bool on);
	bool getLED (unsigned int led);
	void setLED (unsigned int led, bool state);
	uint8_t battery ();
	std::string extension ();
	void setMPNormalization (int32_t x, int32_t y, int32_t z, int32_t factor);
	std::array<int32_t, 4> getMPNormalization ();

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::AbstractClass<WiimoteDevice> JsClass;

	JSObject *makeJsObject (const JsHelpers::Thread *thread) override;

private:
	void readEvents ();

	int _pipe[2];
	std::thread _thread;
	struct xwii_iface *_dev;
	bool _tracking[XWII_ABS_NUM];
	unsigned int _opened, _available;
	std::string _name;

	static bool _registered;
};

#endif
