#include "pickup_building_task.h"

#include "bot_globals.h"
#include "bot_weapons.h"

#include <in_buttons.h>

CBotTaskEngiPickupBuilding ::CBotTaskEngiPickupBuilding(edict_t *pBuilding)
{
	m_pBuilding = pBuilding;
	m_fTime     = 0.0f;
}
// move building / move sentry / move disp / move tele
void CBotTaskEngiPickupBuilding ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if (m_fTime == 0.0f)
		m_fTime = engine->Time() + 6.0f;

	if (!m_pBuilding.get())
	{
		fail();
		return;
	}

	// don't pick up sentry now because my sentry is in good use!!!
	if (((CBotTF2 *)pBot)->sentryRecentlyHadEnemy())
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->lookAtEdict(m_pBuilding.get());
	pBot->setLookAtTask(LOOK_EDICT);

	((CBotTF2 *)pBot)->updateCarrying();

	if (((CBotTF2 *)pBot)->isCarrying()) // if ( CBotGlobals::entityOrigin(m_pBuilding) ==
	                                     // CBotGlobals::entityOrigin(pBot->getEdict()) )
		complete();
	else if (m_fTime < engine->Time())
		fail();
	else if (!pWeapon)
		fail();
	else if (pWeapon->getID() != TF2_WEAPON_WRENCH)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)))
			fail();
	}
	else if (pBot->distanceFrom(m_pBuilding) < 100)
	{
		if (CBotGlobals::yawAngleFromEdict(pBot->getEdict(), CBotGlobals::entityOrigin(m_pBuilding)) < 25)
		{
			if (randomInt(0, 1))
				pBot->secondaryAttack();
			else
				pBot->letGoOfButton(IN_ATTACK2);

			((CBotTF2 *)pBot)->resetCarryTime();
		}
	}
	else
		pBot->setMoveTo((CBotGlobals::entityOrigin(m_pBuilding)));
}
void CBotTaskEngiPickupBuilding ::debugString(char *string)
{
	sprintf(string, "CBotTaskEngiPickupBuilding");
}