#pragma once

#include "botutil/base_sched.h"

class CBotTFEngiLookAfterSentry : public CBotSchedule
{
  public:
	CBotTFEngiLookAfterSentry(edict_t *pSentry);

	void init();
};