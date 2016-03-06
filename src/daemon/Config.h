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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <vector>

class Config
{
public:
	bool loadConfig (const std::string &filename);

	struct ScriptRule
	{
		std::map<std::string, std::string> rules;
		std::string script_file;
	};
	std::vector<ScriptRule> default_scripts;

	static Config config;
	static std::string getConfigFilePath (const std::string &file);

private:
	Config ();

	static const std::string _user_config_path;
};

#endif

