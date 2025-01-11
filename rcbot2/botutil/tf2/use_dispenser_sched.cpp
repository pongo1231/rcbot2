#include "use_dispenser_sched.h"

#include "bot_globals.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/interrupt.h"
#include "botutil/tf2/wait_health_task.h"

CBotUseDispSched ::CBotUseDispSched(CBot *pBot, edict_t *pDisp) //, bool bNest )
{
	CFindPathTask *pathtask          = new CFindPathTask(pDisp);
	CBotTF2WaitHealthTask *gethealth = new CBotTF2WaitHealthTask(CBotGlobals::entityOrigin(pDisp));
	addTask(pathtask);
	pathtask->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));

	addTask(gethealth); // second
	gethealth->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));

	// if ( bNest )
	//	addTask(new CBotNest()); // third
}

void CBotUseDispSched ::init()
{
	setID(SCHED_USE_DISPENSER);
}