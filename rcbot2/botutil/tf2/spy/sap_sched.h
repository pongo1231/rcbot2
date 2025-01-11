#pragma once

#include "bot_fortress.h"
#include "botutil/base_sched.h"

class CBotSpySapBuildingSched : public CBotSchedule
{
  public:
	CBotSpySapBuildingSched(edict_t *pBuilding, eEngiBuild id);

	void init();
};