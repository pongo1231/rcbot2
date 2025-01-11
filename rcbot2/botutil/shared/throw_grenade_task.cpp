#include "throw_grenade_task.h"

#include "bot_cvars.h"
#include "bot_dod_bot.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_weapons.h"

CThrowGrenadeTask ::CThrowGrenadeTask(CBotWeapon *pWeapon, int ammo, Vector vLoc)
{
	m_pWeapon         = pWeapon;
	m_fTime           = 0;
	m_vLoc            = vLoc;

	m_fHoldAttackTime = 0;
	m_iAmmo           = ammo;
}

void CThrowGrenadeTask ::init()
{
	m_fTime = 0;
}

void CThrowGrenadeTask::debugString(char *string)
{
	sprintf(string, "CThrowGrenadeTask\nm_vLoc =(%0.4f,%0.4f,%0.4f)\nfTime = %0.4f", m_vLoc.x, m_vLoc.y, m_vLoc.z,
	        m_fTime);
}

void CThrowGrenadeTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fTime == 0)
	{
		m_fTime = engine->Time() + 2.5f;

		if (sv_gravity.IsValid())
		{
			float fFraction = pBot->distanceFrom(m_vLoc) / MAX_GREN_THROW_DIST;
			// m_vLoc.z = m_vLoc.z + getGrenadeZ(pBot->getOrigin(),m_vLoc,m_pWeapon->getProjectileSpeed());
			m_vLoc.z        = m_vLoc.z + (sv_gravity.GetFloat() * randomFloat(1.5f, 2.5f) * fFraction);
		}
	}

	// time out
	if (m_fTime < engine->Time())
	{
		fail();
		return;
	}
	// used all grenades Updated: 2017-05-22 Created: 2017 - 05 - 22 Creator : Gabor Szuromi
	if (!m_pWeapon || !m_pWeapon->hasWeapon())
	{
		complete();
		return;
	}

	if (m_pWeapon->getAmmo(pBot) < m_iAmmo)
	{
		pBot->grenadeThrown();
		complete();
		return;
	}

	if (!m_pWeapon)
	{
		fail();
		return;
	}

	CBotWeapon *pWeapon;

	pWeapon = pBot->getCurrentWeapon();
	pBot->wantToChangeWeapon(false);

	if (pWeapon && pWeapon->isGravGun() && CClassInterface::gravityGunObject(INDEXENT(pWeapon->getWeaponIndex())))
	{
		// drop it
		if (randomInt(0, 1))
			pBot->primaryAttack();
		else
			pBot->secondaryAttack();
	}
	else if (pWeapon != m_pWeapon)
	{
		pBot->selectBotWeapon(m_pWeapon);
	}
	else
	{
		pBot->setLookVector(m_vLoc);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (pBot->isFacing(m_vLoc))
		{
			if (randomInt(0, 1))
				pBot->primaryAttack();
		}
		else if (pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
		{
			pBot->primaryAttack();
			fail();
		}
	}
}