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

#ifndef HIDPP20_DEVICE_H
#define HIDPP20_DEVICE_H

#include "../InputDevice.h"

#include <hidpp20/Device.h>
#include <hidpp/Dispatcher.h>

extern "C" {
#include <linux/input.h>
}

namespace HIDPP20
{
class IMouseButtonSpy;
class IOnboardProfiles;
class IReprogControlsV4;
}

class HIDPP20Device: public InputDevice
{
public:
	HIDPP20Device (HIDPP::Device &&device);
	virtual ~HIDPP20Device ();

	virtual void interrupt ();
	virtual void readEvents ();

	virtual InputDevice::Event getEvent (InputDevice::Event event);

	virtual std::string driver () const;
	virtual std::string name () const;
	virtual std::string serial () const;

	virtual operator bool () const;

	bool hasFeature (uint16_t feature_id) const;
	std::vector<uint8_t> callFunction (uint16_t id, unsigned int function, const std::vector<uint8_t> &params);

	void sendRawEvents (bool enable);

	enum EventType {
		EventRawHIDPP = EV_MAX+1,
		EventOnboardProfilesCurrentProfile,
		EventOnboardProfilesCurrentDPIIndex,
		EventReprogControlsV4Button, // Same as EV_KEY but "code" is a control ID.
		EventReprogControlsV4RawXY,
	};

	unsigned int MouseButtonSpy_GetButtonCount ();
	void MouseButtonSpy_Start ();
	void MouseButtonSpy_Stop ();
	std::vector<uint8_t> MouseButtonSpy_GetMapping ();
	void MouseButtonSpy_SetMapping (const std::vector<uint8_t> &button_mapping);

	bool OnboardProfiles_GetHostMode ();
	void OnboardProfiles_SetHostMode (bool host_mode);
	std::map<std::string, unsigned int> OnboardProfiles_GetCurrentProfile ();
	void OnboardProfiles_SetCurrentProfile (bool rom, unsigned int index);
	unsigned int OnboardProfiles_GetCurrentDPIIndex ();
	void OnboardProfiles_SetCurrentDPIIndex (unsigned int index);

	unsigned int ReprogControlsV4_GetControlCount ();
	std::map<std::string, int> ReprogControlsV4_GetControlInfo (unsigned int index);
	uint16_t ReprogControlsV4_GetRemap (uint16_t control_id);
	void ReprogControlsV4_SetRemap (uint16_t control_id, uint16_t remap);
	bool ReprogControlsV4_GetDivert (uint16_t control_id);
	void ReprogControlsV4_SetDivert (uint16_t control_id, bool divert);
	bool ReprogControlsV4_GetPersistentDivert (uint16_t control_id);
	void ReprogControlsV4_SetPersistentDivert (uint16_t control_id, bool divert);
	bool ReprogControlsV4_GetRawXYDivert (uint16_t control_id);
	void ReprogControlsV4_SetRawXYDivert (uint16_t control_id, bool divert);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::AbstractClass<HIDPP20Device> JsClass;

	virtual JSObject *makeJsObject (const JsHelpers::Thread *thread);

private:
	bool eventHandler (const HIDPP::Report &report);

	HIDPP20::Device _device;
	std::map<uint16_t, uint8_t> _features_id_index;
	std::map<uint8_t, uint16_t> _features_index_id;

	bool _send_raw_events;

	struct MouseButtonSpy;
	std::unique_ptr<MouseButtonSpy> _mbs;

	struct OnboardProfiles;
	std::unique_ptr<OnboardProfiles> _op;

	struct ReprogControlsV4;
	std::unique_ptr<ReprogControlsV4> _rc4;

	std::vector<HIDPP::Dispatcher::listener_iterator> _listener_iterators;
	MTQueue<HIDPP::Report> _report_queue;

	static bool _registered;
};

#endif
