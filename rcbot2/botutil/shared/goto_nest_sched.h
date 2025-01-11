#pragma once

#include "botutil/base_sched.h"

class CGotoNestSched : public CBotSchedule
{
  public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CGotoNestSched(int iWaypoint);

	void init();
};