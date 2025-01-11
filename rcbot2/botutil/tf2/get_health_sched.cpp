#include "get_health_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/wait_health_task.h"

CBotTF2GetHealthSched ::CBotTF2GetHealthSched(Vector vOrigin)
{
	CFindPathTask *task1         = new CFindPathTask(vOrigin);
	CBotTF2WaitHealthTask *task2 = new CBotTF2WaitHealthTask(vOrigin);

	// if bot doesn't have need ammo flag anymore ....
	// fail so that the bot doesn't move onto the next task
	task1->setCompleteInterrupt(0, CONDITION_NEED_HEALTH);
	task2->setCompleteInterrupt(0, CONDITION_NEED_HEALTH);

	addTask(task1); // first
	addTask(task2); // second
}

void CBotTF2GetHealthSched ::init()
{
	setID(SCHED_TF2_GET_HEALTH);
}