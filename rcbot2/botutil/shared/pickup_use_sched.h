#pragma once

#include "botutil/base_sched.h"

class CBotPickupSchedUse : public CBotSchedule
{
  public:
	CBotPickupSchedUse(edict_t *pEdict);

	void init();
};