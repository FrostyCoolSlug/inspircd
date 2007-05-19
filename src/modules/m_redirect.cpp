/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2007 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "users.h"
#include "channels.h"
#include "modules.h"
#include "inspircd.h"

/* $ModDesc: Provides channel mode +L (limit redirection) */

/** Handle channel mode +L
 */
class Redirect : public ModeHandler
{
 public:
	Redirect(InspIRCd* Instance) : ModeHandler(Instance, 'L', 1, 0, false, MODETYPE_CHANNEL, false) { }

	ModePair ModeSet(userrec* source, userrec* dest, chanrec* channel, const std::string &parameter)
	{
		if (channel->IsModeSet('L'))
			return std::make_pair(true, channel->GetModeParameter('L'));
		else
			return std::make_pair(false, parameter);
	}

	bool CheckTimeStamp(time_t theirs, time_t ours, const std::string &their_param, const std::string &our_param, chanrec* channel)
	{
		/* When TS is equal, the alphabetically later one wins */
		return (their_param < our_param);
	}
	
	ModeAction OnModeChange(userrec* source, userrec* dest, chanrec* channel, std::string &parameter, bool adding)
	{
		if (adding)
		{
			chanrec* c = NULL;

			if (!ServerInstance->IsChannel(parameter.c_str()))
			{
				source->WriteServ("403 %s %s :Invalid channel name",source->nick, parameter.c_str());
				parameter = "";
				return MODEACTION_DENY;
			}

			c = ServerInstance->FindChan(parameter);
			if (c)
			{
				/* Fix by brain: Dont let a channel be linked to *itself* either */
				if (IS_LOCAL(source))
				{
					if ((c == channel) || (c->IsModeSet('L')))
					{
						source->WriteServ("690 %s :Circular or chained +L to %s not allowed (Channel already has +L). Pack of wild dogs has been unleashed.",source->nick,parameter.c_str());
						parameter = "";
						return MODEACTION_DENY;
					}
					else
					{
						for (chan_hash::const_iterator i = ServerInstance->chanlist->begin(); i != ServerInstance->chanlist->end(); i++)
						{
							if ((i->second != channel) && (i->second->IsModeSet('L')) && (irc::string(i->second->GetModeParameter('L').c_str()) == irc::string(channel->name)))
							{
								source->WriteServ("690 %s :Circular or chained +L to %s not allowed (Already forwarded here from %s). Angry monkeys dispatched.",source->nick,parameter.c_str(),i->second->name);
								return MODEACTION_DENY;
							}
						}
					}
				}
			}

			channel->SetMode('L', true);
			channel->SetModeParam('L', parameter.c_str(), true);
			return MODEACTION_ALLOW;
		}
		else
		{
			if (channel->IsModeSet('L'))
			{
				channel->SetMode('L', false);
				return MODEACTION_ALLOW;
			}
		}

		return MODEACTION_DENY;
		
	}
};

class ModuleRedirect : public Module
{
	
	Redirect* re;
	
 public:
 
	ModuleRedirect(InspIRCd* Me)
		: Module(Me)
	{
		
		re = new Redirect(ServerInstance);
		if (!ServerInstance->AddMode(re, 'L'))
			throw ModuleException("Could not add new modes!");
	}
	
	void Implements(char* List)
	{
		List[I_OnUserPreJoin] = 1;
	}

	virtual int OnUserPreJoin(userrec* user, chanrec* chan, const char* cname, std::string &privs)
	{
		if (chan)
		{
			if (chan->IsModeSet('L') && chan->limit)
			{
				if (chan->GetUserCounter() >= chan->limit)
				{
					std::string channel = chan->GetModeParameter('L');

					/* sometimes broken ulines can make circular or chained +L, avoid this */
					chanrec* destchan = NULL;
					destchan = ServerInstance->FindChan(channel);
					if (destchan && destchan->IsModeSet('L'))
					{
						user->WriteServ("470 %s :%s is full, but has a circular redirect (+L), not following redirection to %s", user->nick, cname, channel.c_str());
						return 1;
					}

					user->WriteServ("470 %s :%s has become full, so you are automatically being transferred to the linked channel %s", user->nick, cname, channel.c_str());
					chanrec::JoinUser(ServerInstance, user, channel.c_str(), false, "", ServerInstance->Time(true));
					return 1;
				}
			}
		}
		return 0;
	}

	virtual ~ModuleRedirect()
	{
		ServerInstance->Modes->DelMode(re);
		DELETE(re);
	}
	
	virtual Version GetVersion()
	{
		return Version(1, 1, 0, 0, VF_COMMON | VF_VENDOR, API_VERSION);
	}
};


class ModuleRedirectFactory : public ModuleFactory
{
 public:
	ModuleRedirectFactory()
	{
	}
	
	~ModuleRedirectFactory()
	{
	}
	
	virtual Module * CreateModule(InspIRCd* Me)
	{
		return new ModuleRedirect(Me);
	}
	
};


extern "C" DllExport void * init_module( void )
{
	return new ModuleRedirectFactory;
}

