#pragma once

#include "botutil/base_sched.h"

class CBotTFEngiUpgrade : public CBotSchedule
{
  public:
	CBotTFEngiUpgrade(CBot *pBot, edict_t *pBuilding);

	void init();
};