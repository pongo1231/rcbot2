#pragma once

#include "botutil/base_sched.h"

class CBotTF2MessAroundSched : public CBotSchedule
{
  public:
	CBotTF2MessAroundSched(edict_t *pFriendly, int iMaxVoiceCmd);

	void init();
};