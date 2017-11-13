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

#ifndef REMAPPER_H
#define REMAPPER_H

#include "../InputDevice.h"
#include "UInput.h"
#include "../jstpl/jstpl.h"

/**
 * Remap input events from an input device and send them to a uinput device.
 *
 * Remapper can only remap simple events: events with only a type, a code and a
 * value, and sent through the simpleEvent signal.
 *
 * Once configured and remapped, the remapper will pass remapped event from the
 * input device to uinput without need for any callbacks.
 *
 * Add remapped event with addEvent and configure it with calls to mapped_event
 * setters. An input event that was not added will be ignored.
 *
 * The remapper will process events after connect is called until disconnect is
 * called or the object is destroyed.
 */
class Remapper
{
public:
	struct event_id {
		uint16_t type, code;
		bool operator< (const event_id &other) const {
			return type < other.type || (type == other.type && code < other.code);
		}
	};

	/**
	 * Represents a single source event.
	 *
	 * Use the different methods for configuring the corresponding output
	 * events. Every setter returns the current object so calls can be
	 * chained.
	 *
	 * Without any change, the event is passed through without any change.
	 *
	 * Modifier conditions can be added: the event is only processed
	 * depending on the modifier value.
	 */
	class mapped_event
	{
		friend class Remapper;
	public:
		/**
		 * Change the output type and code.
		 */
		mapped_event *setEvent (uint16_t new_type, uint16_t new_code);
		/**
		 * Change the output code, leave the type unchanged.
		 */
		mapped_event *setCode (uint16_t new_code);
		/**
		 * Add a modifier condition.
		 *
		 * The modifier value must be between \p min and \p max (included).
		 */
		mapped_event *addModifier (uint16_t type, uint16_t code, int32_t min, int32_t max);
		/**
		 * Add a modifier condition.
		 *
		 * The modifier value must be at least \p min.
		 */
		mapped_event *addModifierMin (uint16_t type, uint16_t code, int32_t min);
		/**
		 * Add a modifier condition.
		 *
		 * The modifier value must be at most \p max.
		 */
		mapped_event *addModifierMax (uint16_t type, uint16_t code, int32_t max);
		/**
		 * Transform the event value.
		 *
		 * The new value is: (\p mult * value) / \p div + \p offset.
		 */
		mapped_event *setTransform (int mult, int div, int offset);
		/**
		 * Change the default value.
		 *
		 * The default value (default: 0) is used when modifiers are released.
		 */
		mapped_event *setDefaultValue (int32_t value);

		static const JSClass js_class;
		static const JSFunctionSpec js_fs[];

		using JsClass = jstpl::AbstractClass<mapped_event>;

	private:
		mapped_event (Remapper *parent, const event_id &event);

		bool testModifiers ();
		void process (int32_t value);
		void reset ();

		Remapper *_parent;
		event_id _event;

		struct modifier_range {
			int32_t min = INT_MIN;
			int32_t max = INT_MAX;
		};
		std::map<event_id, modifier_range> _modifiers;
		modifier_range &insertModifier (uint16_t type, uint16_t code);

		struct transform {
			int mult = 1;
			int div = 1;
			int offset = 0;
		} _transform;

		bool _state = false;
		int32_t _default_value = 0;

		static bool _registered;
	};

	/**
	 * Create a remapper listening for \p device and sending events to \p
	 * uinput.
	 */
	Remapper (InputDevice *device, UInput *uinput);
	Remapper (const Remapper &) = delete;
	~Remapper ();

	inline InputDevice *inputDevice () { return _device; }
	inline UInput *uinput () { return _uinput; }

	/**
	 * Create a remapped event.
	 *
	 * The new remapped event will only pass through the matching events
	 * and may need to be configured.
	 */
	mapped_event *addEvent (uint16_t old_type, uint16_t old_code);

	/**
	 * Connect to the input device simple events.
	 */
	void connect ();
	/**
	 * Disconnect the input device.
	 */
	void disconnect ();

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];

	using JsClass = jstpl::Class<Remapper, InputDevice *, UInput *>;

private:
	void addModifier (mapped_event *ev, const event_id &mod_ev);
	void event (uint16_t code, uint16_t type, int32_t value);

	InputDevice *_device;
	sigc::connection _conn;
	UInput *_uinput;

	std::multimap<event_id, mapped_event> _events;
	std::multimap<event_id, decltype(_events)::iterator> _modifier_events;

	static bool _registered;
};

#endif // REMAPPER_H
