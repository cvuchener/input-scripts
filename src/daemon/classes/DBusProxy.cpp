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

#include "DBusProxy.h"

#include "../DBusConnections.h"
#include "../ClassManager.h"

DBusProxy::DBusProxy (int bus, std::string service, std::string path, std::string interface):
	DBus::InterfaceProxy (interface),
	DBus::ObjectProxy (DBusConnections::getBus (static_cast<DBusConnections::Bus> (bus)), path, service.c_str ())
{
}

DBusProxy::~DBusProxy ()
{
}

static const char *getSignatureEnd (const char *signature)
{
	switch (signature[0]) {
	case '(': {
		const char *current = signature+1;
		while (*current != ')') {
			if (*current == '\0')
				throw std::invalid_argument ("Missing matching ) in signature");
			current = getSignatureEnd (current);
		}
		return current+1;
	}

	case '{': {
		const char *current = signature+1;
		for (unsigned int i = 0; i < 2; ++i)
			current = getSignatureEnd (current);
		if (*current != '}')
			throw std::invalid_argument ("Missing matching } in signature");
		return current+1;
	}

	case 'y':
	case 'b':
	case 'n':
	case 'q':
	case 'i':
	case 'u':
	case 'x':
	case 't':
	case 'd':
	case 'h':
	case 's':
	case 'o':
	case 'g':
	case 'v':
		return signature+1;

	case 'a':
		return getSignatureEnd (signature+1);

	default:
		throw std::invalid_argument ("Type signature not supported");
	}
}

template <typename T, typename T2 = T>
static inline void writeBasicType (JSContext *cx, DBus::MessageIter &iter, JS::HandleValue value) {
	using JsHelpers::readJSValue;
	T var;
	readJSValue (cx, var, value);
	iter << static_cast<T2> (var);
}

static const char *writeValue (JSContext *cx, DBus::MessageIter &iter, const char *signature, JS::HandleValue value)
{
	using JsHelpers::readJSValue;

	switch (signature[0]) {
	case 'y':
		writeBasicType<uint8_t> (cx, iter, value);
		return signature+1;
	case 'b':
		writeBasicType<bool> (cx, iter, value);
		return signature+1;
	case 'n':
		writeBasicType<int16_t> (cx, iter, value);
		return signature+1;
	case 'q':
		writeBasicType<uint16_t> (cx, iter, value);
		return signature+1;
	case 'i':
		writeBasicType<int32_t> (cx, iter, value);
		return signature+1;
	case 'u':
		writeBasicType<uint32_t> (cx, iter, value);
		return signature+1;
	case 'x':
		writeBasicType<int64_t> (cx, iter, value);
		return signature+1;
	case 't':
		writeBasicType<uint64_t> (cx, iter, value);
		return signature+1;
	case 'd':
		writeBasicType<double> (cx, iter, value);
		return signature+1;
	case 'h':
		throw std::invalid_argument ("Unix file descriptor not supported");
	case 's':
		writeBasicType<std::string> (cx, iter, value);
		return signature+1;
	case 'o':
		writeBasicType<std::string, DBus::Path> (cx, iter, value);
		return signature+1;
	case 'g':
		writeBasicType<std::string, DBus::Signature> (cx, iter, value);
		return signature+1;
	case 'a': {
		const char *end = getSignatureEnd (signature+1);
		std::string sig = std::string (signature+1, end);
		DBus::MessageIter array_iter = iter.new_array (sig.c_str ());
		if (signature[1] == '{') {
			// Dictionary
			std::string key_sig = std::string (signature+2, signature+3);
			std::string value_sig = std::string (signature+3, end-1);
			if (!value.isObject ())
				throw std::invalid_argument ("Dict signature expects an object");
			JS::RootedObject obj (cx, value.toObjectOrNull ());
			JSIdArray *ids = JS_Enumerate (cx, obj);
			unsigned int len = JS_IdArrayLength (cx, ids);
			for (unsigned int i = 0; i < len; ++i) {
				JS::RootedId id (cx, JS_IdArrayGet (cx, ids, i));
				JS::RootedValue key (cx);
				JS_IdToValue (cx, id, &key);
				JS::RootedValue prop (cx);
				JS_GetPropertyById (cx, obj, id, &prop);
				DBus::MessageIter dict_entry = array_iter.new_dict_entry ();
				writeValue (cx, dict_entry, key_sig.c_str (), key);
				writeValue (cx, dict_entry, value_sig.c_str (), prop);
				array_iter.close_container (dict_entry);
			}
			JS_DestroyIdArray (cx, ids);
		}
		else {
			// Array
			if (!JS_IsArrayObject (cx, value))
				throw std::invalid_argument ("Array expected");
			JS::RootedObject array (cx, value.toObjectOrNull ());
			unsigned int len;
			JS_GetArrayLength (cx, array, &len);
			for (unsigned int i = 0; i < len; ++i) {
				JS::RootedValue elem (cx);
				JS_GetElement (cx, array, i, &elem);
				writeValue (cx, array_iter, signature+1, elem);
			}
		}
		iter.close_container (array_iter);
		return end;
	}
	case '(': {
		// Struct
		const char *current = signature+1;
		unsigned int i = 0;
		if (!JS_IsArrayObject (cx, value))
			throw std::invalid_argument ("Struct signature expects an array");
		JS::RootedObject array (cx, value.toObjectOrNull ());
		unsigned int len;
		JS_GetArrayLength (cx, array, &len);
		DBus::MessageIter struct_iter = iter.new_struct ();
		while (*current != ')') {
			if (*current == '\0')
				throw std::invalid_argument ("Missing matching ) in signature");
			if (i >= len)
				throw std::invalid_argument ("Array too short");
			JS::RootedValue elem (cx);
			JS_GetElement (cx, array, i, &elem);
			current = writeValue (cx, struct_iter, current, elem);
			++i;
		}
		iter.close_container (struct_iter);
		return current+1;
	}
	case 'v': {
		// Variant
		if (!JS_IsArrayObject (cx, value))
			throw std::invalid_argument ("Variant signature expects an array");
		JS::RootedObject array (cx, value.toObjectOrNull ());
		unsigned int len;
		JS_GetArrayLength (cx, array, &len);
		if (len != 2)
			throw std::invalid_argument ("Array length must be 2");
		JS::RootedValue sig_value (cx), variant_value (cx);
		JS_GetElement (cx, array, 0, &sig_value);
		JS_GetElement (cx, array, 1, &variant_value);
		std::string sig;
		readJSValue (cx, sig, sig_value);
		DBus::MessageIter variant_iter = iter.new_variant (sig.c_str ());
		writeValue (cx, variant_iter, sig.c_str (), variant_value);
		iter.close_container (variant_iter);
		return signature+1;
	}

	default:
		throw std::invalid_argument ("Type signature not supported");
	}
}

template <typename T, typename T2 = T>
static inline void readBasicType (JSContext *cx, JS::MutableHandleValue value, DBus::MessageIter &iter)
{
	using JsHelpers::setJSValue;

	T var;
	iter >> var;
	setJSValue (cx, value, static_cast<T2> (var));
}

static void readValue (JSContext *cx, JS::MutableHandleValue value, DBus::MessageIter &iter)
{
	using JsHelpers::setJSValue;

	const char *signature = iter.signature ();
	switch (signature[0]) {
	case 'y':
		readBasicType<uint8_t> (cx, value, iter);
		return;
	case 'b':
		readBasicType<bool> (cx, value, iter);
		return;
	case 'n':
		readBasicType<int16_t> (cx, value, iter);
		return;
	case 'q':
		readBasicType<uint16_t> (cx, value, iter);
		return;
	case 'i':
		readBasicType<int32_t> (cx, value, iter);
		return;
	case 'u':
		readBasicType<uint32_t> (cx, value, iter);
		return;
	case 'x':
		readBasicType<int64_t> (cx, value, iter);
		return;
	case 't':
		readBasicType<uint64_t> (cx, value, iter);
		return;
	case 'd':
		readBasicType<double> (cx, value, iter);
		return;
	case 'h':
		throw std::invalid_argument ("Unix file descriptor not supported");
	case 's':
		readBasicType<std::string> (cx, value, iter);
		return;
	case 'o':
		readBasicType<DBus::Path, std::string> (cx, value, iter);
		return;
	case 'g':
		readBasicType<DBus::Signature, std::string> (cx, value, iter);
		return;
	case 'a': {
		DBus::MessageIter array_iter = iter.recurse ();
		if (signature[1] == '{') {
			// Dictionary
			JS::RootedObject obj (cx, JS_NewObject (cx, nullptr));
			while (!array_iter.at_end ()) {
				DBus::MessageIter dict_entry = array_iter.recurse ();
				JS::RootedValue value (cx);
				std::string key;
				dict_entry >> key;
				readValue (cx, &value, dict_entry);
				JS_SetProperty (cx, obj, key.c_str (), value);
				++array_iter;
			}
			value.setObject (*obj);

		}
		else {
			// Array
			JS::RootedObject array (cx, JS_NewArrayObject (cx, 0));
			unsigned int i = 0;
			while (!array_iter.at_end ()) {
				JS::RootedValue elem (cx);
				readValue (cx, &elem, array_iter);
				JS_SetElement (cx, array, i, elem);
				++i;
				++array_iter;
			}
			value.setObject (*array);
		}
		return;
	}
	case '(': {
		// Struct
		JS::RootedObject array (cx, JS_NewArrayObject (cx, 0));
		unsigned int i = 0;
		DBus::MessageIter struct_iter = iter.recurse ();
		while (!struct_iter.at_end ()) {
			JS::RootedValue elem (cx);
			readValue (cx, &elem, struct_iter);
			JS_SetElement (cx, array, i, elem);
			++i;
			++struct_iter;
		}
		value.setObject (*array);
		return;
	}
	case 'v': {
		// Variant
		JS::RootedObject array (cx, JS_NewArrayObject (cx, 0));
		DBus::MessageIter variant_iter = iter.recurse ();
		JS::RootedValue sig_value (cx), variant_value (cx);
		std::string sig = variant_iter.signature ();
		setJSValue (cx, &sig_value, sig);
		readValue (cx, &variant_value, variant_iter);
		JS_SetElement (cx, array, 0, sig_value);
		JS_SetElement (cx, array, 1, variant_value);
		value.setObject (*array);
		return;
	}

	default:
		throw std::invalid_argument ("Type signature not supported");
	}
}

bool DBusProxy::call (JSContext *cx, JS::CallArgs &args)
{
	using JsHelpers::readJSValue;

	std::string interface, method;
	try {
		readJSValue (cx, interface, args[0]);
	}
	catch (std::invalid_argument e) {
		JS_ReportError (cx, "Invalid interface argument: %s", e.what ());
		return false;
	}
	try {
		readJSValue (cx, method, args[1]);
	}
	catch (std::invalid_argument e) {
		JS_ReportError (cx, "Invalid method argument: %s", e.what ());
		return false;
	}

	DBus::CallMessage m;
	m.interface (interface.c_str ());
	m.member (method.c_str ());
	DBus::MessageIter wi = m.writer ();
	for (unsigned int i = 2; i < args.length (); i+=2) {
		if (i+1 >= args.length ())
			throw std::invalid_argument ("Missing value");
		std::string sig;
		try {
			readJSValue (cx, sig, args[i]);
			if (*getSignatureEnd (sig.c_str ()) != '\0')
				throw std::invalid_argument ("Signature contains more than one type");
		}
		catch (std::invalid_argument e) {
			JS_ReportError (cx, "Invalid signature for argument %d: %s", (i-2)/2, e.what ());
			return false;
		}
		try {
			writeValue (cx, wi, sig.c_str (), args[i+1]);
		}
		catch (std::invalid_argument e) {
			JS_ReportError (cx, "Cannot convert argument %d with signature %s: %s", (i-2)/2, sig.c_str (), e.what ());
			return false;
		}
	}
	std::unique_ptr<DBus::Message> ret;
	try {
		ret.reset (new DBus::Message (invoke_method (m)));
	}
	catch (std::exception &e) {
		JS_ReportError (cx, "Exception during DBus call: %s", e.what ());
		return false;
	}
	{
		JS::RootedObject array (cx, JS_NewArrayObject (cx, 0));
		DBus::MessageIter ri = ret->reader ();
		unsigned int i = 0;
		while (!ri.at_end ()) {
			JS::RootedValue elem (cx);
			try {
				readValue (cx, &elem, ri);
				JS_SetElement (cx, array, i, elem);
			}
			catch (std::invalid_argument e) {
				JS_ReportWarning (cx, "Error while reading out argument %d: %s", i, e.what ());
			}
			++i;
			++ri;
		}
		args.rval ().setObject (*array);
	}
	return true;
}

const JSClass DBusProxy::js_class = JS_HELPERS_CLASS("DBusProxy", DBusProxy);

const JSFunctionSpec DBusProxy::js_fs[] = {
	{
		"call",
		&JsHelpers::LLMethodWrapper<DBusProxy, &DBusProxy::call>,
		0, 0
	},
	JS_FS_END
};

const std::pair<std::string, int> DBusProxy::js_int_const[] = {
	// enum Bus
	{ "SystemBus", DBusConnections::SystemBus },
	{ "SessionBus", DBusConnections::SessionBus },
	{ "", 0 }
};

bool DBusProxy::_registered = ClassManager::registerClass<DBusProxy::JsClass> ("DBusProxy");
