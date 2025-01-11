#include "heal_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_navigator.h"
#include "bot_weapons.h"

#include <in_buttons.h>

CBotTF2MedicHeal ::CBotTF2MedicHeal()
{
	m_pHeal         = nullptr;
	m_bHealerJumped = false;
}

void CBotTF2MedicHeal::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pHeal;
	CBotTF2 *pBotTF2;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if (!pBot->isTF2())
	{
		fail();
		return;
	}

	pBotTF2 = (CBotTF2 *)pBot;

	pHeal   = pBotTF2->getHealingEntity();

	if (pHeal && !m_bHealerJumped)
	{
		Vector vVelocity;

		if (!CClassInterface::getVelocity(pHeal, &vVelocity) || (vVelocity.Length2D() > 1.0f))
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(pHeal);

			if (p)
			{
				if (p->GetLastUserCommand().buttons & IN_JUMP)
				{
					m_bHealerJumped = true;
					m_vJump         = CBotGlobals::entityOrigin(pHeal);
				}
			}
		}
	}

	if (!pHeal)
	{
		// because the medic would have followed this guy, he would have lost his own waypoint
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if (pBot->getCurrentWeapon() == nullptr)
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if (pBotTF2->getHealFactor(pHeal) == 0.0f)
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	} /*
	 else if ( !pBot->isVisible(pHeal) )
	 {
	     pBot->getNavigator()->rollBackPosition();
	     ((CBotFortress*)pBot)->clearHealingEntity();
	     fail();
	 }*/
	else if (pBot->distanceFrom(pHeal) > 416)
	{
		pBotTF2->clearHealingEntity();
		pBot->getNavigator()->rollBackPosition();
		fail();
	}
	else if (pBot->getCurrentWeapon()->getWeaponInfo()->getID() != TF2_WEAPON_MEDIGUN)
	{
		pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_MEDIGUN));
	}
	else
	{
		Vector vVelocity;

		CClassInterface::getVelocity(pBot->getEdict(), &vVelocity);

		pBot->clearFailedWeaponSelect();

		if (m_bHealerJumped && (pBot->distanceFrom(m_vJump) < 64.0f) && (vVelocity.Length() > 1.0f))
		{
			pBot->jump();
			m_bHealerJumped = false;
		}

		if (!pBotTF2->healPlayer())
		{
			pBot->getNavigator()->rollBackPosition();
			pBotTF2->clearHealingEntity();
			fail();
		}
	}
}
