#include "wait_flag_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/wait_flag_task.h"

CBotTF2FindFlagSched ::CBotTF2FindFlagSched(Vector vOrigin)
{
	addTask(new CFindPathTask(vOrigin));             // first
	addTask(new CBotTF2WaitFlagTask(vOrigin, true)); // second
}

void CBotTF2FindFlagSched ::init()
{
	setID(SCHED_TF2_FIND_FLAG);
}