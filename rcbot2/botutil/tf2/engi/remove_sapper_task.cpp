#include "remove_sapper_task.h"

#include "bot_globals.h"
#include "bot_mods.h"

CBotRemoveSapper ::CBotRemoveSapper(edict_t *pBuilding, eEngiBuild id)
{
	m_fTime     = 0.0f;
	m_pBuilding = MyEHandle(pBuilding);
	m_id        = id;
	m_fHealTime = 0.0f;
}

void CBotRemoveSapper ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	// int i = 0;
	edict_t *pBuilding;
	CBotTF2 *pTF2Bot = (CBotTF2 *)pBot;

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + randomFloat(8.0f, 12.0f);
		pBot->removeCondition(CONDITION_COVERT);
	}

	pBuilding = m_pBuilding.get();

	if (!pBuilding)
	{
		fail();
		return;
	}

	if (m_id == ENGI_DISP)
	{
		if (!CTeamFortress2Mod::isDispenserSapped(pBuilding))
		{
			if (m_fHealTime == 0.0f)
				m_fHealTime = engine->Time() + randomFloat(2.0f, 3.0f);
			else if (m_fHealTime < engine->Time())
			{
				complete();
				return;
			}
		}
		else if (m_fHealTime > 0.0f)
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if (m_id == ENGI_TELE)
	{
		if (!CTeamFortress2Mod::isTeleporterSapped(pBuilding))
		{
			if (m_fHealTime == 0.0f)
				m_fHealTime = engine->Time() + randomFloat(2.0f, 3.0f);
			else if (m_fHealTime < engine->Time())
			{
				complete();
				return;
			}
		}
		else if (m_fHealTime > 0.0f)
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}
	else if (m_id == ENGI_SENTRY)
	{
		if (!CTeamFortress2Mod::isSentrySapped(pBuilding))
		{
			if (m_fHealTime == 0.0f)
				m_fHealTime = engine->Time() + randomFloat(2.0f, 3.0f);
			else if (m_fHealTime < engine->Time())
			{
				complete();
				return;
			}
		}
		else if (m_fHealTime > 0.0f)
		{
			fail();
			pTF2Bot->waitRemoveSap();
			return;
		}
	}

	if (m_fTime < engine->Time())
	{
		fail();
	} // Fix 16/07/09
	else if (!pBot->isVisible(pBuilding))
	{
		if (pBot->distanceFrom(pBuilding) > 200)
			fail();
		else if (pBot->distanceFrom(pBuilding) > 100)
			pBot->setMoveTo(CBotGlobals::entityOrigin(pBuilding));

		pBot->setLookAtTask(LOOK_EDICT, 0.15f);
		pBot->lookAtEdict(pBuilding);
	}
	else
	{
		if (!((CBotFortress *)pBot)->upgradeBuilding(pBuilding, true))
			fail();
	}
}