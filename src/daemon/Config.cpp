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

#include "Config.h"

#include <fstream>
#include <json/json.h>
#include <json/reader.h>
#include "Log.h"

extern "C" {
#include <unistd.h>
#include <sys/stat.h>
}

Config::Config ()
{
}

Config Config::config;

bool Config::loadConfig (const std::string &filename)
{
	std::string config_file_path;
	try {
		config_file_path = getConfigFilePath (filename);
	}
	catch (std::exception &e) {
		Log::error () << "Could not find " << filename << ": "
			      << e.what () << std::endl;
		return false;
	}
	Log::info () << "Loading config file: " << config_file_path << std::endl;
	std::ifstream config_file (config_file_path);
	Json::Reader reader;
	Json::Value config_json;
	if (!reader.parse (config_file, config_json)) {
		Log::error () << "Cannot parse " << config_file_path << ": "
			      << reader.getFormattedErrorMessages () << std::endl;
		return false;
	}
	if (config_json.isMember ("library_scripts")) {
		Json::Value library_scripts_json = config_json["library_scripts"];
		if (!library_scripts_json.isObject ()) {
			Log::error () << "library_scripts must be an object." << std::endl;
			goto library_scripts_end;
		}
		for (const auto &key: library_scripts_json.getMemberNames ()) {
			library_scripts.emplace (key, library_scripts_json[key].asString ());
		}
	}
library_scripts_end:
	if (config_json.isMember ("default_scripts")) {
		Json::Value default_scripts_json = config_json["default_scripts"];
		if (!default_scripts_json.isArray ()) {
			Log::error () << "default_scripts must be an array" << std::endl;
			goto default_script_end;
		}
		for (unsigned int i = 0; i < default_scripts_json.size (); ++i) {
			Json::Value script = default_scripts_json[i];
			default_scripts.emplace_back ();
			if (!script.isMember ("file")) {
				Log::error () << "default_scripts[" << i << "] need a \"file\" property." << std::endl;
				default_scripts.pop_back ();
				continue;
			}
			auto &last = default_scripts.back ();
			last.script_file = script["file"].asString ();
			Log::debug () << "Added rule for file: " << last.script_file << std::endl;
			for (auto &rule: { "driver", "name", "serial" })
				if (script.isMember (rule)) {
					Log::debug () << rule << " = " << script[rule].asString () << std::endl;
					last.rules.emplace (rule, script[rule].asString ());
				}
		}
	}
	else {
		Log::warning () << "Missing default_scripts section in "
				<< config_file_path << std::endl;
	}
default_script_end:
	return true;
}

static bool fileExists (const std::string &filename)
{
	int ret;
	struct stat statbuf;
	ret = stat (filename.c_str (), &statbuf);
	return ret != -1 || errno != ENOENT;
}

std::string Config::getConfigFilePath (const std::string &filename)
{
	if (filename[0] == '/')
		return filename;

	std::string path;
	if (!_user_config_path.empty ()) {
		path = _user_config_path + "/input-scripts/" + filename;
		if (fileExists (path))
			return path;
	}

	path = "/etc/input-scripts/" + filename;
	if (fileExists (path))
		return path;

	throw std::system_error (ENOENT, std::system_category ());
}

static std::string setUserConfigPath ()
{
	const char *xdg_config_home = std::getenv ("XDG_CONFIG_HOME");
	if (xdg_config_home)
		return std::string (xdg_config_home);

	const char *home = std::getenv ("HOME");
	if (home)
		return std::string (home) + "/.config";

	Log::warning () << "No user configuration path set." << std::endl;
	return std::string ();
}

const std::string Config::_user_config_path = setUserConfigPath ();

