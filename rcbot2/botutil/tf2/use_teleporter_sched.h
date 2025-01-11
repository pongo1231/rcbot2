#pragma once

#include "botutil/base_sched.h"

class CBotUseTeleSched : public CBotSchedule
{
  public:
	CBotUseTeleSched(edict_t *pTele);

	void init();
};