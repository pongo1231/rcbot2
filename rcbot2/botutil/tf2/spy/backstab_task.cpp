#include "backstab_task.h"

#include "bot_globals.h"
#include "bot_weapons.h"

CBotBackstab ::CBotBackstab(edict_t *_pEnemy)
{
	m_fTime = 0.0f;
	pEnemy  = _pEnemy;
}

void CBotBackstab ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	Vector vrear;
	Vector vangles;
	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;
	CBotTF2 *pTF2Bot = (CBotTF2 *)pBot;

	pBot->wantToChangeWeapon(false);
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	pBotWeapon = pBot->getCurrentWeapon();

	if (!pBotWeapon)
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}

	if (!CBotGlobals::isAlivePlayer(pEnemy))
		fail();

	if (!m_fTime)
		m_fTime = engine->Time() + randomFloat(5.0f, 10.0f);

	pBot->setLookAtTask(LOOK_EDICT);
	pBot->lookAtEdict(pEnemy);

	if (m_fTime < engine->Time())
	{
		fail();
		pTF2Bot->waitBackstab();
		return;
	}
	else if (!pEnemy || !CBotGlobals::entityIsValid(pEnemy) || !CBotGlobals::entityIsAlive(pEnemy))
	{
		if (pBot->getEnemy() && (pEnemy != pBot->getEnemy()) && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
		    && CBotGlobals::isAlivePlayer(pBot->getEnemy()))
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	else if (!pBot->isVisible(pEnemy))
	{
		// this guy will do
		if (pBot->getEnemy() && (pEnemy != pBot->getEnemy()) && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
		    && CBotGlobals::isAlivePlayer(pBot->getEnemy()))
		{
			pEnemy = pBot->getEnemy();
		}
		else
		{
			fail();
		}

		pTF2Bot->waitBackstab();
		return;
	}
	else if (pWeapon->getID() != TF2_WEAPON_KNIFE)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE)))
		{
			fail();
			pTF2Bot->waitBackstab();
			return;
		}
	}

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy), &vangles);
	vrear = CBotGlobals::entityOrigin(pEnemy) - (vangles * 45) + Vector(0, 0, 32);

	pTF2Bot->resetAttackingEnemy();

	if (pBot->distanceFrom(vrear) > 40)
	{
		pBot->setMoveTo(vrear);
	}
	else
	{
		// uncloak
		if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()))
			pTF2Bot->spyUnCloak();
		else
			pTF2Bot->handleAttack(pBotWeapon, pEnemy);
	}
}