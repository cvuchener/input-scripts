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

#include "WiimoteDevice.h"

#include "../ClassManager.h"

extern "C" {
#include <unistd.h>
#include <fcntl.h>
}

WiimoteDevice::WiimoteDevice (const std::string &path)
{
	int ret;

	if (0 != (ret = xwii_iface_new (&_dev, path.c_str ())))
		throw std::system_error (-ret, std::system_category (), "xwii_iface_new");
	_opened = xwii_iface_opened (_dev);
	_available = xwii_iface_available (_dev);
	if (0 != (ret = xwii_iface_watch (_dev, true))) {
		xwii_iface_unref (_dev);
		throw std::system_error (-ret, std::system_category (), "xwii_iface_watch");
	}
	char *devtype;
	if (0 != (ret = xwii_iface_get_devtype (_dev, &devtype))) {
		xwii_iface_unref (_dev);
		throw std::system_error (-ret, std::system_category (), "xwii_iface_get_devtype");
	}
	_name.assign (devtype);
	free (devtype);
	// kernel driver bug: devtype is "unknown" when it should be "pending"
	if (_name == "unknown") {
		xwii_iface_unref (_dev);
		throw UnknownDeviceError ();
	}
	if (-1 == pipe2 (_pipe, O_CLOEXEC)) {
		xwii_iface_unref (_dev);
		throw std::system_error (errno, std::system_category (), "pipe2");
	}
}

WiimoteDevice::~WiimoteDevice ()
{
	xwii_iface_unref (_dev);
	close (_pipe[0]);
	close (_pipe[1]);
}

void WiimoteDevice::start ()
{
	assert (!_thread.joinable ());
	_thread = std::thread ([this] () {
		try {
			readEvents ();
		}
		catch (std::exception &e) {
			Log::error () << "Wiimote device failed: " << e.what () << std::endl;
			error.emit ();
		}
	});
}

void WiimoteDevice::stop ()
{
	assert (_thread.joinable ());
	char c = 0;
	write (_pipe[1], &c, sizeof (char));
	_thread.join ();
}

void WiimoteDevice::readEvents ()
{
	int ret;

	while (true) {
		fd_set set;
		int xwii_fd = xwii_iface_get_fd (_dev);
		int nfds = std::max (xwii_fd, _pipe[0]) + 1;
		FD_ZERO (&set);
		FD_SET (xwii_fd, &set);
		FD_SET (_pipe[0], &set);

		while (-1 == select (nfds, &set, nullptr, nullptr, nullptr)) {
			if (errno != EINTR)
				throw std::system_error (errno, std::system_category (), "select");
		}

		if (FD_ISSET (xwii_fd, &set)) {
			struct xwii_event ev;
			while (0 == (ret = xwii_iface_dispatch (_dev, &ev, sizeof (struct xwii_event)))) {
				switch (ev.type) {
				case XWII_EVENT_KEY:
					eventRead ({
						{ "type", ev.type },
						{ "code", ev.v.key.code },
						{ "value", ev.v.key.state }
					});
					break;
				case XWII_EVENT_ACCEL:
					eventRead ({
						{ "type", ev.type },
						{ "x", ev.v.abs[0].x },
						{ "y", ev.v.abs[0].y },
						{ "z", ev.v.abs[0].z }
					});
					break;
				case XWII_EVENT_IR:
					for (unsigned int i = 0; i < 4; ++i) {
						if (xwii_event_ir_is_valid (&ev.v.abs[i])) {
							_tracking[i] = true;
							eventRead ({
								{ "type", ev.type },
								{ "id", i },
								{ "tracking", 1 },
								{ "x", ev.v.abs[i].x },
								{ "y", ev.v.abs[i].y }
							});
						}
						else if (_tracking[i]) {
							_tracking[i] = false;
							eventRead ({
								{ "type", ev.type },
								{ "id", i },
								{ "tracking", 0 }
							});
						}
					}
					break;
				case XWII_EVENT_BALANCE_BOARD:
					for (unsigned int i = 0; i < 4; ++i)
						eventRead ({
							{ "type", ev.type },
							{ "id", i },
							{ "w", ev.v.abs[i].x }
						});
					break;
				case XWII_EVENT_MOTION_PLUS:
					eventRead ({
						{ "type", ev.type },
						{ "x", ev.v.abs[0].x },
						{ "y", ev.v.abs[0].y },
						{ "z", ev.v.abs[0].z }
					});
					break;
				case XWII_EVENT_PRO_CONTROLLER_KEY:
				case XWII_EVENT_PRO_CONTROLLER_MOVE:
					Log::warning () << "Event not implemented" << std::endl;
					break;
				case XWII_EVENT_WATCH: {
					unsigned int old_opened = _opened;
					unsigned int old_available = _available;
					_opened = xwii_iface_opened (_dev);
					_available = xwii_iface_available (_dev);
					Log::debug () << std::hex
						      << "Open devices: " << _opened
						      << " (changed: " << (old_opened^_opened)
						      << ")" << std::endl;
					Log::debug () << std::hex
						      << "Available devices: " << _available
						      << " (changed: " << (old_available^_available)
						      << ")" << std::endl;
					eventRead ({
						{ "type", ev.type },
						{ "available", _available },
						{ "changed", old_available^_available }
					});
					break;
				}
				case XWII_EVENT_CLASSIC_CONTROLLER_KEY:
				case XWII_EVENT_CLASSIC_CONTROLLER_MOVE:
				case XWII_EVENT_NUNCHUK_KEY:
				case XWII_EVENT_NUNCHUK_MOVE:
				case XWII_EVENT_DRUMS_KEY:
				case XWII_EVENT_DRUMS_MOVE:
				case XWII_EVENT_GUITAR_KEY:
				case XWII_EVENT_GUITAR_MOVE:
					Log::warning () << "Event not implemented" << std::endl;
					break;
				case XWII_EVENT_GONE:
					error.emit ();
					return;
				default:
					Log::debug () << "Unsupported xwiimote event type: " << ev.type << std::endl;
				}
			}
			if (ret != -EAGAIN)
				throw std::system_error (-ret, std::system_category (), "xwii_iface_dispatch");
		}
		if (FD_ISSET (_pipe[0], &set)) {
			char c;
			read (_pipe[0], &c, sizeof (char));
			return;
		}
	}
}

std::string WiimoteDevice::driver () const
{
	return "wiimote";
}

std::string WiimoteDevice::name () const
{
	return _name;
}

std::string WiimoteDevice::serial () const
{
	return std::string ();
}

InputDevice::Event WiimoteDevice::getEvent (InputDevice::Event event)
{
	return event;
}

void WiimoteDevice::open (unsigned ifaces)
{
	int ret;
	ret = xwii_iface_open (_dev, ifaces);
	_opened = xwii_iface_opened (_dev);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "xwii_iface_open");
}

void WiimoteDevice::close (unsigned ifaces)
{
	xwii_iface_close (_dev, ifaces);
	_opened = xwii_iface_opened (_dev);
}

void WiimoteDevice::rumble (bool on)
{
	int ret;
	ret = xwii_iface_rumble (_dev, on);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "xwii_iface_rumble");
}

bool WiimoteDevice::getLED (unsigned int led)
{
	int ret;
	bool state;
	ret = xwii_iface_get_led (_dev, led, &state);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "xwii_iface_get_led");
	return state;
}

void WiimoteDevice::setLED (unsigned int led, bool state)
{
	int ret;
	ret = xwii_iface_set_led (_dev, led, state);
	if (ret != 0)
		throw std::system_error (-ret , std::system_category (), "xwii_iface_get_led");
}

uint8_t WiimoteDevice::battery ()
{
	int ret;
	uint8_t capacity;
	ret = xwii_iface_get_battery (_dev, &capacity);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "xwii_iface_get_battery");
	return capacity;
}

std::string WiimoteDevice::extension ()
{
	int ret;
	std::string extension;
	char *str;
	ret = xwii_iface_get_extension (_dev, &str);
	if (ret != 0)
		throw std::system_error (-ret, std::system_category (), "xwii_iface_get_extension");
	extension.assign (str);
	free (str);
	return extension;
}

void WiimoteDevice::setMPNormalization (int32_t x, int32_t y, int32_t z, int32_t factor)
{
	 xwii_iface_set_mp_normalization (_dev, x, y, z, factor);
}

std::array<int32_t, 4> WiimoteDevice::getMPNormalization ()
{
	int32_t x, y, z, factor;
	xwii_iface_get_mp_normalization (_dev, &x, &y, &z, &factor);
	return { x, y, z, factor };
}

#define DEFINE_XWII_CONSTANT(name) { #name, XWII_##name }
const std::pair<std::string, int> WiimoteDevice::js_int_const[] = {
	DEFINE_XWII_CONSTANT(EVENT_KEY),
	DEFINE_XWII_CONSTANT(EVENT_ACCEL),
	DEFINE_XWII_CONSTANT(EVENT_IR),
	DEFINE_XWII_CONSTANT(EVENT_BALANCE_BOARD),
	DEFINE_XWII_CONSTANT(EVENT_MOTION_PLUS),
	DEFINE_XWII_CONSTANT(EVENT_PRO_CONTROLLER_KEY),
	DEFINE_XWII_CONSTANT(EVENT_PRO_CONTROLLER_MOVE),
	DEFINE_XWII_CONSTANT(EVENT_WATCH),
	DEFINE_XWII_CONSTANT(EVENT_CLASSIC_CONTROLLER_KEY),
	DEFINE_XWII_CONSTANT(EVENT_CLASSIC_CONTROLLER_MOVE),
	DEFINE_XWII_CONSTANT(EVENT_NUNCHUK_KEY),
	DEFINE_XWII_CONSTANT(EVENT_NUNCHUK_MOVE),
	DEFINE_XWII_CONSTANT(EVENT_DRUMS_KEY),
	DEFINE_XWII_CONSTANT(EVENT_DRUMS_MOVE),
	DEFINE_XWII_CONSTANT(EVENT_GUITAR_KEY),
	DEFINE_XWII_CONSTANT(EVENT_GUITAR_MOVE),
	DEFINE_XWII_CONSTANT(EVENT_GONE),
	DEFINE_XWII_CONSTANT(KEY_LEFT),
	DEFINE_XWII_CONSTANT(KEY_RIGHT),
	DEFINE_XWII_CONSTANT(KEY_UP),
	DEFINE_XWII_CONSTANT(KEY_DOWN),
	DEFINE_XWII_CONSTANT(KEY_A),
	DEFINE_XWII_CONSTANT(KEY_B),
	DEFINE_XWII_CONSTANT(KEY_PLUS),
	DEFINE_XWII_CONSTANT(KEY_MINUS),
	DEFINE_XWII_CONSTANT(KEY_HOME),
	DEFINE_XWII_CONSTANT(KEY_ONE),
	DEFINE_XWII_CONSTANT(KEY_TWO),
	DEFINE_XWII_CONSTANT(KEY_X),
	DEFINE_XWII_CONSTANT(KEY_Y),
	DEFINE_XWII_CONSTANT(KEY_TL),
	DEFINE_XWII_CONSTANT(KEY_TR),
	DEFINE_XWII_CONSTANT(KEY_ZL),
	DEFINE_XWII_CONSTANT(KEY_ZR),
	DEFINE_XWII_CONSTANT(KEY_THUMBL),
	DEFINE_XWII_CONSTANT(KEY_THUMBR),
	DEFINE_XWII_CONSTANT(KEY_C),
	DEFINE_XWII_CONSTANT(KEY_Z),
	DEFINE_XWII_CONSTANT(KEY_STRUM_BAR_UP),
	DEFINE_XWII_CONSTANT(KEY_STRUM_BAR_DOWN),
	DEFINE_XWII_CONSTANT(KEY_FRET_FAR_UP),
	DEFINE_XWII_CONSTANT(KEY_FRET_UP),
	DEFINE_XWII_CONSTANT(KEY_FRET_MID),
	DEFINE_XWII_CONSTANT(KEY_FRET_LOW),
	DEFINE_XWII_CONSTANT(KEY_FRET_FAR_LOW),
	DEFINE_XWII_CONSTANT(IFACE_CORE),
	DEFINE_XWII_CONSTANT(IFACE_ACCEL),
	DEFINE_XWII_CONSTANT(IFACE_IR),
	DEFINE_XWII_CONSTANT(IFACE_MOTION_PLUS),
	DEFINE_XWII_CONSTANT(IFACE_NUNCHUK),
	DEFINE_XWII_CONSTANT(IFACE_CLASSIC_CONTROLLER),
	DEFINE_XWII_CONSTANT(IFACE_BALANCE_BOARD),
	DEFINE_XWII_CONSTANT(IFACE_PRO_CONTROLLER),
	DEFINE_XWII_CONSTANT(IFACE_DRUMS),
	DEFINE_XWII_CONSTANT(IFACE_GUITAR),
	DEFINE_XWII_CONSTANT(IFACE_ALL),
	DEFINE_XWII_CONSTANT(IFACE_WRITABLE),
	DEFINE_XWII_CONSTANT(LED1),
	DEFINE_XWII_CONSTANT(LED2),
	DEFINE_XWII_CONSTANT(LED3),
	DEFINE_XWII_CONSTANT(LED4),
	{ "", 0 }
};

const JSClass WiimoteDevice::js_class = JS_HELPERS_CLASS("WiimoteDevice", WiimoteDevice);

const JSFunctionSpec WiimoteDevice::js_fs[] = {
	JS_HELPERS_METHOD("open", WiimoteDevice::open),
	JS_HELPERS_METHOD("close", WiimoteDevice::close),
	JS_HELPERS_METHOD("rumble", WiimoteDevice::rumble),
	JS_HELPERS_METHOD("getLED", WiimoteDevice::getLED),
	JS_HELPERS_METHOD("setLED", WiimoteDevice::setLED),
	JS_HELPERS_METHOD("battery", WiimoteDevice::battery),
	JS_HELPERS_METHOD("extension", WiimoteDevice::extension),
	JS_HELPERS_METHOD("setMPNormalization", WiimoteDevice::setMPNormalization),
	//JS_HELPERS_METHOD("getMPNormalization", WiimoteDevice::getMPNormalization),
	JS_FS_END
};

JSObject *WiimoteDevice::makeJsObject (const JsHelpers::Thread *thread)
{
	return thread->makeJsObject (this);
}

bool WiimoteDevice::_registered = ClassManager::registerClass<WiimoteDevice::JsClass> ("InputDevice");
