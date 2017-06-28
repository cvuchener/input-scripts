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

#ifndef CLASS_MANAGER_H
#define CLASS_MANAGER_H

#include <map>
#include <string>
#include <memory>
#include "JsHelpers/Class.h"

class ClassManager
{
public:
	template<typename Class>
	static bool registerClass (const std::string &name)
	{
		return _classes.emplace (name, &classFactory<Class>).second;
	}

	static std::map<std::string, std::unique_ptr<JsHelpers::BaseClass>> initClasses (JSContext *cx, JS::HandleObject obj);

private:
	typedef std::unique_ptr<JsHelpers::BaseClass> Factory (JSContext *, JS::HandleObject, JS::HandleObject);

	template<typename Class>
	static std::unique_ptr<JsHelpers::BaseClass> classFactory (JSContext *cx, JS::HandleObject obj, JS::HandleObject parent_proto)
	{
		return std::make_unique<Class> (cx, obj, parent_proto);
	}

	static std::map<std::string, std::function<Factory>> _classes;
};

#endif
