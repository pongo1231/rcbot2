#include "use_teleporter_task.h"

#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_navigator.h"

CBotTFUseTeleporter::CBotTFUseTeleporter(edict_t *pTele)
{
	m_pTele         = pTele;
	m_fTime         = 0.0;
	m_iState        = 0;  // TELE_APPROACH
	m_fStateEval    = 0.0f;
}

void CBotTFUseTeleporter::execute(CBot *pBot, CBotSchedule *pSchedule)
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
		m_fTime       = engine->Time() + 13.0f;
		m_vLastOrigin = pBot->getOrigin();
	}

	if (m_fTime < engine->Time())
	{
		if (CClients::clientsDebugging(BOT_DEBUG_TASK))
			CClients::clientDebugMsg(BOT_DEBUG_TASK, "TELEPORT: TIMEOUT", pBot);

		fail();
		return;
	}

	if (!CTeamFortress2Mod::getTeleporterExit(m_pTele))
	{
		fail();
		return;
	}

	Vector vTele  = CBotGlobals::entityOrigin(m_pTele.get());
	float fDist   = pBot->distanceFrom(vTele);

	switch (m_iState)
	{
	case 0: // TELE_APPROACH — walk toward entrance, check for other users
	{
		if ((fDist <= 48)
		    && (CClassInterface::getGroundEntity(pBot->getEdict()) == m_pTele.get()))
		{
			// We're on the entrance — wait for teleport
			pBot->stopMoving();
			m_iState = 2; // TELE_USING
		}
		else
		{
			// Check if another teammate is using the entrance
			int users = CBotGlobals::countTeamMatesNearOrigin(vTele, 50.0f,
			    pBot->getTeam(), pBot->getEdict());

			if (users > 0 && fDist < 120.0f)
			{
				// Someone else is on it — stage nearby
				m_iState     = 1; // TELE_WAIT
				m_fStateEval = engine->Time() + 2.0f;
			}
			else
			{
				pBot->setMoveTo(vTele);
			}
		}
		break;
	}

	case 1: // TELE_WAIT — stand nearby, re-evaluate every 2s
	{
		if (m_fStateEval < engine->Time())
		{
			m_fStateEval = engine->Time() + 2.0f;

			int users = CBotGlobals::countTeamMatesNearOrigin(vTele, 50.0f,
			    pBot->getTeam(), pBot->getEdict());

			if (users == 0)
			{
				// Entrance is clear — go use it
				m_iState = 0; // back to TELE_APPROACH
				break;
			}

			// Re-evaluate: is walking faster than waiting?
			CBotFortress *pFBot = (CBotFortress *)pBot;
			float fWait, fRun;
			if (!pFBot->teleporterWalkVsWaitTime(m_pTele.get(), &fWait, &fRun))
			{
				// Walking is faster — abandon teleporter
				fail();
				return;
			}
		}

		// Move to staging position 80 units from entrance
		Vector vAway = pBot->getOrigin() - vTele;
		vAway.z = 0;
		vAway = (vAway.Length() > 0.1f) ? vAway / vAway.Length() : Vector(1, 0, 0);
		Vector vStaging = vTele + vAway * 80.0f;

		if (pBot->distanceFrom(vStaging) > 16.0f)
			pBot->setMoveTo(vStaging);
		else
			pBot->stopMoving();
		break;
	}

	case 2: // TELE_USING — standing on entrance, waiting for teleport
	{
		if ((fDist > 48)
		    || (CClassInterface::getGroundEntity(pBot->getEdict()) != m_pTele.get()))
		{
			// Slipped off — go back to approach
			m_iState = 0;
			break;
		}

		pBot->stopMoving();

		// Detect teleport success: origin changed significantly
		if ((m_vLastOrigin - pBot->getOrigin()).Length() > 50)
		{
		pBot->getNavigator()->freeMapMemory();

		// Clear queue state
		((CBotFortress *)pBot)->clearTeleporterQueueState();

		complete();
		}
		break;
	}
	}

	m_vLastOrigin = pBot->getOrigin();
}

void CBotTFUseTeleporter::debugString(char *string)
{
	sprintf(string, "CBotTFUseTeleporter\nm_pTele = %x", (int)m_pTele.get());
}
