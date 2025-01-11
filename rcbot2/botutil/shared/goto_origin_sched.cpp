#include "goto_origin_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/shared/move_to_task.h"

CBotGotoOriginSched ::CBotGotoOriginSched(Vector vOrigin)
{
	addTask(new CFindPathTask(vOrigin)); // first
	addTask(new CMoveToTask(vOrigin));   // second
}

CBotGotoOriginSched ::CBotGotoOriginSched(edict_t *pEdict)
{
	addTask(new CFindPathTask(pEdict));
	addTask(new CMoveToTask(pEdict));
}

void CBotGotoOriginSched ::init()
{
	setID(SCHED_GOTO_ORIGIN);
}