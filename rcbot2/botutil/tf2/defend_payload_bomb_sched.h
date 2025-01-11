#pragma once

#include "botutil/base_sched.h"

class CBotTF2DefendPayloadBombSched : public CBotSchedule
{
  public:
	CBotTF2DefendPayloadBombSched(edict_t *ePayloadBomb);

	void init();
};