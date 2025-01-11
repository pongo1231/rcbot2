#include "pipetrap_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/demo/pipetrap_task.h"

CBotTF2DemoPipeTrapSched ::CBotTF2DemoPipeTrapSched(eDemoTrapType type, Vector vStand, Vector vLoc, Vector vSpread,
                                                    bool bAutoDetonate, int wptarea)
{
	addTask(new CFindPathTask(vStand));
	addTask(new CBotTF2DemomanPipeTrap(type, vStand, vLoc, vSpread, bAutoDetonate, wptarea));
}

void CBotTF2DemoPipeTrapSched ::init()
{
	setID(SCHED_TF2_DEMO_PIPETRAP);
}