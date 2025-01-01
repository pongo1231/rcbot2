#ifndef __BOT_EXT_SOURCEMOD_H__
#define __BOT_EXT_SOURCEMOD_H__

#include <IExtensionSys.h>
#include <smsdk_config.h>

#include "bot_plugin_meta.h"

using SourceMod::IExtension;
using SourceMod::IExtensionManager;
using SourceMod::IShareSys;

class RCBotSourceModExt : public SourceMod::IExtensionInterface
{
  public:
	virtual bool OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t maxlength, bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();
	virtual void OnExtensionPauseChange(bool pause);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool IsMetamodExtension();
	virtual const char *GetExtensionName();
	virtual const char *GetExtensionURL();
	virtual const char *GetExtensionTag();
	virtual const char *GetExtensionAuthor();
	virtual const char *GetExtensionVerString();
	virtual const char *GetExtensionDescription();
	virtual const char *GetExtensionDateString();
};

bool SM_AcquireInterfaces(char *error, size_t maxlength);
bool SM_LoadExtension(char *error, size_t maxlength);
void SM_UnloadExtension();
void SM_UnsetInterfaces();

extern RCBotSourceModExt g_RCBotSourceMod;

extern SourceMod::IExtensionManager *smexts;

extern SourceMod::IShareSys *sharesys;
extern SourceMod::IExtension *myself;

#endif
