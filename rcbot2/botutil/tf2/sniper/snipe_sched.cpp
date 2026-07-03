#include "snipe_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/sniper/snipe_task.h"

CBotTF2SnipeSched::CBotTF2SnipeSched(Vector vOrigin, int iWpt)
{
	CBotTask *pFindPath;
	if (iWpt >= 0)
		pFindPath = new CFindPathTask(iWpt);
	else
		pFindPath = new CFindPathTask(vOrigin);

	CBotTask *pSnipeTask = new CBotTF2Snipe(vOrigin, iWpt);

	addTask(pFindPath);  // first
	addTask(pSnipeTask); // second

	pFindPath->setFailInterrupt(CONDITION_PARANOID);
	pSnipeTask->setFailInterrupt(CONDITION_PARANOID);
}

void CBotTF2SnipeSched::init()
{
	setID(SCHED_SNIPE);
}