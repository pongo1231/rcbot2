#include "pickup_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/shared/move_to_task.h"

CBotPickupSched::CBotPickupSched(edict_t *pEdict)
{
	addTask(new CFindPathTask(pEdict));
	addTask(new CMoveToTask(pEdict));
}

void CBotPickupSched ::init()
{
	setID(SCHED_PICKUP);
}