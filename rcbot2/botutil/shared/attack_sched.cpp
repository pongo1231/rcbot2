#include "attack_sched.h"

#include "botutil/shared/attack_entity_task.h"

CBotAttackSched ::CBotAttackSched(edict_t *pEdict)
{
	addTask(new CAttackEntityTask(pEdict));
	// addTask(new CFindGoodHideSpot(pEdict));
}

void CBotAttackSched ::init()
{
	setID(SCHED_ATTACK);
}