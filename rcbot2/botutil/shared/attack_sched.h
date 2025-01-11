#pragma once

#include "botutil/base_sched.h"

class CBotAttackSched : public CBotSchedule
{
  public:
	CBotAttackSched(edict_t *pEdict);

	void init();
};