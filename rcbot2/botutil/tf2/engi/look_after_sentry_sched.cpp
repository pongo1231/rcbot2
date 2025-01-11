#include "look_after_sentry_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/look_after_task.h"

CBotTFEngiLookAfterSentry ::CBotTFEngiLookAfterSentry(edict_t *pSentry)
{
	addTask(new CFindPathTask(pSentry));        // first
	addTask(new CBotTF2EngiLookAfter(pSentry)); // second
}

void CBotTFEngiLookAfterSentry ::init()
{
	setID(SCHED_LOOKAFTERSENTRY);
}