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

#ifndef JSTPL_CLASS_MANAGER_H
#define JSTPL_CLASS_MANAGER_H

#include <jsapi.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace jstpl
{

class BaseClass;

class ClassManager
{
public:
	template<typename Class>
	static bool registerClass (const std::string &parent = std::string ());

	static std::map<std::string, std::unique_ptr<BaseClass>> initClasses (JSContext *cx, JS::HandleObject obj);

	static bool inherits (const std::string &cls, const std::string &parent);

private:
	typedef std::unique_ptr<BaseClass> Factory (JSContext *, JS::HandleObject, const BaseClass *);

	template<typename Class>
	static std::unique_ptr<BaseClass> classFactory (JSContext *cx, JS::HandleObject obj, const BaseClass *parent);

	struct ClassData {
		std::string parent_name;
		std::function<Factory> factory;

		using Node = std::pair<const std::string, ClassData>;
		Node *parent;
		Node *root;
		std::vector<Node *> children;

		template <typename F>
		ClassData (const std::string &parent, F factory):
			parent_name (parent),
			factory (factory),
			parent (nullptr)
		{
		}
	};

	static void buildInheritanceTrees ();

	static std::map<std::string, ClassData> _classes;
	static std::vector<ClassData::Node *> _inheritance_trees;
};

}

#include "Class.h"

namespace jstpl
{

template<typename Class>
bool ClassManager::registerClass (const std::string &parent)
{
	return _classes.emplace (std::piecewise_construct,
				 std::forward_as_tuple (Class::type::js_class.name),
				 std::forward_as_tuple (parent, &classFactory<Class>)
				).second;
}

template<typename Class>
std::unique_ptr<BaseClass> ClassManager::classFactory (JSContext *cx, JS::HandleObject obj, const BaseClass *parent)
{
	return std::make_unique<Class> (cx, obj, parent);
}

}

#endif
