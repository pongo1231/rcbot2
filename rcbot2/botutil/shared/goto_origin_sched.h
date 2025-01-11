#pragma once

#include "botutil/base_sched.h"

class CBotGotoOriginSched : public CBotSchedule
{
  public:
	CBotGotoOriginSched(Vector vOrigin);

	CBotGotoOriginSched(edict_t *pEdict);

	void init();
};