#include "build_building_sched.h"

#include "bot_waypoint.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/build_building_task.h"
#include "botutil/tf2/engi/interrupt.h"

CBotTFEngiBuild::CBotTFEngiBuild(CBot *pBot, eEngiBuild iObject, CWaypoint *pWaypoint)
{
	CFindPathTask *pathtask = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
	addTask(pathtask); // first

	pathtask->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));

	addTask(new CBotTFEngiBuildTask(iObject, pWaypoint)); // second
}

CBotTFEngiBuild::CBotTFEngiBuild(CBot *pBot, eEngiBuild iObject, Vector vOrigin, float fAimYaw, int iArea)
{
	// Navmesh-computed build spot: pathfind to the vector, then build at it.
	CFindPathTask *pathtask = new CFindPathTask(vOrigin);
	pathtask->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));
	addTask(pathtask); // first

	addTask(new CBotTFEngiBuildTask(iObject, vOrigin, fAimYaw, iArea)); // second
}

void CBotTFEngiBuild::init()
{
	setID(SCHED_TF_BUILD);
}