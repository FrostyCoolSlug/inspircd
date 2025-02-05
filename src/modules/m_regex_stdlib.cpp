/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2019 Robby <robby@chatbelgie.be>
 *   Copyright (C) 2013, 2016, 2018, 2020 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2013 Attila Molnar <attilamolnar@hush.com>
 *   Copyright (C) 2012 ChrisTX <xpipe@hotmail.de>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "inspircd.h"
#include "modules/regex.h"

#include <regex>

class StdLibPattern final
	: public Regex::Pattern
{
 private:
	std::regex regex;

 public:
	StdLibPattern(const std::string& pattern, uint8_t options, std::regex::flag_type type)
		: Regex::Pattern(pattern, options)
	{
		// Convert the generic pattern options to stdlib pattern flags.
		std::regex_constants::syntax_option_type flags = type | std::regex::optimize;
		if (options & Regex::OPT_CASE_INSENSITIVE)
			flags |= std::regex::icase;

		try
		{
			regex.assign(pattern, flags);
		}
		catch(const std::regex_error& error)
		{
			throw Regex::Exception(pattern, error.what());
		}
	}

	bool IsMatch(const std::string& text) override
	{
		return std::regex_search(text, regex);
	}
};

class StdLibEngine final
	: public Regex::Engine
{
 public:
	std::regex::flag_type regextype;

	StdLibEngine(Module* Creator)
		: Regex::Engine(Creator, "stdregex")
	{
	}

	Regex::PatternPtr Create(const std::string& pattern, uint8_t options) override
	{
		return std::make_shared<StdLibPattern>(pattern, options, regextype);
	}
};

class ModuleRegexStdLib : public Module
{
 private:
	StdLibEngine regex;

 public:
	ModuleRegexStdLib()
		: Module(VF_VENDOR, "Provides the stdregex regular expression engine which uses the C++11 std::regex regular expression matching system.")
		, regex(this)
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		auto tag = ServerInstance->Config->ConfValue("stdregex");
		regex.regextype = tag->getEnum("type", std::regex::ECMAScript,
		{
			{ "awk",        std::regex::awk },
			{ "bre",        std::regex::basic },
			{ "ecmascript", std::regex::ECMAScript },
			{ "egrep",      std::regex::egrep },
			{ "ere",        std::regex::extended },
			{ "grep",       std::regex::grep },
		});
	}
};

MODULE_INIT(ModuleRegexStdLib)
