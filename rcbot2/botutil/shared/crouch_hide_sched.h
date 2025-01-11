#pragma once

#include "botutil/base_sched.h"

class CCrouchHideSched : public CBotSchedule
{
  public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CCrouchHideSched(edict_t *pCoverFrom);

	void init();
};