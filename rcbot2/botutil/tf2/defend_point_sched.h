#pragma once

#include "botutil/base_sched.h"

class CBotDefendPointSched : public CBotSchedule
{
  public:
	CBotDefendPointSched(Vector vPoint, int iRadius, int iArea);

	void init();
};