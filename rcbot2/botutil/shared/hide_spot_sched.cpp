#include "hide_spot_sched.h"

#include "botutil/dod/wait_for_grenade_task.h"
#include "botutil/shared/find_good_hide_spot_task.h"
#include "botutil/shared/find_path_task.h"

CGotoHideSpotSched ::CGotoHideSpotSched(CBot *pBot, edict_t *pEdict, bool bIsGrenade)
{
	// run at flank while shooting
	CFindPathTask *pHideGoalPoint = new CFindPathTask(pEdict);

	pBot->setCoverFrom(pEdict);
	addTask(new CFindGoodHideSpot(pEdict));
	addTask(pHideGoalPoint);
	if (bIsGrenade)
		addTask(new CDODWaitForGrenadeTask(pEdict));

	// don't need to hide if the player we're hiding from died while we're running away
	pHideGoalPoint->failIfTaskEdictDead();
	pHideGoalPoint->setLookTask(LOOK_WAYPOINT);
	// no interrupts, should be a quick waypoint path anyway
	pHideGoalPoint->setNoInterruptions();
	// get vector from good hide spot task
	pHideGoalPoint->getPassedVector();
	pHideGoalPoint->dontGoToEdict();
	if (bIsGrenade)
	{
		pHideGoalPoint->setRange(BLAST_RADIUS + 100.0f);
		pHideGoalPoint->completeOutOfRangeFromEdict();
	}
}

CGotoHideSpotSched ::CGotoHideSpotSched(CBot *pBot, Vector vOrigin, IBotTaskInterrupt *interrupt)
{
	// run at flank while shooting
	CFindPathTask *pHideGoalPoint = new CFindPathTask();

	pBot->setCoverFrom(nullptr);
	addTask(new CFindGoodHideSpot(vOrigin));
	addTask(pHideGoalPoint);

	// no interrupts, should be a quick waypoint path anyway
	pHideGoalPoint->setNoInterruptions();
	pHideGoalPoint->setInterruptFunction(interrupt);
	// get vector from good hide spot task
	pHideGoalPoint->getPassedVector();
}

void CGotoHideSpotSched ::init()
{
	setID(SCHED_GOOD_HIDE_SPOT);
}