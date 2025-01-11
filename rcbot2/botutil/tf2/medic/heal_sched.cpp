#include "heal_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/medic/heal_task.h"

CBotTF2HealSched::CBotTF2HealSched(edict_t *pHeal)
{
	CFindPathTask *findpath = new CFindPathTask(pHeal);
	findpath->setCompleteInterrupt(CONDITION_SEE_HEAL);
	addTask(findpath);
	addTask(new CBotTF2MedicHeal());
}

void CBotTF2HealSched::init()
{
	setID(SCHED_HEAL);
}