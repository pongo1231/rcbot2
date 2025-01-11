#include "move_building_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/pickup_building_task.h"
#include "botutil/tf2/engi/place_building_task.h"

CBotEngiMoveBuilding ::CBotEngiMoveBuilding(edict_t *pBotEdict, edict_t *pBuilding, eEngiBuild iObject,
                                            Vector vNewLocation, bool bCarrying)
{
	// not carrying
	if (!bCarrying)
	{
		addTask(new CFindPathTask(pBuilding));
		addTask(new CBotTaskEngiPickupBuilding(pBuilding));
	}
	// otherwise already carrying

	addTask(new CFindPathTask(vNewLocation));
	addTask(new CBotTaskEngiPlaceBuilding(iObject, vNewLocation));
}

void CBotEngiMoveBuilding ::init()
{
	setID(SCHED_TF2_ENGI_MOVE_BUILDING);
}