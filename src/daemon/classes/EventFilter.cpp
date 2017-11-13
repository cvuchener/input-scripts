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

#include "EventFilter.h"

EventFilter::EventFilter (InputDevice *input):
	_input (input),
	_simple_only (false),
	_inverted (false)
{
}

EventFilter::~EventFilter ()
{
	_simple_conn.disconnect ();
	_all_conn.disconnect ();
}

bool EventFilter::simpleOnly () const
{
	return _simple_only;
}

void EventFilter::setSimpleOnly (bool simple_only)
{
	_simple_only = simple_only;
}

bool EventFilter::inverted () const
{
	return _inverted;
}

void EventFilter::setInverted (bool invert)
{
	_inverted = invert;
}

void EventFilter::addMatchType (uint16_t type)
{
	_simple_filters.emplace (type, std::monostate ());
}

void EventFilter::addMatchCode (uint16_t type, uint16_t code)
{
	_simple_filters.emplace (type, code);
}

void EventFilter::addMatchCodeRange (uint16_t type, uint16_t min_code, uint16_t max_code)
{
	_simple_filters.emplace (type, std::make_pair (min_code, max_code));
}

void EventFilter::addMatchProp (uint16_t type, const std::string &prop, int value)
{
	_prop_filters[type][prop].emplace_back (value);
}

void EventFilter::addMatchPropRange (uint16_t type, const std::string &prop, int min_value, int max_value)
{
	_prop_filters[type][prop].emplace_back (std::make_pair (min_value, max_value));
}

void EventFilter::connect ()
{
	if (_simple_conn.connected ()) {
		Log::warning () << "Filter already connected." << std::endl;
		return;
	}

	_simple_conn = _input->simpleEvent.connect ([this] (uint16_t type, uint16_t code, int32_t value) {
		processSimpleEvent (type, code, value);
	});
	if (!_simple_only) {
		_all_conn = _input->event.connect ([this] (const InputDevice::Event &event) {
			processEvent (event);
		});
	}
}

void EventFilter::disconnect ()
{
	_simple_conn.disconnect ();
	_all_conn.disconnect ();
}

template <typename T>
struct FilterTest
{
	T value;

	bool operator() (std::monostate)
	{
		return true;
	}
	bool operator() (T val)
	{
		return value == val;
	}
	bool operator() (const std::pair<T, T> &range)
	{
		return value >= range.first && value <= range.second;
	}
};

bool EventFilter::testEvent (const InputDevice::Event &event) {
	uint16_t type = event.at ("type");
	auto code = event.find ("code");
	if (code != event.end ()) {
		if (testSimpleEvent (type, code->second))
			return true;
	}
	auto prop_filters = _prop_filters.find (type);
	if (prop_filters == _prop_filters.end ())
		return false;
	for (const auto &p: prop_filters->second) {
		auto prop = event.find (p.first);
		if (prop == event.end ())
			continue;
		FilterTest<int> test = { prop->second };
		for (const auto &filter: p.second)
			if (std::visit (test, filter))
				return true;
	}
	return false;
}

bool EventFilter::testSimpleEvent (uint16_t type, uint16_t code)
{
	FilterTest<uint16_t> test = { code };
	auto range = _simple_filters.equal_range (type);
	for (auto it = range.first; it != range.second; ++it) {
		if (std::visit (test, it->second))
			return true;
	}
	return false;
}

void EventFilter::processEvent (const InputDevice::Event &ev)
{
	if (testEvent (ev) != _inverted)
		event.emit (ev);
}

void EventFilter::processSimpleEvent (uint16_t type, uint16_t code, int32_t value)
{
	if (testSimpleEvent (type, code) != _inverted)
		simpleEvent.emit (type, code, value);
}

const JSClass EventFilter::js_class = jstpl::make_class<EventFilter> ("EventFilter");

const JSFunctionSpec EventFilter::js_fs[] = {
	jstpl::make_method<&EventFilter::addMatchType> ("addMatchType"),
	jstpl::make_method<&EventFilter::addMatchCode> ("addMatchCode"),
	jstpl::make_method<&EventFilter::addMatchCodeRange> ("addMatchCodeRange"),
	jstpl::make_method<&EventFilter::addMatchProp> ("addMatchProp"),
	jstpl::make_method<&EventFilter::addMatchPropRange> ("addMatchPropRange"),
	jstpl::make_method<&EventFilter::connect> ("connect"),
	jstpl::make_method<&EventFilter::disconnect> ("disconnect"),
	JS_FS_END
};

const JSPropertySpec EventFilter::js_ps[] = {
	jstpl::make_property<&EventFilter::simpleOnly, &EventFilter::setSimpleOnly> ("simple_only"),
	jstpl::make_property<&EventFilter::inverted, &EventFilter::setInverted> ("inverted"),
	JS_PS_END
};

const jstpl::SignalMap EventFilter::js_signals = {
	{ "event", jstpl::make_signal_connector (&EventFilter::event) },
	{ "simpleEvent", jstpl::make_signal_connector (&EventFilter::simpleEvent) },
};

bool EventFilter::_registered = jstpl::ClassManager::registerClass<EventFilter::JsClass> ();

