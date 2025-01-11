#pragma once

#include "botutil/base_sched.h"

class CBotBackstabSched : public CBotSchedule
{
  public:
	CBotBackstabSched(edict_t *pEnemy);

	void init();
};