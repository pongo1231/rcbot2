#include "attack_sentrygun_sched.h"

#include "bot_weapons.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/attack_sentrygun_task.h"

CBotTF2AttackSentryGun::CBotTF2AttackSentryGun(edict_t *pSentry, CBotWeapon *pWeapon)
{
	CFindPathTask *path = new CFindPathTask(pSentry);

	addTask(path);
	addTask(new CBotTF2AttackSentryGunTask(pSentry, pWeapon));

	path->completeInRangeFromEdict();
	path->completeIfSeeTaskEdict();

	path->setRange(pWeapon->primaryMaxRange() - 100);
}