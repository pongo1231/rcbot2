#pragma once

#include "botutil/base_sched.h"

class CBotTF2SnipeCrossBowSched : public CBotSchedule
{
  public:
	CBotTF2SnipeCrossBowSched(Vector vOrigin, int iWpt);

	void init();
};