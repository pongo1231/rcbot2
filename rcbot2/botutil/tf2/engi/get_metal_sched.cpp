#include "get_metal_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/wait_ammo_task.h"

CBotGetMetalSched ::CBotGetMetalSched(Vector vOrigin)
{
	CFindPathTask *task1       = new CFindPathTask(vOrigin);
	CBotTF2WaitAmmoTask *task2 = new CBotTF2WaitAmmoTask(vOrigin);

	task1->setCompleteInterrupt(0, CONDITION_NEED_AMMO);
	task2->setCompleteInterrupt(0, CONDITION_NEED_AMMO);

	addTask(task1); // first
	addTask(task2);
}

void CBotGetMetalSched ::init()
{
	setID(SCHED_GET_METAL);
}