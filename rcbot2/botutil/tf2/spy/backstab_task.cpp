#include "backstab_task.h"

#include "bot_globals.h"
#include "bot_weapons.h"

CBotBackstab::CBotBackstab(edict_t *_pEnemy)
{
	m_fTime = 0.0f;
	pEnemy  = _pEnemy;
}

void CBotBackstab::execute(CBot *pBot, CBotSchedule *pSchedule)
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
			pTF2Bot->waitBackstab();
			return;
		}
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
			pTF2Bot->waitBackstab();
			return;
		}
	}
	else if (pWeapon->getID() != TF2_WEAPON_KNIFE)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE)))
		{
			fail();
			pTF2Bot->waitBackstab();
			return;
		}
		pBotWeapon = pBot->getCurrentWeapon();
	}

	AngleVectors(CBotGlobals::entityEyeAngles(pEnemy), &vangles);
	vrear = CBotGlobals::entityOrigin(pEnemy) - (vangles * 45) + Vector(0, 0, 32);

	// Approach from alternating sides instead of walking straight at the enemy's back
	// This makes it harder for the enemy to detect and shoot the spy
	float fDistToRear = pBot->distanceFrom(vrear);
	Vector vRight     = vangles.Cross(Vector(0, 0, 1));

	if (vRight.Length() > 0.1f)
	{
		vRight = vRight / vRight.Length();

		// Alternate approach side every ~1.5 seconds based on time
		int iSide      = ((int)(engine->Time() * 0.7f)) % 2;
		float fSideDir = iSide ? 1.0f : -1.0f;

		if (fDistToRear > 100.0f)
		{
			// Far away: wide flank approach to avoid enemy's sight cone
			vrear = vrear + (vRight * fSideDir * 80.0f);
		}
		else if (fDistToRear > 50.0f)
		{
			// Medium range: narrower side approach
			vrear = vrear + (vRight * fSideDir * 35.0f);
		}
	}

	pTF2Bot->resetAttackingEnemy();

	if (fDistToRear > 40.0f)
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