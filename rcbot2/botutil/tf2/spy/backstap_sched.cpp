#include "backstap_sched.h"

#include "bot_globals.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/spy/backstab_task.h"

CBotBackstabSched ::CBotBackstabSched(edict_t *pEnemy)
{
	Vector vrear;
	Vector vangles;

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy), &vangles);
	vrear = CBotGlobals::entityOrigin(pEnemy) - (vangles * 45) + Vector(0, 0, 32);

	addTask(new CFindPathTask(vrear));
	addTask(new CBotBackstab(pEnemy));
}

void CBotBackstabSched ::init()
{
	setID(SCHED_BACKSTAB);
}