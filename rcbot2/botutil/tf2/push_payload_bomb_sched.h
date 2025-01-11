#pragma once

#include "botutil/base_sched.h"

class CBotTF2PushPayloadBombSched : public CBotSchedule
{
  public:
	CBotTF2PushPayloadBombSched(edict_t *ePayloadBomb);

	void init();
};