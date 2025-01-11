#include "use_teleporter_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_navigator.h"

CBotTFUseTeleporter ::CBotTFUseTeleporter(edict_t *pTele)
{ // going to use this

	m_pTele = pTele;
	m_fTime = 0.0;
}

void CBotTFUseTeleporter ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_pTele || !CBotGlobals::entityIsValid(m_pTele))
	{
		fail();
		return;
	}

	if (!pBot->isTF())
	{
		if (((CBotFortress *)pBot)->hasFlag())
		{
			fail();
			return;
		}
	}

	if (!m_fTime)
	{
		// initialize
		m_fTime       = engine->Time() + 13.0f;
		m_vLastOrigin = pBot->getOrigin();
	}

	// FIX BUG
	// if ( !((CBotFortress*)pBot)->isTeleporterUseful(m_pTele) )
	//	fail();

	if (m_fTime < engine->Time())
	{
		if (CClients::clientsDebugging(BOT_DEBUG_TASK))
			CClients::clientDebugMsg(BOT_DEBUG_TASK, "TELEPORT: TIMEOUT", pBot);

		fail();
	}
	else
	{
		if (CTeamFortress2Mod::getTeleporterExit(m_pTele)) // exit is still alive?
		{
			Vector vTele = CBotGlobals::entityOrigin(m_pTele);

			if ((pBot->distanceFrom(vTele) > 48)
			    || (CClassInterface::getGroundEntity(pBot->getEdict()) != m_pTele.get()))
			{
				pBot->setMoveTo((vTele));

				if ((m_vLastOrigin - pBot->getOrigin()).Length() > 50)
				{
					pBot->getNavigator()->freeMapMemory(); // restart navigator

					complete(); // finished
				}
			}
			else
			{
				pBot->stopMoving();
			}

			m_vLastOrigin = pBot->getOrigin();
		}
		else
			fail();
	}
}

void CBotTFUseTeleporter ::debugString(char *string)
{
	sprintf(string, "CBotTFUseTeleporter\nm_pTele = %x", (int)m_pTele.get());
}