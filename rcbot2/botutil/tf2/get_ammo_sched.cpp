#include "get_ammo_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/wait_ammo_task.h"

CBotTF2GetAmmoSched ::CBotTF2GetAmmoSched(Vector vOrigin)
{
	CFindPathTask *task1       = new CFindPathTask(vOrigin);
	CBotTF2WaitAmmoTask *task2 = new CBotTF2WaitAmmoTask(vOrigin);

	// if bot doesn't have need ammo flag anymore ....
	// fail so that the bot doesn't move onto the next task
	task1->setCompleteInterrupt(0, CONDITION_NEED_AMMO);
	task2->setCompleteInterrupt(0, CONDITION_NEED_AMMO);

	addTask(task1); // first
	addTask(task2); // second
}

void CBotTF2GetAmmoSched ::init()
{
	setID(SCHED_TF2_GET_AMMO);
}