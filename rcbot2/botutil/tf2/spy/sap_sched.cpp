#include "sap_sched.h"

#include "bot_globals.h"
#include "bot_waypoint_locations.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/spy/sap_task.h"

void CBotSpySapBuildingSched ::init()
{
	setID(SCHED_SPY_SAP_BUILDING);
}

CBotSpySapBuildingSched ::CBotSpySapBuildingSched(edict_t *pBuilding, eEngiBuild id)
{
	CFindPathTask *findpath = new CFindPathTask(pBuilding);

	addTask(findpath);                         // first
	addTask(new CBotTF2SpySap(pBuilding, id)); // second

	findpath->setDangerPoint(CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(pBuilding), 200.0f, -1));
}