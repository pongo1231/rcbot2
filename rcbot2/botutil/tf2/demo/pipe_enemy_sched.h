#pragma once

#include "botutil/base_sched.h"

class CBotTF2DemoPipeEnemySched : public CBotSchedule
{
  public:
	CBotTF2DemoPipeEnemySched(CBotWeapon *pLauncher, Vector vStand, edict_t *pEnemy);

	void init();
};