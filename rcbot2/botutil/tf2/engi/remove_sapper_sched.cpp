#include "remove_sapper_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/remove_sapper_task.h"

CBotRemoveSapperSched ::CBotRemoveSapperSched(edict_t *pBuilding, eEngiBuild id)
{
	CFindPathTask *pathtask = new CFindPathTask(pBuilding);
	addTask(pathtask);
	pathtask->completeInRangeFromEdict();
	pathtask->setRange(150.0f);
	addTask(new CBotRemoveSapper(pBuilding, id));
}

void CBotRemoveSapperSched ::init()
{
	setID(SCHED_REMOVESAPPER);
}