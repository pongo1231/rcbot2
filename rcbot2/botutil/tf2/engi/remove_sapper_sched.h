#pragma once

#include "bot_fortress.h"
#include "botutil/base_sched.h"

class CBotRemoveSapperSched : public CBotSchedule
{
  public:
	CBotRemoveSapperSched(edict_t *pBuilding, eEngiBuild id);

	void init();
};