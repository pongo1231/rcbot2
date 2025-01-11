#include "wait_for_grenade_task.h"

#include "bot_dod_bot.h"
#include "bot_globals.h"

void CDODWaitForGrenadeTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{

	if (m_pGrenade.get() == nullptr)
	{
		complete();
	}
	else if (!CBotGlobals::entityIsAlive(m_pGrenade))
		complete();
	else if (m_fTime == 0)
	{
		CDODBot *pDODBot = (CDODBot *)pBot;

		m_fTime          = engine->Time() + randomFloat(3.0f, 5.0f);

		pDODBot->prone();
	}
	else if (m_fTime < engine->Time())
	{
		CDODBot *pDODBot = (CDODBot *)pBot;

		complete();

		pDODBot->unProne();
	}
}

void CDODWaitForGrenadeTask ::debugString(char *string)
{
	sprintf(string, "CDODWaitForGrenadeTask");
}