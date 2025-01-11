#pragma once

#include "botutil/base_sched.h"

class CBotAttackPointSched : public CBotSchedule
{
  public:
	CBotAttackPointSched(Vector vPoint, int iRadius, int iArea, bool bHasRoute = false, Vector vRoute = Vector(0, 0, 0),
	                     bool bNest = false, edict_t *pLastEnemySentry = nullptr);

	void init();
};