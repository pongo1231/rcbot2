#include "crouch_hide_sched.h"

#include "botutil/shared/crouch_hide_task.h"

CCrouchHideSched ::CCrouchHideSched(edict_t *pCoverFrom)
{
	addTask(new CCrouchHideTask(pCoverFrom));
}

void CCrouchHideSched ::init()
{
	setID(SCHED_CROUCH_AND_HIDE);
}