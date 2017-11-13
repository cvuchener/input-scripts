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

#include "Remapper.h"

Remapper::mapped_event::mapped_event (Remapper *parent, const event_id &event):
	_parent (parent),
	_event (event)
{
}

Remapper::mapped_event *Remapper::mapped_event::setEvent (uint16_t new_type, uint16_t new_code)
{
	_event.type = new_type;
	_event.code = new_code;
	return this;
}

Remapper::mapped_event *Remapper::mapped_event::setCode (uint16_t new_code)
{
	_event.code = new_code;
	return this;
}

Remapper::mapped_event::modifier_range &Remapper::mapped_event::insertModifier (uint16_t type, uint16_t code)
{
	event_id ev = { type, code };
	auto ret = _modifiers.try_emplace (ev);
	if (!ret.second)
		throw std::invalid_argument ("Modifier event already exists.");
	_parent->addModifier (this, ev);
	return ret.first->second;
}

Remapper::mapped_event *Remapper::mapped_event::addModifier (uint16_t type, uint16_t code, int32_t min, int32_t max)
{
	auto &range = insertModifier (type, code);
	range.min = min;
	range.max = max;
	return this;
}

Remapper::mapped_event *Remapper::mapped_event::addModifierMin (uint16_t type, uint16_t code, int32_t min)
{
	auto &range = insertModifier (type, code);
	range.min = min;
	return this;
}

Remapper::mapped_event *Remapper::mapped_event::addModifierMax (uint16_t type, uint16_t code, int32_t max)
{
	auto &range = insertModifier (type, code);
	range.max = max;
	return this;
}

Remapper::mapped_event *Remapper::mapped_event::setTransform (int mult, int div, int offset)
{
	_transform.mult = mult;
	_transform.div = div;
	_transform.offset = offset;
	return this;
}

Remapper::mapped_event *Remapper::mapped_event::setDefaultValue (int32_t value)
{
	_default_value = value;
	return this;
}

bool Remapper::mapped_event::testModifiers ()
{
	for (const auto &p: _modifiers) {
		int32_t value = _parent->inputDevice ()->getSimpleEvent (p.first.type, p.first.code);
		if (value < p.second.min || value > p.second.max)
			return false;
	}
	return true;
}

void Remapper::mapped_event::process (int32_t value)
{
	int32_t new_value = (_transform.mult * value) / _transform.div + _transform.offset;
	_parent->uinput ()->sendEvent (_event.type, _event.code, new_value);
}

void Remapper::mapped_event::reset ()
{
	_parent->uinput ()->sendEvent (_event.type, _event.code, _default_value);
}

const JSClass Remapper::mapped_event::js_class = JsHelpers::make_class<Remapper::mapped_event> ("RemapperEvent");

const JSFunctionSpec Remapper::mapped_event::js_fs[] = {
	JsHelpers::make_method<&Remapper::mapped_event::setEvent> ("setEvent"),
	JsHelpers::make_method<&Remapper::mapped_event::setCode> ("setCode"),
	JsHelpers::make_method<&Remapper::mapped_event::addModifier> ("addModifier"),
	JsHelpers::make_method<&Remapper::mapped_event::addModifierMin> ("addModifierMin"),
	JsHelpers::make_method<&Remapper::mapped_event::addModifierMax> ("addModifierMax"),
	JsHelpers::make_method<&Remapper::mapped_event::setTransform> ("setTransform"),
	JsHelpers::make_method<&Remapper::mapped_event::setDefaultValue> ("setDefaultValue"),
	JS_FS_END
};

bool Remapper::mapped_event::_registered = ClassManager::registerClass<Remapper::mapped_event::JsClass> ();

Remapper::Remapper (InputDevice *device, UInput *uinput):
	_device (device),
	_uinput (uinput)
{
}

Remapper::~Remapper ()
{
	_conn.disconnect ();
}

Remapper::mapped_event *Remapper::addEvent (uint16_t old_type, uint16_t old_code)
{
	event_id ev = { old_type, old_code };
	auto it = _events.emplace (ev, mapped_event (this, ev));
	return &it->second;
}

void Remapper::connect ()
{
	if (_conn.connected ())
		return;
	_conn = _device->simpleEvent.connect ([this] (uint16_t code, uint16_t type, int32_t value) {
		event (code, type, value);
	});
}

void Remapper::disconnect ()
{
	_conn.disconnect ();
}

void Remapper::addModifier (mapped_event *ev, const event_id &mod_ev)
{
	auto it = std::find_if (_events.begin (), _events.end (),
				[ev] (const auto &p) { return &p.second == ev; });
	assert (it != _events.end ());
	_modifier_events.emplace (mod_ev, it);
}

void Remapper::event (uint16_t code, uint16_t type, int32_t value)
{
	event_id ev = { code, type };
	auto ev_range = _events.equal_range (ev);
	for (auto it = ev_range.first; it != ev_range.second; ++it) {
		auto &mapped = it->second;
		if (mapped.testModifiers ())
			mapped.process (value);
	}
	auto mod_range = _modifier_events.equal_range (ev);
	for (auto it = mod_range.first; it != mod_range.second; ++it) {
		auto &e = it->second->first;
		auto &mapped = it->second->second;
		bool new_state = mapped.testModifiers ();
		if (mapped._state && !new_state) {
			mapped.reset ();
		}
		if (!mapped._state && new_state) {
			mapped.process (_device->getSimpleEvent (e.type, e.code));
		}
		mapped._state = new_state;
	}
}

const JSClass Remapper::js_class = JsHelpers::make_class<Remapper> ("Remapper");

const JSFunctionSpec Remapper::js_fs[] = {
	JsHelpers::make_method<&Remapper::addEvent> ("addEvent"),
	JsHelpers::make_method<&Remapper::connect> ("connect"),
	JsHelpers::make_method<&Remapper::disconnect> ("disconnect"),
	JS_FS_END
};

bool Remapper::_registered = ClassManager::registerClass<Remapper::JsClass> ();
