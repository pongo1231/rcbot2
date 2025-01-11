#include "investigate_hide_point_task.h"

#include "bot_mtrand.h"
#include "bot_waypoint.h"

CBotInvestigateHidePoint ::CBotInvestigateHidePoint(int iWaypointIndexToInvestigate, int iOriginalWaypointIndex)
{
	CWaypoint *pWaypoint    = CWaypoints::getWaypoint(iWaypointIndexToInvestigate);
	CWaypoint *pOriginalWpt = CWaypoints::getWaypoint(iOriginalWaypointIndex);
	m_vOrigin               = pOriginalWpt->getOrigin();
	m_vMoveTo               = pWaypoint->getOrigin();
	m_fTime                 = 0;
	m_fInvestigateTime      = 0;
	m_iState                = 0;

	for (int i = 0; i < pWaypoint->numPaths(); i++)
	{
		CWaypoint *pWaypointOther = CWaypoints::getWaypoint(pWaypoint->getPath(i));

		if (pWaypointOther == pWaypoint)
			continue;
		if (pWaypointOther == pOriginalWpt)
			continue;

		m_CheckPoints.push_back(pWaypointOther->getOrigin());
	}

	m_iCurrentCheckPoint = 0;
}

void CBotInvestigateHidePoint::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fTime == 0)
	{
		m_fTime            = engine->Time() + randomFloat(6.0f, 8.0f);
		m_fInvestigateTime = randomFloat(0.3f, 1.0f);
	}
	else if (m_fTime < engine->Time())
	{
		if (m_iState == 2)
			complete();
		else if (m_iState != 2) // go back to origin
		{
			m_fTime  = engine->Time() + randomFloat(1.0f, 2.0f);
			m_iState = 2;
		}
	}

	if (m_CheckPoints.size() == 0)
	{
		// don't move, just look
		pBot->setLookVector(m_vMoveTo);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (m_fInvestigateTime < engine->Time())
			complete();
	}
	else
	{

		switch (m_iState)
		{
		case 0: // goto m_vMoveTo
			if (pBot->distanceFrom(m_vMoveTo) > 70)
				pBot->setMoveTo(m_vMoveTo);
			else
			{
				m_iState           = 1;
				m_fInvestigateTime = engine->Time() + randomFloat(0.3f, 1.3f);
			}
			pBot->setLookVector(m_vMoveTo);
			pBot->setLookAtTask(LOOK_VECTOR);
			break;
		case 1:
			pBot->stopMoving();
			if (m_iCurrentCheckPoint < m_CheckPoints.size())
			{
				if (m_fInvestigateTime < engine->Time())
				{
					m_iCurrentCheckPoint++;
					m_fInvestigateTime = engine->Time() + randomFloat(0.3f, 1.3f);
				}
				else
				{
					pBot->setLookVector(m_CheckPoints[m_iCurrentCheckPoint]);
					pBot->setLookAtTask(LOOK_VECTOR);
				}
			}
			else
				m_iState = 2;
			break;
		case 2:
			// go back to origin
			if (pBot->distanceFrom(m_vOrigin) > 70)
				pBot->setMoveTo(m_vOrigin);
			else
			{
				complete();
			}

			pBot->setLookVector(m_vOrigin);
			pBot->setLookAtTask(LOOK_VECTOR);

			break;
		}
	}
}

void CBotInvestigateHidePoint::debugString(char *string)
{
	sprintf(string, "CBotInvestigateHidePoint");
}