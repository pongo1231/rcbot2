#pragma once

#include "botutil/base_sched.h"

class CBotInvestigateNoiseSched : public CBotSchedule
{
  public:
	CBotInvestigateNoiseSched(Vector vSource, Vector vAttackPoint);

	void init();
};