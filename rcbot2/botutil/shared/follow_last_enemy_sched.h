#pragma once

#include "botutil/base_sched.h"

class CBotFollowLastEnemy : public CBotSchedule
{
  public:
	CBotFollowLastEnemy(CBot *pBot, edict_t *pEnemy, Vector vLastSee);

	void init();
};