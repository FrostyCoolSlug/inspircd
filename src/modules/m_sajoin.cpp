/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2013, 2018-2021 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2013 Daniel Vassdal <shutter@canternet.org>
 *   Copyright (C) 2012-2014, 2016 Attila Molnar <attilamolnar@hush.com>
 *   Copyright (C) 2012, 2019 Robby <robby@chatbelgie.be>
 *   Copyright (C) 2009 Daniel De Graaf <danieldg@inspircd.org>
 *   Copyright (C) 2007 Dennis Friis <peavey@inspircd.org>
 *   Copyright (C) 2006 jamie <jamie@e03df62e-2008-0410-955e-edbf42e46eb7>
 *   Copyright (C) 2005, 2007 Robin Burchell <robin+git@viroteck.net>
 *   Copyright (C) 2004, 2007, 2010 Craig Edwards <brain@inspircd.org>
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

class CommandSajoin : public Command
{
 public:
	CommandSajoin(Module* Creator) : Command(Creator,"SAJOIN", 1)
	{
		allow_empty_last_param = false;
		access_needed = CmdAccess::OPERATOR;
		syntax = { "[<nick>] <channel>[,<channel>]+" };
		translation = { TR_NICK, TR_TEXT };
	}

	CmdResult Handle(User* user, const Params& parameters) override
	{
		const unsigned int channelindex = (parameters.size() > 1) ? 1 : 0;
		if (CommandParser::LoopCall(user, this, parameters, channelindex))
			return CmdResult::FAILURE;

		const std::string& channel = parameters[channelindex];
		const std::string& nickname = parameters.size() > 1 ? parameters[0] : user->nick;

		User* dest = ServerInstance->Users.Find(nickname);
		if ((dest) && (dest->registered == REG_ALL))
		{
			if (user != dest && !user->HasPrivPermission("users/sajoin-others"))
			{
				user->WriteNotice("*** You are not allowed to /SAJOIN other users (the privilege users/sajoin-others is needed to /SAJOIN others).");
				return CmdResult::FAILURE;
			}

			if (dest->server->IsService())
			{
				user->WriteNumeric(ERR_NOPRIVILEGES, "Cannot use an SA command on a U-lined client");
				return CmdResult::FAILURE;
			}
			if (IS_LOCAL(user) && !ServerInstance->Channels.IsChannel(channel))
			{
				/* we didn't need to check this for each character ;) */
				user->WriteNumeric(ERR_BADCHANMASK, channel, "Invalid channel name");
				return CmdResult::FAILURE;
			}

			Channel* chan = ServerInstance->Channels.Find(channel);
			if ((chan) && (chan->HasUser(dest)))
			{
				user->WriteRemoteNotice("*** " + dest->nick + " is already on " + channel);
				return CmdResult::FAILURE;
			}

			/* For local users, we call Channel::JoinUser which may create a channel and set its TS.
			 * For non-local users, we just return CmdResult::SUCCESS, knowing this will propagate it where it needs to be
			 * and then that server will handle the command.
			 */
			LocalUser* localuser = IS_LOCAL(dest);
			if (localuser)
			{
				chan = Channel::JoinUser(localuser, channel, true);
				if (chan)
				{
					ServerInstance->SNO.WriteGlobalSno('a', user->nick+" used SAJOIN to make "+dest->nick+" join "+channel);
					return CmdResult::SUCCESS;
				}
				else
				{
					user->WriteNotice("*** Could not join "+dest->nick+" to "+channel);
					return CmdResult::FAILURE;
				}
			}
			else
			{
				return CmdResult::SUCCESS;
			}
		}
		else
		{
			user->WriteNotice("*** No such nickname: '" + nickname + "'");
			return CmdResult::FAILURE;
		}
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override
	{
		return ROUTE_OPT_UCAST(parameters[0]);
	}
};

class ModuleSajoin : public Module
{
 private:
	CommandSajoin cmd;

 public:
	ModuleSajoin()
		: Module(VF_VENDOR | VF_OPTCOMMON, "Adds the /SAJOIN command which allows server operators to force users to join one or more channels.")
		, cmd(this)
	{
	}
};

MODULE_INIT(ModuleSajoin)
