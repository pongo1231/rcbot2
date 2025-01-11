#pragma once

#include "botutil/base_sched.h"

class CBotTF2FindFlagSched : public CBotSchedule
{
  public:
	CBotTF2FindFlagSched(Vector vOrigin);

	void init();
};