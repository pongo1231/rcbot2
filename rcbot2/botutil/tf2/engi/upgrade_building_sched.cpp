#include "upgrade_building_sched.h"

#include "bot_mods.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/tf2/engi/interrupt.h"
#include "botutil/tf2/engi/upgrade_building_task.h"

CBotTFEngiUpgrade ::CBotTFEngiUpgrade(CBot *pBot, edict_t *pBuilding)
{
	CFindPathTask *pathtask = new CFindPathTask(pBuilding);

	addTask(pathtask);

	pathtask->completeInRangeFromEdict();
	pathtask->failIfTaskEdictDead();
	pathtask->setRange(150.0f);

	if (!CTeamFortress2Mod::isSentryGun(pBuilding))
	{
		pathtask->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));

		CBotTF2UpgradeBuilding *upgbuilding = new CBotTF2UpgradeBuilding(pBuilding);
		addTask(upgbuilding);
		upgbuilding->setInterruptFunction(new CBotTF2EngineerInterrupt(pBot));
	}
	else
	{
		addTask(new CBotTF2UpgradeBuilding(pBuilding));
	}
}

void CBotTFEngiUpgrade ::init()
{
	setID(SCHED_UPGRADE);
}