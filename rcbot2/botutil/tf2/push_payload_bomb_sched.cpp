#include "push_payload_bomb_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/push_payload_bomb_task.h"

CBotTF2PushPayloadBombSched ::CBotTF2PushPayloadBombSched(edict_t *ePayloadBomb)
{
	addTask(new CFindPathTask(ePayloadBomb));              // first
	addTask(new CBotTF2PushPayloadBombTask(ePayloadBomb)); // second
}

void CBotTF2PushPayloadBombSched ::init()
{
	setID(SCHED_TF2_PUSH_PAYLOADBOMB);
}