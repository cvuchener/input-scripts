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

#include "UInput.h"

#include "../Log.h"
#include "../ClassManager.h"

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
}

UInput::UInput ():
	_use_ff (false)
{
	int ret;
	memset (&_uidev, 0, sizeof (struct uinput_user_dev));
	snprintf (_uidev.name, UINPUT_MAX_NAME_SIZE,
	          "Input script device");
	_uidev.id.bustype = BUS_VIRTUAL;

	_fd = open ("/dev/uinput", O_RDWR | O_CLOEXEC);
	if (_fd == -1)
		throw std::system_error (errno, std::system_category (), "open");
	ret = pipe2 (_pipe, O_CLOEXEC);
	if (ret == -1)
		throw std::system_error (errno, std::system_category (), "pipe");
}

UInput::~UInput ()
{
	destroy ();
	close (_fd);
	close (_pipe[0]);
	close (_pipe[1]);
}

std::string UInput::name () const
{
	return _uidev.name;
}

void UInput::setName (const std::string &name)
{
	strncpy (_uidev.name, name.c_str (), UINPUT_MAX_NAME_SIZE);
}

uint16_t UInput::busType () const
{
	return _uidev.id.bustype;
}

void UInput::setBusType (uint16_t bustype)
{
	_uidev.id.bustype = bustype;
}

uint16_t UInput::vendor () const
{
	return _uidev.id.vendor;
}

void UInput::setVendor (uint16_t vendor)
{
	_uidev.id.vendor = vendor;
}

uint16_t UInput::product () const
{
	return _uidev.id.product;
}

void UInput::setProduct (uint16_t product)
{
	_uidev.id.product = product;
}

uint16_t UInput::version () const
{
	return _uidev.id.product;
}

void UInput::setVersion (uint16_t version)
{
	_uidev.id.version = version;
}

void UInput::setKey (uint16_t code)
{
	if (-1 == ioctl (_fd, UI_SET_EVBIT, EV_KEY))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_EVBIT");
	if (-1 == ioctl (_fd, UI_SET_KEYBIT, code))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_KEYBIT");
}

void UInput::setAbs (uint16_t code, int32_t min, int32_t max, int32_t fuzz, int32_t flat)
{
	if (code > ABS_MAX)
		throw std::invalid_argument ("ABS code is hight than ABS_MAX");
	if (-1 == ioctl (_fd, UI_SET_EVBIT, EV_ABS))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_EVBIT");
	if (-1 == ioctl (_fd, UI_SET_ABSBIT, code))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_ABSBIT");
	_uidev.absmax[code] = max;
	_uidev.absmin[code] = min;
	_uidev.absfuzz[code] = fuzz;
	_uidev.absflat[code] = flat;
}

void UInput::setRel (uint16_t code)
{
	if (-1 == ioctl (_fd, UI_SET_EVBIT, EV_REL))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_EVBIT");
	if (-1 == ioctl (_fd, UI_SET_RELBIT, code))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_RELBIT");
}

void UInput::setFF (uint16_t code)
{
	if (-1 == ioctl (_fd, UI_SET_EVBIT, EV_FF))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_EVBIT");
	if (-1 == ioctl (_fd, UI_SET_FFBIT, code))
		throw std::system_error (errno, std::system_category (), "ioctl UI_SET_FFBIT");
	_use_ff = true;
}

void UInput::setFFEffectsMax (uint32_t effects_max)
{
	_uidev.ff_effects_max = effects_max;
}

void UInput::create ()
{
	if (_use_ff && _uidev.ff_effects_max < 1) {
		Log::warning () << "uinput ff_effects_max too low, using 1" << std::endl;
		_uidev.ff_effects_max = 1;
	}
	if (-1 == write (_fd, &_uidev, sizeof (struct uinput_user_dev)))
		throw std::system_error (errno, std::system_category (), "write");
	if (-1 == ioctl (_fd, UI_DEV_CREATE))
		throw std::system_error (errno, std::system_category (), "ioctl UI_DEV_CREATE");
	_thread = std::thread (&UInput::readEvents, this);
}

void UInput::destroy ()
{
	if (-1 == ioctl (_fd, UI_DEV_DESTROY))
		throw std::system_error (errno, std::system_category (), "ioctl UI_DEV_DESTROY");
	if (_thread.joinable ()) {
		char c = 0;
		write (_pipe[1], &c, sizeof (char));
		_thread.join ();
	}
}

void UInput::sendKey (uint16_t code, int32_t value)
{
	sendEvent (EV_KEY, code, value);
}

void UInput::sendAbs (uint16_t code, int32_t value)
{
	sendEvent (EV_ABS, code, value);
}

void UInput::sendRel (uint16_t code, int32_t value)
{
	sendEvent (EV_REL, code, value);
}

void UInput::sendSyn (uint16_t code)
{
	sendEvent (EV_SYN, code, 0);
}

void UInput::sendEvent (uint16_t type, uint16_t code, int32_t value)
{
	struct input_event ev;
	memset (&ev, 0, sizeof (struct input_event));
	ev.type = type;
	ev.code = code;
	ev.value = value;
	if (-1 == write (_fd, &ev, sizeof (struct input_event)))
		throw std::system_error (errno, std::system_category (), "write");
}

void UInput::setFFUploadEffect (std::function<void (int, std::map<std::string, int>)> ff_upload_effect)
{
	_ff_upload_effect = ff_upload_effect;
}

void UInput::setFFEraseEffect (std::function<void (int)> ff_erase_effect)
{
	_ff_erase_effect = ff_erase_effect;
}

void UInput::setFFStart (std::function<void (int)> ff_start)
{
	_ff_start = ff_start;
}

void UInput::setFFStop (std::function<void (int)> ff_stop)
{
	_ff_stop = ff_stop;
}

void UInput::setFFSetGain (std::function<void (int32_t)> ff_set_gain)
{
	_ff_set_gain = ff_set_gain;
}

static void effectEnvelopeToMap (std::map<std::string, int> &properties, const struct ff_envelope *envelope)
{
	properties["attack_length"] = envelope->attack_length;
	properties["attack_level"] = envelope->attack_level;
	properties["fade_length"] = envelope->fade_length;
	properties["fade_level"] = envelope->fade_level;
}

static void effectToMap (std::map<std::string, int> &properties, const struct ff_effect *effect)
{
	properties.clear ();
	properties["type"] = effect->type;
	properties["id"] = effect->id;
	properties["direction"] = effect->direction;
	properties["trigger_button"] = effect->trigger.button;
	properties["trigger_interval"] = effect->trigger.interval;
	properties["replay_length"] = effect->replay.length;
	properties["replay_delay"] = effect->replay.delay;
	switch (effect->type) {
	case FF_CONSTANT:
		properties["level"] = effect->u.constant.level;
		effectEnvelopeToMap (properties, &effect->u.constant.envelope);
		break;
	case FF_RAMP:
		properties["start_level"] = effect->u.ramp.start_level;
		properties["end_level"] = effect->u.ramp.end_level;
		effectEnvelopeToMap (properties, &effect->u.constant.envelope);
		break;
	case FF_PERIODIC:
		properties["waveform"] = effect->u.periodic.waveform;
		properties["period"] = effect->u.periodic.period;
		properties["magnitude"] = effect->u.periodic.magnitude;
		properties["offset"] = effect->u.periodic.offset;
		properties["phase"] = effect->u.periodic.phase;
		effectEnvelopeToMap (properties, &effect->u.constant.envelope);
		break;
	case FF_RUMBLE:
		properties["strong_magnitude"] = effect->u.rumble.strong_magnitude;
		properties["weak_magnitude"] = effect->u.rumble.weak_magnitude;
		break;
	default:
		Log::warning () << "Effect type " << effect->type << " not supported" << std::endl;
	};
}

void UInput::readEvents ()
{
	std::map<std::string, int> properties;
	int ret;
	int nfds = std::max (_fd, _pipe[0]) + 1;
	fd_set fds;
	Log::debug () << "UInput thread" << std::endl;
	while (true) {
		FD_ZERO (&fds);
		FD_SET (_fd, &fds);
		FD_SET (_pipe[0], &fds);
		ret = select (nfds, &fds, nullptr, nullptr, nullptr);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			throw std::system_error (errno, std::system_category (), "UInput thread select");
		}
		if (FD_ISSET (_fd, &fds)) {
			struct input_event ev;
			ret = read (_fd, &ev, sizeof (struct input_event));
			if (ret == -1)
				throw std::system_error (errno, std::system_category (), "UInput thread read");
			switch (ev.type) {
			case EV_LED:
				Log::warning () << "LED event ignored: " << ev.code << ", " << ev.value << std::endl;
				break;

			case EV_FF:
				switch (ev.code) {
				case FF_GAIN:
					Log::debug () << "FF: set gain " << ev.value << std::endl;
					_ff_set_gain (ev.value);
					break;

				default:
					Log::debug () << "FF: event " << ev.code << ", " << ev.value << std::endl;
					if (ev.value)
						_ff_start (ev.code);
					else
						_ff_stop (ev.code);
				}
				break;

			case EV_UINPUT:
				switch (ev.code) {
				case UI_FF_UPLOAD: {
					struct uinput_ff_upload upload;
					memset (&upload, 0, sizeof (struct uinput_ff_upload));
					upload.request_id = ev.value;
					ret = ioctl (_fd, UI_BEGIN_FF_UPLOAD, &upload);
					if (ret == -1)
						throw std::system_error (errno, std::system_category (), "ioctl UI_BEGIN_FF_UPLOAD");
					Log::debug () << "Upload effect " << upload.effect.id << ", type: " << upload.effect.type << std::endl;
					effectToMap (properties, &upload.effect);
					_ff_upload_effect (upload.effect.id, properties);
					upload.retval = 0;
					ret = ioctl (_fd, UI_END_FF_UPLOAD, &upload);
					if (ret == -1)
						throw std::system_error (errno, std::system_category (), "ioctl UI_END_FF_UPLOAD");
					break;
				}
				case UI_FF_ERASE: {
				}
					struct uinput_ff_erase erase;
					memset (&erase, 0, sizeof (struct uinput_ff_erase));
					erase.request_id = ev.value;
					ret = ioctl (_fd, UI_BEGIN_FF_ERASE, &erase);
					if (ret == -1)
						throw std::system_error (errno, std::system_category (), "ioctl UI_BEGIN_FF_ERASE");
					_ff_erase_effect (erase.effect_id);
					erase.retval = 0;
					ret = ioctl (_fd, UI_END_FF_ERASE, &erase);
					if (ret == -1)
						throw std::system_error (errno, std::system_category (), "ioctl UI_END_FF_ERASE");
					break;
				}
				break;

			default:
				Log::warning () << "uinput: event type " << ev.type << " not implemented " << std::endl;
			}
		}
		if (FD_ISSET (_pipe[0], &fds)) {
			char c;
			read (_pipe[0], &c, sizeof (char));
			Log::debug () << "Interrupt uinput read thread" << std::endl;
			break;
		}
	}
}

const JSClass UInput::js_class = JS_HELPERS_CLASS("UInput", UInput);

const JSFunctionSpec UInput::js_fs[] = {
	JS_HELPERS_METHOD("create", UInput::create),
	JS_HELPERS_METHOD("destroy", UInput::destroy),
	JS_HELPERS_METHOD("setKey", UInput::setKey),
	JS_HELPERS_METHOD("setAbs", UInput::setAbs),
	JS_HELPERS_METHOD("setRel", UInput::setRel),
	JS_HELPERS_METHOD("setFF", UInput::setFF),
	JS_HELPERS_METHOD("setFFEffectsMax", UInput::setFFEffectsMax),
	JS_HELPERS_METHOD("sendKey", UInput::sendKey),
	JS_HELPERS_METHOD("sendAbs", UInput::sendAbs),
	JS_HELPERS_METHOD("sendRel", UInput::sendRel),
	JS_HELPERS_METHOD("sendSyn", UInput::sendSyn),
	JS_HELPERS_METHOD("sendEvent", UInput::sendEvent),
	JS_HELPERS_METHOD("setFFUploadEffect", UInput::setFFUploadEffect),
	JS_HELPERS_METHOD("setFFEraseEffect", UInput::setFFEraseEffect),
	JS_HELPERS_METHOD("setFFStart", UInput::setFFStart),
	JS_HELPERS_METHOD("setFFStop", UInput::setFFStop),
	JS_HELPERS_METHOD("setFFSetGain", UInput::setFFSetGain),
	JS_FS_END
};

const JSPropertySpec UInput::js_ps[] = {
	JS_HELPERS_PROPERTY("name", UInput::name, UInput::setName),
	JS_HELPERS_PROPERTY("busType", UInput::busType, UInput::setBusType),
	JS_HELPERS_PROPERTY("vendor", UInput::vendor, UInput::setVendor),
	JS_HELPERS_PROPERTY("product", UInput::product, UInput::setProduct),
	JS_HELPERS_PROPERTY("version", UInput::version, UInput::setVersion),
	JS_PS_END
};

bool UInput::_registered = ClassManager::registerClass<UInput::JsClass> ("UInput");
