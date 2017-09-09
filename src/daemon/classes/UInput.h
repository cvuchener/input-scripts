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

#ifndef UINPUT_H
#define UINPUT_H

#include <cstdint>
#include <thread>
#include <map>
#include "../JsHelpers/JsHelpers.h"

extern "C" {
#include <linux/uinput.h>
}

class UInput
{
public:
	UInput ();
	UInput (const UInput &) = delete;
	~UInput ();

	std::string name () const;
	void setName (const std::string &name);
	uint16_t busType () const;
	void setBusType (uint16_t bustype);
	uint16_t vendor () const;
	void setVendor (uint16_t vendor);
	uint16_t product () const;
	void setProduct (uint16_t product);
	uint16_t version () const;
	void setVersion (uint16_t version);

	void setKey (uint16_t code);
	void setAbs (uint16_t code, int32_t min, int32_t max, int32_t fuzz, int32_t flat);
	void setRel (uint16_t code);
	void setFF (uint16_t code);
	void setFFEffectsMax (uint32_t effects_max);

	void create ();
	void destroy ();

	void sendKey (uint16_t code, int32_t value);
	void sendAbs (uint16_t code, int32_t value);
	void sendRel (uint16_t code, int32_t value);
	void sendSyn (uint16_t code = 0);
	void sendEvent (uint16_t type, uint16_t code, int32_t value);

	void setFFUploadEffect (std::function<void (int, std::map<std::string, int>)>);
	void setFFEraseEffect (std::function<void (int)>);
	void setFFStart (std::function<void (int)>);
	void setFFStop (std::function<void (int)>);
	void setFFSetGain (std::function<void (int32_t)>);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const JSPropertySpec js_ps[];

	typedef JsHelpers::Class<UInput> JsClass;

private:
	void readEvents ();

	struct uinput_user_dev _uidev;
	bool _use_ff;
	int _fd;
	int _pipe[2];
	std::thread _thread;
	std::function<void (int, std::map<std::string, int>)> _ff_upload_effect;
	std::function<void (int)> _ff_erase_effect;
	std::function<void (int)> _ff_start;
	std::function<void (int)> _ff_stop;
	std::function<void (int32_t)> _ff_set_gain;

	static bool _registered;
};

#endif // UINPUT_H
