#pragma once

#include "botutil/base_sched.h"

class CBotGetMetalSched : public CBotSchedule
{
  public:
	CBotGetMetalSched(Vector vOrigin);

	void init();
};