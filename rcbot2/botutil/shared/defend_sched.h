#pragma once

#include "botutil/base_sched.h"

class CBotDefendSched : public CBotSchedule
{
  public:
	CBotDefendSched(Vector vOrigin, float fMaxTime = 0);

	CBotDefendSched(int iWaypointID, float fMaxTime = 0);

	void init();
};