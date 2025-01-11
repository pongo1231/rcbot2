#pragma once

#include "bot_fortress.h"
#include "botutil/base_sched.h"

class CBotTF2DemoPipeTrapSched : public CBotSchedule
{
  public:
	CBotTF2DemoPipeTrapSched(eDemoTrapType type, Vector vStand, Vector vLoc, Vector vSpread, bool bAutoDetonate = false,
	                         int wptarea = -1);

	void init();
};