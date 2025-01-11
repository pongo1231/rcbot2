#include "wait_for_bomb_task.h"

#include "bot_getprop.h"
#include "bot_waypoint_locations.h"

void CDODWaitForBombTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fTime == 0.0f)
	{
		CWaypoint *pCurrent;
		pBot->updateCondition(CONDITION_RUN);
		pCurrent = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
		    pBot->getOrigin(), 400.0f, CWaypoints::getWaypointIndex(m_pBlocking), true, false, true));

		if (pCurrent == nullptr)
			pCurrent = m_pBlocking;

		m_fTime  = engine->Time() + randomFloat(2.0f, 5.0f);
		m_pRunTo = CWaypoints::getNextCoverPoint(pBot, pCurrent, m_pBlocking);
	}

	if (m_pBombTarget.get() == nullptr)
	{
		complete();
		return;
	}

	if (m_pBombTarget.get()->GetUnknown() == nullptr)
	{
		complete();
		return;
	}

	pBot->updateCondition(CONDITION_RUN);

	if (m_pRunTo)
	{
		if (m_pRunTo->touched(pBot->getOrigin(), Vector(0, 0, 0), 48.0f))
		{
			if (pBot->distanceFrom(m_pBombTarget) > (BLAST_RADIUS * 2))
				pBot->stopMoving();
			else
				m_pRunTo = CWaypoints::getNextCoverPoint(pBot, m_pRunTo, m_pBlocking);
		}
		else
			pBot->setMoveTo(m_pRunTo->getOrigin());
	}

	if (m_fTime < engine->Time())
	{
		complete();
		return;
	}

	if (CClassInterface::getDODBombState(m_pBombTarget) != 2)
	{
		complete();
		return;
	}

	pBot->lookAtEdict(m_pBombTarget);
	pBot->setLookAtTask(LOOK_EDICT);
}

void CDODWaitForBombTask ::debugString(char *string)
{
	sprintf(string, "CDODWaitForBombTask");
}