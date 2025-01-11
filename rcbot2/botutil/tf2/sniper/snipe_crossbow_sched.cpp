#include "snipe_crossbow_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/sniper/snipe_crossbow_task.h"

CBotTF2SnipeCrossBowSched::CBotTF2SnipeCrossBowSched(Vector vOrigin, int iWpt)
{
	CBotTask *pFindPath  = new CFindPathTask(iWpt);
	CBotTask *pSnipeTask = new CBotTF2SnipeCrossBow(vOrigin, iWpt);

	addTask(pFindPath);  // first
	addTask(pSnipeTask); // second

	pFindPath->setFailInterrupt(CONDITION_PARANOID);
	pSnipeTask->setFailInterrupt(CONDITION_PARANOID);
}

void CBotTF2SnipeCrossBowSched::init()
{
	setID(SCHED_SNIPE);
}