#pragma once

#include "botutil/base_sched.h"

class CBotTF2GetAmmoSched : public CBotSchedule
{
  public:
	CBotTF2GetAmmoSched(Vector vOrigin);

	void init();
};