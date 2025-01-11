#pragma once

#include "botutil/base_sched.h"

class CDeployMachineGunSched : public CBotSchedule
{
  public:
	// iWaypoint = the waypoint number the bot will go to (to nest)
	// if iWaypoint is -1 it will find a random, suitable nest
	CDeployMachineGunSched(CBotWeapon *pWeapon, CWaypoint *pWaypoint, Vector vEnemy);

	void init()
	{
		setID(SCHED_DEPLOY_MACHINE_GUN);
	}
};