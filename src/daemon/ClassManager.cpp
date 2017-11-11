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

#include "ClassManager.h"

#include <stack>

std::map<std::string, std::unique_ptr<JsHelpers::BaseClass>> ClassManager::initClasses (JSContext *cx, JS::HandleObject obj)
{
	if (_inheritance_trees.empty ())
		buildInheritanceTrees ();

	std::map<std::string, std::unique_ptr<JsHelpers::BaseClass>> classes;
	std::stack<ClassData::Node *> s;
	for (auto node: _inheritance_trees)
		s.push (node);
	while (!s.empty ()) {
		auto node = s.top ();
		auto &cls_name = node->first;
		auto &cls = node->second;
		s.pop ();
		for (auto child: cls.children)
			s.push (child);

		if (cls.parent) {
			auto parent = classes.find (cls.parent_name);
			if (parent == classes.end ()) {
				Log::error () << "Could not find parent " << cls.parent_name
					      << " of class " << cls_name << std::endl;
				continue;
			}
			classes.emplace (cls_name,
					 cls.factory (cx, obj, parent->second.get ()));
		}
		else {
			classes.emplace (cls_name,
					 cls.factory (cx, obj, nullptr));
		}
	}
	return classes;
}

bool ClassManager::inherits (const std::string &cls_name, const std::string &parent_name)
{
	if (_inheritance_trees.empty ())
		buildInheritanceTrees ();

	auto cls = _classes.find (cls_name);
	if (cls == _classes.end ()) {
		Log::error () << "Class " << cls_name << " not found." << std::endl;
		return false;
	}
	auto parent = _classes.find (parent_name);
	if (parent == _classes.end ()) {
		Log::error () << "Class " << parent_name << " not found." << std::endl;
		return false;
	}

	if (cls == parent)
		return true;

	if (cls->second.root != parent->second.root)
		return false;
	ClassData::Node *current = cls->second.parent;
	while (current != nullptr && current != &(*parent))
		current = current->second.parent;
	return current == &(*parent);
}

void ClassManager::buildInheritanceTrees ()
{
	for (auto &p: _classes)
		p.second.root = &p;
	for (auto &p: _classes) {
		auto &cls_name = p.first;
		auto &cls = p.second;
		if (cls.parent_name.empty ()) {
			_inheritance_trees.push_back (&p);
		}
		else {
			auto it = _classes.find (cls.parent_name);
			if (it == _classes.end ()) {
				Log::error () << "Could not find parent " << cls.parent_name
					      << " of class " << cls_name << std::endl;
				continue;
			}
			auto &parent_name = it->first;
			auto &parent = it->second;
			if (parent.root == &p) {
				Log::error () << "Cyclic inheritance: " << parent_name
					      << " already inherits from " << cls_name << std::endl;
				continue;
			}
			cls.parent = &(*it);
			parent.children.push_back (&p);

			// Update root in sub-tree
			std::stack<ClassData::Node *> s;
			s.push (&p);
			while (!s.empty ()) {
				auto node = s.top ();
				s.pop ();
				node->second.root = parent.root;
				for (auto child: node->second.children)
					s.push (child);
			}
		}
	}
}

std::map<std::string, ClassManager::ClassData> ClassManager::_classes;
std::vector<ClassManager::ClassData::Node *> ClassManager::_inheritance_trees;
