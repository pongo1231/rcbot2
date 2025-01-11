#pragma once

#include "botutil/base_sched.h"

class CGotoHideSpotSched : public CBotSchedule
{
  public:
	// find a hide spot
	// hide from an enemy (pEdict)
	CGotoHideSpotSched(CBot *pBot, edict_t *pEdict, bool bIsGrenade = false);
	// hide from a Vector
	CGotoHideSpotSched(CBot *pBot, Vector vOrigin, IBotTaskInterrupt *interrupt = nullptr);

	void init();
};