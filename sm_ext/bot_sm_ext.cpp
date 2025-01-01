#include "bot_sm_ext.h"
#include "bot_sm_natives.h"

using namespace SourceMod;

RCBotSourceModExt g_RCBotSourceMod;

bool RCBotSourceModExt::OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t maxlength, bool late)
{
	sharesys = sys;
	myself   = me;

	/* Get the default interfaces from our configured SDK header */
	if (!SM_AcquireInterfaces(error, maxlength))
	{
		return false;
	}

	sharesys->RegisterLibrary(myself, "RCBot2");
	sharesys->AddNatives(myself, g_RCBotNatives);
	return true;
}

void RCBotSourceModExt::OnExtensionUnload()
{
	SM_UnsetInterfaces();
}

void RCBotSourceModExt::OnExtensionsAllLoaded()
{
}

void RCBotSourceModExt::OnExtensionPauseChange(bool pause)
{
}

bool RCBotSourceModExt::QueryRunning(char *error, size_t maxlength)
{
	return true;
}

bool RCBotSourceModExt::IsMetamodExtension()
{
	return false;
}

const char *RCBotSourceModExt::GetExtensionName()
{
	return g_RCBotPluginMeta.GetName();
}
const char *RCBotSourceModExt::GetExtensionURL()
{
	return g_RCBotPluginMeta.GetURL();
}
const char *RCBotSourceModExt::GetExtensionTag()
{
	return g_RCBotPluginMeta.GetLogTag();
}
const char *RCBotSourceModExt::GetExtensionAuthor()
{
	return g_RCBotPluginMeta.GetAuthor();
}
const char *RCBotSourceModExt::GetExtensionVerString()
{
	return g_RCBotPluginMeta.GetVersion();
}
const char *RCBotSourceModExt::GetExtensionDescription()
{
	return g_RCBotPluginMeta.GetDescription();
}
const char *RCBotSourceModExt::GetExtensionDateString()
{
	return g_RCBotPluginMeta.GetDate();
}

bool SM_LoadExtension(char *error, size_t maxlength)
{
	if ((smexts = (IExtensionManager *)g_SMAPI->MetaFactory(SOURCEMOD_INTERFACE_EXTENSIONS, NULL, NULL)) == NULL)
	{
		if (error && maxlength)
		{
			snprintf(error, maxlength, SOURCEMOD_INTERFACE_EXTENSIONS " interface not found");
		}
		return false;
	}

	/* This could be more dynamic */
	char path[256];
	g_SMAPI->PathFormat(path, sizeof(path), "addons/rcbot2/bin/RCBot2Meta%s",
#if defined __linux__
	                    "_i486.so"
#else
	                    ".dll"
#endif
	);

	if ((myself = smexts->LoadExternal(&g_RCBotSourceMod, path, "rcbot2.ext", error, maxlength)) == NULL)
	{
		SM_UnsetInterfaces();
		return false;
	}
	return true;
}

void SM_UnloadExtension()
{
	smexts->UnloadExtension(myself);
}
