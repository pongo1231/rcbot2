#pragma once

#include "botutil/base_sched.h"

class CBotTF2ShootLastEnemyPos : public CBotSchedule
{
  public:
	CBotTF2ShootLastEnemyPos(Vector vLastSeeEnemyPos, Vector vVelocity, edict_t *pLastEnemy);

	void init();
};