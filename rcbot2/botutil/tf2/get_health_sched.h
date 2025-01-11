#pragma once

#include "botutil/base_sched.h"

class CBotTF2GetHealthSched : public CBotSchedule
{
  public:
	CBotTF2GetHealthSched(Vector vOrigin);

	void init();
};