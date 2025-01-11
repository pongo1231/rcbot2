#pragma once

#include "botutil/base_sched.h"

class CBotUseDispSched : public CBotSchedule
{
  public:
	CBotUseDispSched(CBot *pBot, edict_t *pDisp);

	void init();
};