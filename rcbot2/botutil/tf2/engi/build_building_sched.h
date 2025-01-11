#pragma once

#include "bot_fortress.h"
#include "botutil/base_sched.h"

class CBotTFEngiBuild : public CBotSchedule
{
  public:
	CBotTFEngiBuild(CBot *pBot, eEngiBuild iObject, CWaypoint *pWaypoint);

	void init();
};