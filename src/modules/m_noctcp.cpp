/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2019 linuxdaemon <linuxdaemon.irc@gmail.com>
 *   Copyright (C) 2019 Matt Schatz <genius3000@g3k.solutions>
 *   Copyright (C) 2013, 2017-2020 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2012, 2019 Robby <robby@chatbelgie.be>
 *   Copyright (C) 2012 Attila Molnar <attilamolnar@hush.com>
 *   Copyright (C) 2009 Uli Schlachter <psychon@inspircd.org>
 *   Copyright (C) 2009 Daniel De Graaf <danieldg@inspircd.org>
 *   Copyright (C) 2008 Robin Burchell <robin+git@viroteck.net>
 *   Copyright (C) 2007 Dennis Friis <peavey@inspircd.org>
 *   Copyright (C) 2004, 2006 Craig Edwards <brain@inspircd.org>
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
#include "modules/exemption.h"
#include "modules/extban.h"

class ModuleNoCTCP : public Module
{
 private:
	CheckExemption::EventProvider exemptionprov;
	ExtBan::Acting extban;
	SimpleChannelMode nc;
	SimpleUserMode ncu;

 public:
	ModuleNoCTCP()
		: Module(VF_VENDOR, "Adds channel mode C (noctcp) which allows channels to block messages which contain CTCPs and user mode T (u_noctcp) which allows users to block private messages that contain CTCPs.")
		, exemptionprov(this)
		, extban(this, "noctcp", 'C')
		, nc(this, "noctcp", 'C')
		, ncu(this, "u_noctcp", 'T')
	{
	}

	ModResult OnUserPreMessage(User* user, const MessageTarget& target, MessageDetails& details) override
	{
		if (!IS_LOCAL(user))
			return MOD_RES_PASSTHRU;

		std::string_view ctcpname;
		if (!details.IsCTCP(ctcpname) || irc::equals(ctcpname, "ACTION"))
			return MOD_RES_PASSTHRU;

		switch (target.type)
		{
			case MessageTarget::TYPE_CHANNEL:
			{
				if (user->HasPrivPermission("channels/ignore-noctcp"))
					return MOD_RES_PASSTHRU;

				Channel* c = target.Get<Channel>();
				for (const auto& [u, _] : c->GetUsers())
				{
					if (u->IsModeSet(ncu))
						details.exemptions.insert(u);
				}

				ModResult res = CheckExemption::Call(exemptionprov, user, c, "noctcp");
				if (res == MOD_RES_ALLOW)
					return MOD_RES_PASSTHRU;

				if (c->IsModeSet(nc))
				{
					user->WriteNumeric(Numerics::CannotSendTo(c, "CTCPs", &nc));
					return MOD_RES_DENY;
				}

				if (extban.GetStatus(user, c) == MOD_RES_DENY)
				{
					user->WriteNumeric(Numerics::CannotSendTo(c, "CTCPs", 'C', "noctcp"));
					return MOD_RES_DENY;
				}
				break;
			}
			case MessageTarget::TYPE_USER:
			{
				if (user->HasPrivPermission("users/ignore-noctcp"))
					return MOD_RES_PASSTHRU;

				User* u = target.Get<User>();
				if (u->IsModeSet(ncu))
				{
					user->WriteNumeric(Numerics::CannotSendTo(u, "CTCPs", &ncu));
					return MOD_RES_DENY;
				}
				break;
			}
			case MessageTarget::TYPE_SERVER:
			{
				if (user->HasPrivPermission("users/ignore-noctcp"))
					return MOD_RES_PASSTHRU;

				const UserManager::LocalList& list = ServerInstance->Users.GetLocalUsers();
				for (UserManager::LocalList::const_iterator iter = list.begin(); iter != list.end(); ++iter)
				{
					LocalUser* u = *iter;
					if (u->IsModeSet(ncu))
						details.exemptions.insert(u);
				}
				break;
			}
		}
		return MOD_RES_PASSTHRU;
	}
};

MODULE_INIT(ModuleNoCTCP)
