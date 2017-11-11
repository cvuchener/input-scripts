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

#ifndef HIDPP10_DEVICE_H
#define HIDPP10_DEVICE_H

#include "../InputDevice.h"
#include "../event/EventDevice.h"

#include <hidpp10/Device.h>
#include <hidpp/Dispatcher.h>

namespace HIDPP10
{
struct MouseInfo;
}

class HIDPP10Device: public InputDevice
{
public:
	HIDPP10Device (HIDPP::Device &&device, const std::vector<std::string> &paths);
	virtual ~HIDPP10Device ();

	void start () override;
	void stop () override;

	InputDevice::Event getEvent (InputDevice::Event event) override;
	int32_t getSimpleEvent (uint16_t type, uint16_t code) override;

	std::string driver () const override;
	std::string name () const override;
	std::string serial () const override;

	std::vector<EventDevice *> getEventDevices ();

	std::vector<uint8_t> setRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size);
	std::vector<uint8_t> getRegister (uint8_t address, const std::vector<uint8_t> &params, std::size_t result_size);

	unsigned int getIndividualFeatureFlags ();
	void setIndividualFeatureFlags (unsigned int flags);

	std::vector<unsigned int> getCurrentResolution ();
	void setCurrentResolution (const std::vector<unsigned int> &dpi);

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const std::pair<std::string, int> js_int_const[];
	typedef JsHelpers::AbstractClass<HIDPP10Device> JsClass;

	virtual JSObject *makeJsObject (const JsHelpers::Thread *thread);

private:
	bool eventHandler (const HIDPP::Report &report);

	HIDPP10::Device _device;
	std::vector<EventDevice> _evdev;
	const HIDPP10::MouseInfo *_mouse_info;

	std::vector<HIDPP::Dispatcher::listener_iterator> _listener_iterators;
	MTQueue<HIDPP::Report> _report_queue;

	static bool _registered;
};

#endif
