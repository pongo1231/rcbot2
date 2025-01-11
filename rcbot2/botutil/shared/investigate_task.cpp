#include "investigate_task.h"

#include "bot_getprop.h"
#include "bot_waypoint_locations.h"

void CBotInvestigateTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fTime == 0)
	{
		CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(m_vOrigin, m_fRadius, -1));

		if (pWaypoint == nullptr)
		{
			// can't investigate
			// but other tasks depend on this, so complete it
			complete();
			return;
		}

		if (pWaypoint->numPaths() > 0)
		{
			for (int i = 0; i < pWaypoint->numPaths(); i++)
				m_InvPoints.push_back(CWaypoints::getWaypoint(pWaypoint->getPath(i))->getOrigin());

			m_iCurPath = randomInt(0, pWaypoint->numPaths() - 1);
		}

		m_fTime = engine->Time() + m_fMaxTime;
	}

	if (m_fTime < engine->Time())
		complete();

	if (m_InvPoints.size() > 0)
	{
		Vector vPoint;

		if (m_iState == 0) // goto inv point
			vPoint = m_InvPoints[m_iCurPath];
		else if (m_iState == 1) // goto origin
			vPoint = m_vOrigin;

		if ((pBot->distanceFrom(vPoint) < 80) || ((m_iState == 0) && (pBot->distanceFrom(m_vOrigin) > m_fRadius)))
		{
			m_iState = (!m_iState) ? 1 : 0;

			if (m_iState == 0)
				m_iCurPath = randomInt(0, m_InvPoints.size() - 1);
		}
		else
			pBot->setMoveTo(vPoint);
	}
	else
		pBot->stopMoving();
	// walk
	pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict()) / 8);
	// pBot->setLookVector();

	if (m_bHasPOV)
	{
		pBot->setLookVector(m_vPOV);
		pBot->setLookAtTask(LOOK_VECTOR);
	}
	else
		pBot->setLookAtTask(LOOK_AROUND);
}