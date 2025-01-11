#pragma once

#include "botutil/base_sched.h"

class CBotTF2AttackSentryGun : public CBotSchedule
{
  public:
	CBotTF2AttackSentryGun(edict_t *pSentry, CBotWeapon *pWeapon);

	void init()
	{
		setID(SCHED_ATTACK_SENTRY_GUN);
	}
};