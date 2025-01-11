#pragma once

#include "botutil/base_sched.h"

class CBotTF2GetFlagSched : public CBotSchedule
{
  public:
	CBotTF2GetFlagSched(Vector vOrigin, bool bUseRoute = false, Vector vRoute = Vector(0, 0, 0));

	void init();
};
