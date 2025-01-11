#include "deploy_machine_gun_sched.h"

#include "bot_waypoint.h"
#include "botutil/dod/snipe_task.h"
#include "botutil/shared/find_path_task.h"

CDeployMachineGunSched ::CDeployMachineGunSched(CBotWeapon *pWeapon, CWaypoint *pWaypoint, Vector vEnemy)
{
	addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint), LOOK_LAST_ENEMY));
	addTask(new CBotDODSnipe(pWeapon, pWaypoint->getOrigin(), pWaypoint->getAimYaw(), true, vEnemy.z,
	                         pWaypoint->getFlags()));
}