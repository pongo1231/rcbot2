#include "build_building_sched.h"

#include "bot_waypoint.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/build_building_task.h"
#include "botutil/tf2/engi/interrupt.h"

CBotTFEngiBuild ::CBotTFEngiBuild(CBot *pBot, eEngiBuild iObject, CWaypoint *pWaypoint)
{
	CFindPathTask *pathtask = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
	addTask(pathtask); // first

	pathtask->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));

	addTask(new CBotTFEngiBuildTask(iObject, pWaypoint)); // second
}

void CBotTFEngiBuild ::init()
{
	setID(SCHED_TF_BUILD);
}