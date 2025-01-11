#pragma once

#include "botutil/base_sched.h"

class CBotTF2HealSched : public CBotSchedule
{
  public:
	CBotTF2HealSched(edict_t *pHeal);
	void init();
};