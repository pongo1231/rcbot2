#include "attack_point_sched.h"

#include "bot_globals.h"
#include "bot_waypoint_locations.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/attack_point_task.h"
#include "botutil/tf2/nest_task.h"

CBotAttackPointSched ::CBotAttackPointSched(Vector vPoint, int iRadius, int iArea, bool bHasRoute, Vector vRoute,
                                            bool bNest, edict_t *pLastEnemySentry)
{
	int iDangerWpt = -1;

	if (pLastEnemySentry != nullptr)
		iDangerWpt =
		    CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pLastEnemySentry), 200.0f, -1, true, true);

	// First find random route
	if (bHasRoute)
	{
		CFindPathTask *toRoute = new CFindPathTask(vRoute);
		addTask(toRoute); // first
		toRoute->setDangerPoint(iDangerWpt);

		if (bNest)
			addTask(new CBotNest());
	}

	CFindPathTask *toPoint = new CFindPathTask(vPoint);
	addTask(toPoint); // second / first
	toPoint->setDangerPoint(iDangerWpt);
	addTask(new CBotTF2AttackPoint(iArea, vPoint, iRadius)); // third / second
}

void CBotAttackPointSched ::init()
{
	setID(SCHED_ATTACKPOINT);
}