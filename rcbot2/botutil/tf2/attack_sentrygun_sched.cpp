#include "attack_sentrygun_sched.h"

#include "bot_fortress.h"
#include "bot_weapons.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/attack_sentrygun_task.h"

CBotTF2AttackSentryGun::CBotTF2AttackSentryGun(edict_t *pSentry, CBotWeapon *pWeapon)
{
	CFindPathTask *path = new CFindPathTask(pSentry);

	addTask(path);
	addTask(new CBotTF2AttackSentryGunTask(pSentry, pWeapon));

	path->completeInRangeFromEdict();

	if (pWeapon && pWeapon->hasWeapon())
	{
		if (pWeapon->primaryMaxRange() > TF2_MAX_SENTRYGUN_RANGE + 100)
		{
			if (pWeapon->primaryMaxRange() > 4000.0f)
				path->setRange(pWeapon->primaryMaxRange() - 100.0f);
			else
				path->setRange(fmin(pWeapon->primaryMaxRange() - 100.0f,
				                    (float)TF2_MAX_SENTRYGUN_RANGE + 128.0f));
			path->dontGoToEdict();
		}
		else
		{
			path->setRange(pWeapon->primaryMaxRange() - 100);
			path->completeIfSeeTaskEdict();
		}
	}
}