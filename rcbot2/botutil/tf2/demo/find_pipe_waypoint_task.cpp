#include "find_pipe_waypoint_task.h"

#include "bot.h"
#include "bot_mods.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint_visibility.h"
#include "botutil/base_sched.h"

#include <vector.h>

CBotTF2FindPipeWaypoint::CBotTF2FindPipeWaypoint(Vector vOrigin, Vector vTarget)
{
	m_vOrigin         = vOrigin;
	m_vTarget         = vTarget;

	m_i               = 0;
	m_j               = 0;
	m_iters           = 0;
	m_iNearesti       = -1;
	m_iNearestj       = -1;
	m_fNearesti       = 2048.0f;
	m_fNearestj       = 4096.0f;
	m_iTargetWaypoint = (short int)CWaypointLocations::NearestWaypoint(m_vTarget, BLAST_RADIUS, -1, true, true);

	m_pTable          = CWaypoints::getVisiblity();

	if (m_iTargetWaypoint != -1)
	{
		// first find the waypoint nearest the target
		Vector vComp = (vOrigin - vTarget);
		vComp        = vComp / vComp.Length();
		vComp        = vTarget + vComp * 256; // get into a better area
		m_pTarget    = CWaypoints::getWaypoint(m_iTargetWaypoint);
		CWaypointLocations::GetAllInArea(vComp, &m_WaypointsI, m_iTargetWaypoint);
	}
}
// a concurrentish pipe waypoint search
void CBotTF2FindPipeWaypoint ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CWaypoint *pTempi, *pTempj;
	float fidist, fjdist;
	int maxiters = 128;

	// push!
	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
		fail();
	else if (CTeamFortress2Mod::TF2_IsPlayerKrits(pBot->getEdict()))
		maxiters = 256; // speed up search

	pBot->setLookAtTask(LOOK_AROUND);
	pBot->stopMoving();

	if (m_pTarget == nullptr)
	{
		fail();
		return;
	}

	m_iters = 0;

	// loop through every visible waypoint to target (can be unreachable)
	while ((m_i < m_WaypointsI.size()) && (m_iters < maxiters))
	{

		pTempi = CWaypoints::getWaypoint(m_WaypointsI[m_i]);
		fidist = m_pTarget->distanceFrom(pTempi->getOrigin());

		if (fidist > m_fNearesti)
		{
			m_i++;
			m_iters++;
			continue;
		}

		Vector vTempi = pTempi->getOrigin();

		CWaypointLocations::GetAllInArea(vTempi, &m_WaypointsJ, m_WaypointsI[m_i]);

		// loop through every visible waypoint to intermediatery waypoint (cannot be unreachable)
		while ((m_j < m_WaypointsJ.size()) && (m_iters < maxiters))
		{
			if (m_WaypointsJ[m_j] == m_iTargetWaypoint)
			{
				m_j++;
				continue;
			}

			pTempj = CWaypoints::getWaypoint(m_WaypointsJ[m_j]);

			// must be reachable as I want to go here
			if (!pBot->canGotoWaypoint(pTempj->getOrigin(), pTempj))
			{
				m_j++;
				continue;
			}

			// only remember the nearest waypoints - the nearest is updated when both I and J are found
			fjdist = pTempj->distanceFrom(m_vOrigin) + pTempj->distanceFrom(pTempi->getOrigin());

			if (fjdist > m_fNearestj)
			{
				m_j++;
				m_iters++;
				continue;
			}

			// If this waypoint is NOT visible to target it is good
			if (!m_pTable->GetVisibilityFromTo((int)m_iTargetWaypoint, (int)m_WaypointsJ[m_j]))
			{
				m_fNearesti = fidist;
				m_fNearestj = fjdist;
				m_iNearesti = m_WaypointsI[m_i];
				m_iNearestj = m_WaypointsJ[m_j];
			}

			m_iters++;
			m_j++;
		}

		if (m_j == m_WaypointsJ.size())
			m_i++;
	}

	if (m_i == m_WaypointsI.size())
	{
		if (m_iNearesti == -1)
			fail();
		else
		{
			pSchedule->passInt(m_iNearestj);
			pSchedule->passVector(CWaypoints::getWaypoint(m_iNearesti)->getOrigin());

			complete();
		}
	}
}