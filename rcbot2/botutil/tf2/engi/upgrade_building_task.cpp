#include "upgrade_building_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"

CBotTF2UpgradeBuilding ::CBotTF2UpgradeBuilding(edict_t *pBuilding)
{
	m_pBuilding = pBuilding;
	m_fTime     = 0.0f;
}

void CBotTF2UpgradeBuilding ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pBuilding = m_pBuilding.get();
	edict_t *pOwner    = nullptr;
	pBot->wantToShoot(false);
	pBot->wantToInvestigateSound(false);
	pBot->wantToListen(false);

	if (!m_fTime)
		m_fTime = engine->Time() + randomFloat(9.0f, 11.0f);

	if (m_fTime < engine->Time())
	{
		complete();
	} // Fix 16/07/09
	else if (pBuilding == nullptr)
	{
		fail();
		return;
	}
	else if (((pOwner = CTeamFortress2Mod::getSentryOwner(pBuilding)) != nullptr)
	         && CClassInterface::isCarryingObj(pOwner) && (CClassInterface::getCarriedObj(pOwner) == pBuilding))
	{
		// Owner is carrying it
		fail();
		return;
	}
	else if (!pBot->isVisible(pBuilding))
	{
		if (pBot->distanceFrom(pBuilding) > 200)
			fail();
		else if (pBot->distanceFrom(pBuilding) > 100)
			pBot->setMoveTo(CBotGlobals::entityOrigin(pBuilding));

		pBot->setLookAtTask(LOOK_EDICT, 0.15f);
		pBot->lookAtEdict(pBuilding);
	}
	else if (CBotGlobals::entityIsValid(pBuilding) && CBotGlobals::entityIsAlive(pBuilding))
	{
		if (!((CBotFortress *)pBot)->upgradeBuilding(pBuilding))
			fail();
	}
	else
		fail();
}

void CBotTF2UpgradeBuilding::debugString(char *string)
{
	sprintf(string, "CBotTF2UpgradeBuilding");
}