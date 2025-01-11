#pragma once

#include "botutil/base_sched.h"

class CBotTF2SnipeSched : public CBotSchedule
{
  public:
	CBotTF2SnipeSched(Vector vOrigin, int iWpt);

	void init();
};