#pragma once

#include "botutil/base_sched.h"

class CBotPickupSched : public CBotSchedule
{
  public:
	CBotPickupSched(edict_t *pEdict);

	void init();
};