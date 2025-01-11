#include "get_flag_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/wait_flag_task.h"

CBotTF2GetFlagSched ::CBotTF2GetFlagSched(Vector vOrigin, bool bUseRoute, Vector vRoute)
{
	if (bUseRoute)
		addTask(new CFindPathTask(vRoute));

	addTask(new CFindPathTask(vOrigin));       // first
	addTask(new CBotTF2WaitFlagTask(vOrigin)); // second
}

void CBotTF2GetFlagSched ::init()
{
	setID(SCHED_TF2_GET_FLAG);
}