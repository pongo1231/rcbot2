#include "mess_around_sched.h"

#include "botutil/shared/mess_around_task.h"

CBotTF2MessAroundSched ::CBotTF2MessAroundSched(edict_t *pFriendly, int iMaxVoiceCmd)
{
	addTask(new CMessAround(pFriendly, iMaxVoiceCmd));
}

void CBotTF2MessAroundSched ::init()
{
	setID(SCHED_MESSAROUND);
}