#include "backstap_sched.h"

#include "bot_globals.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/spy/backstab_task.h"

CBotBackstabSched::CBotBackstabSched(edict_t *pEnemy)
{
	Vector vrear;
	Vector vangles;

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy), &vangles);
	vrear = CBotGlobals::entityOrigin(pEnemy) - (vangles * 45) + Vector(0, 0, 32);

	// Start approach from a flank side to avoid walking straight at the enemy
	Vector vRight = vangles.Cross(Vector(0, 0, 1));
	if (vRight.Length() > 0.1f)
	{
		vRight = vRight / vRight.Length();
		int iSide      = ((int)(engine->Time() * 0.7f)) % 2;
		float fSideDir = iSide ? 1.0f : -1.0f;
		vrear          = vrear + (vRight * fSideDir * 70.0f);
	}

	addTask(new CFindPathTask(vrear));
	addTask(new CBotBackstab(pEnemy));
}

void CBotBackstabSched::init()
{
	setID(SCHED_BACKSTAB);
}