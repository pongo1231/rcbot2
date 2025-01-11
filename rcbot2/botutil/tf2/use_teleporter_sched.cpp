#include "use_teleporter_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/use_teleporter_task.h"

CBotUseTeleSched ::CBotUseTeleSched(edict_t *pTele)
{
	// find path
	addTask(new CFindPathTask(pTele));       // first
	addTask(new CBotTFUseTeleporter(pTele)); // second
}
void CBotUseTeleSched ::init()
{
	setID(SCHED_USE_TELE);
}