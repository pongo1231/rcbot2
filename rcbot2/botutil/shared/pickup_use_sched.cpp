#include "pickup_use_sched.h"

#include "botutil/hl2dm/use_button_task.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/shared/move_to_task.h"

CBotPickupSchedUse::CBotPickupSchedUse(edict_t *pEdict)
{
	addTask(new CFindPathTask(pEdict));
	addTask(new CMoveToTask(pEdict));
	addTask(new CBotHL2DMUseButton(pEdict));
}

void CBotPickupSchedUse ::init()
{
	setID(SCHED_PICKUP);
}