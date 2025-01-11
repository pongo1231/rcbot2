#include "defend_payload_bomb_sched.h"

#include "bot_globals.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/defend_payload_bomb_task.h"

CBotTF2DefendPayloadBombSched ::CBotTF2DefendPayloadBombSched(edict_t *ePayloadBomb)
{
	addTask(new CFindPathTask(CBotGlobals::entityOrigin(ePayloadBomb))); // first
	addTask(new CBotTF2DefendPayloadBombTask(ePayloadBomb));             // second
}

void CBotTF2DefendPayloadBombSched ::init()
{
	setID(SCHED_TF2_DEFEND_PAYLOADBOMB);
}