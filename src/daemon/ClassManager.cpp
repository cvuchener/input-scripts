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

std::map<std::string, std::unique_ptr<JsHelpers::BaseClass>> ClassManager::initClasses (JSContext *cx, JS::HandleObject obj)
{
	std::map<std::string, std::unique_ptr<JsHelpers::BaseClass>> classes;
	for (const auto &p: _classes)
		classes.emplace (p.first, p.second (cx, obj, JS::NullPtr ()));
	return classes;
}

std::map<std::string, std::function<ClassManager::Factory>> ClassManager::_classes;
