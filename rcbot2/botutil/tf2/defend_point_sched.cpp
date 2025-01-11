#include "defend_point_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/defend_point_task.h"

CBotDefendPointSched ::CBotDefendPointSched(Vector vPoint, int iRadius, int iArea)
{
	addTask(new CFindPathTask(vPoint));                      // first
	addTask(new CBotTF2DefendPoint(iArea, vPoint, iRadius)); // second
}

void CBotDefendPointSched ::init()
{
	setID(SCHED_DEFENDPOINT);
}