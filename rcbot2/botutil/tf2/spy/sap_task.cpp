#include "sap_task.h"

#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_weapons.h"

#include <in_buttons.h>

CBotTF2SpySap ::CBotTF2SpySap(edict_t *pBuilding, eEngiBuild id)
{
	m_pBuilding = MyEHandle(pBuilding);
	m_fTime     = 0.0f;
	m_id        = id;
}

void CBotTF2SpySap ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pBuilding;
	CBotTF2 *tf2Bot = (CBotTF2 *)pBot;

	if (!pBot->isTF())
	{
		fail();
		return;
	}

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + randomFloat(4.0f, 6.0f);
		tf2Bot->resetCloakTime();
	}

	CBotWeapon *weapon;
	pBot->wantToShoot(false);

	pBuilding = m_pBuilding.get();

	if (!pBuilding)
	{
		complete();
		return;
	}

	if (m_id == ENGI_SENTRY)
	{
		if (CTeamFortress2Mod::isSentrySapped(pBuilding))
		{
			complete();
			return;
		}
	}
	else if (m_id == ENGI_DISP)
	{
		if (CTeamFortress2Mod::isDispenserSapped(pBuilding))
		{
			complete();
			return;
		}
	}
	else if (m_id == ENGI_TELE)
	{
		if (CTeamFortress2Mod::isTeleporterSapped(pBuilding))
		{
			complete();
			return;
		}
	}

	pBot->lookAtEdict(pBuilding);
	pBot->setLookAtTask(LOOK_EDICT, 0.2f);
	weapon = tf2Bot->getCurrentWeapon();

	// time out
	if (m_fTime < engine->Time())
		fail();
	else if (!weapon || (weapon->getID() != TF2_WEAPON_BUILDER))
	{
		helpers->ClientCommand(pBot->getEdict(), "build 3 0");
	}
	else
	{
		if (pBot->distanceFrom(pBuilding) > 100)
		{
			pBot->setMoveTo((CBotGlobals::entityOrigin(pBuilding)));
		}
		else
		{
			if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()))
			{
				tf2Bot->spyUnCloak();
			}
			else if (randomInt(0, 1))
				pBot->tapButton(IN_ATTACK);
			// complete();
		}
	}
}

void CBotTF2SpySap ::debugString(char *string)
{
	sprintf(string, "CBotTF2SpySap");
}