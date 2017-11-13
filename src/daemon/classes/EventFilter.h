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

#ifndef EVENT_FILTER_H
#define EVENT_FILTER_H

#include "../InputDevice.h"
#include "../jstpl/jstpl.h"

#include <map>
#include <variant>

/**
 * Filters events from an input device.
 *
 * Allow a script to receive only matching events, thus lowering the cpu
 * usage.
 *
 * Events are passed through when matching *any* of the rules.
 *
 * Properties:
 *  - `simple_only` (\c bool, default: \c false): When true, only process
 *    simple events, \c event signal is disconnected. This property must be
 *    changed before connecting.
 *    \see simpleOnly, setSimpleOnly
 *  - `inverted` (\c bool, default: \c false): When true, unmatched events are
 *    sent instead of matching ones.
 *    \see inverted, setInverted
 */
class EventFilter
{
public:
	/**
	 * Create a filter for input device \p input.
	 */
	EventFilter (InputDevice *input);
	~EventFilter ();

	bool simpleOnly () const;
	void setSimpleOnly (bool simple_only);

	bool inverted () const;
	void setInverted (bool invert);

	/**
	 * Match any event with type \p type.
	 */
	void addMatchType (uint16_t type);
	/**
	 * Match events with type \p type and code \p code.
	 */
	void addMatchCode (uint16_t type, uint16_t code);
	/**
	 * Match events with type \p type and code between \p min_code and \p
	 * max_code (included).
	 */
	void addMatchCodeRange (uint16_t type, uint16_t min_code, uint16_t max_code);
	/**
	 * Match events with type \p type, and property \p prop equal to \p value.
	 *
	 * This rule do not apply to simple events.
	 */
	void addMatchProp (uint16_t type, const std::string &prop, int value);
	/**
	 * Match events with type \p type, and property \p prop between \p
	 * min_value and \p max_value (included).
	 *
	 * This rule do not apply to simple events.
	 */
	void addMatchPropRange (uint16_t type, const std::string &prop, int min_value, int max_value);

	/**
	 * Connect to the input device events.
	 */
	void connect ();
	/**
	 * Disconnect from the input device.
	 */
	void disconnect ();

	/**
	 * Filtered events.
	 *
	 * The signal is not emitted when `simple_only` is \c true.
	 */
	sigc::signal<void (InputDevice::Event)> event;
	/**
	 * Filtered simple events.
	 */
	sigc::signal<void (uint16_t, uint16_t, int32_t)> simpleEvent;

	static const JSClass js_class;
	static const JSFunctionSpec js_fs[];
	static const JSPropertySpec js_ps[];
	static const jstpl::SignalMap js_signals;
	using JsClass = jstpl::Class<EventFilter, InputDevice *>;

private:
	bool testEvent (const InputDevice::Event &event);
	bool testSimpleEvent (uint16_t type, uint16_t code);
	void processEvent (const InputDevice::Event &event);
	void processSimpleEvent (uint16_t type, uint16_t code, int32_t value);

	InputDevice *_input;
	bool _simple_only;
	bool _inverted;

	template <typename T>
	using value_filter = std::variant<std::monostate, T, std::pair<T, T>>;

	std::multimap<uint16_t, value_filter<uint16_t>> _simple_filters;
	std::map<uint16_t, std::map<std::string, std::vector<value_filter<int>>>> _prop_filters;

	sigc::connection _simple_conn, _all_conn;

	static bool _registered;
};

#endif // EVENT_FILTER_H
