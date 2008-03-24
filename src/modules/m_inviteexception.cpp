/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2008 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "u_listmode.h"

/* $ModDesc: Provides support for the +I channel mode */
/* $ModDep: ../../include/u_listmode.h */

/*
 * Written by Om <om@inspircd.org>, April 2005.
 * Based on m_exception, which was originally based on m_chanprotect and m_silence
 *
 * The +I channel mode takes a nick!ident@host, glob patterns allowed,
 * and if a user matches an entry on the +I list then they can join the channel,
 * ignoring if +i is set on the channel
 * Now supports CIDR and IP addresses -- Brain
 */

class InspIRCd* ServerInstance;

/** Handles channel mode +I
 */
class InviteException : public ListModeBase
{
 public:
	InviteException(InspIRCd* Instance) : ListModeBase(Instance, 'I', "End of Channel Invite Exception List", "346", "347", true) { }
};

class ModuleInviteException : public Module
{
	InviteException* ie;
public:
	ModuleInviteException(InspIRCd* Me) : Module(Me)
	{
		ie = new InviteException(ServerInstance);
		if (!ServerInstance->Modes->AddMode(ie))
			throw ModuleException("Could not add new modes!");
		ServerInstance->Modules->PublishInterface("ChannelBanList", this);

		ie->DoImplements(this);
		Implementation eventlist[] = { I_OnRequest, I_On005Numeric, I_OnCheckInvite };
		ServerInstance->Modules->Attach(eventlist, this, 3);
	}
	
	virtual void On005Numeric(std::string &output)
	{
		output.append(" INVEX=I");
	}
	 
	virtual int OnCheckInvite(User* user, Channel* chan)
	{
		if(chan != NULL)
		{
			modelist* list;
			chan->GetExt(ie->GetInfoKey(), list);
			if (list)
			{
				char mask[MAXBUF];
				snprintf(mask, MAXBUF, "%s!%s@%s", user->nick, user->ident, user->GetIPString());
				for (modelist::iterator it = list->begin(); it != list->end(); it++)
				{
					if(match(user->GetFullRealHost(), it->mask.c_str()) || match(user->GetFullHost(), it->mask.c_str()) || (match(mask, it->mask.c_str(), true)))
					{
						// They match an entry on the list, so let them in.
						return 1;
					}
				}
			}
			// or if there wasn't a list, there can't be anyone on it, so we don't need to do anything.
		}

		return 0;		
	}

	virtual const char* OnRequest(Request* request)
	{
		ListModeRequest* LM = (ListModeRequest*)request;
		if (strcmp("LM_CHECKLIST", request->GetId()) == 0)
		{
			modelist* list;
			LM->chan->GetExt(ie->GetInfoKey(), list);
			if (list)
			{
				char mask[MAXBUF];
				snprintf(mask, MAXBUF, "%s!%s@%s", LM->user->nick, LM->user->ident, LM->user->GetIPString());
				for (modelist::iterator it = list->begin(); it != list->end(); it++)
				{
					if (match(LM->user->GetFullRealHost(), it->mask.c_str()) || match(LM->user->GetFullHost(), it->mask.c_str()) || (match(mask, it->mask.c_str(), true)))
					{
						// They match an entry
						return (char*)it->mask.c_str();
					}
				}
				return NULL;
			}
		}
		return NULL;
	}

	virtual void OnCleanup(int target_type, void* item)
	{
		ie->DoCleanup(target_type, item);
	}

	virtual void OnSyncChannel(Channel* chan, Module* proto, void* opaque)
	{
		ie->DoSyncChannel(chan, proto, opaque);
	}

	virtual void OnChannelDelete(Channel* chan)
	{
		ie->DoChannelDelete(chan);
	}

	virtual void OnRehash(User* user, const std::string &param)
	{
		ie->DoRehash();
	}
		
	virtual Version GetVersion()
	{
		return Version(1, 2, 0, 3, VF_VENDOR | VF_COMMON, API_VERSION);
	}

	~ModuleInviteException()
	{
		ServerInstance->Modes->DelMode(ie);
		delete ie;
		ServerInstance->Modules->UnpublishInterface("ChannelBanList", this);
	}
};

MODULE_INIT(ModuleInviteException)
