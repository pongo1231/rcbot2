#include "defend_sched.h"

#include "bot_waypoint.h"
#include "botutil/shared/defend_task.h"
#include "botutil/shared/find_path_task.h"

CBotDefendSched ::CBotDefendSched(Vector vOrigin, float fMaxTime)
{
	addTask(new CFindPathTask(vOrigin));
	addTask(new CBotDefendTask(vOrigin, fMaxTime));
}

CBotDefendSched::CBotDefendSched(int iWaypointID, float fMaxTime)
{
	CWaypoint *pWaypoint;

	pWaypoint = CWaypoints::getWaypoint(iWaypointID);

	addTask(new CFindPathTask(iWaypointID));
	addTask(new CBotDefendTask(pWaypoint->getOrigin(), fMaxTime, 8, false, Vector(0, 0, 0), LOOK_SNIPE,
	                           pWaypoint->getFlags()));
}

void CBotDefendSched ::init()
{
	setID(SCHED_DEFEND);
}