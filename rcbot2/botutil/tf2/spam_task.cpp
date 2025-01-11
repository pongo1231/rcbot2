#include "spam_task.h"

#include "bot_cvars.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "botutil/base_sched.h"
#include "util/grenade.h"

CBotTF2Spam ::CBotTF2Spam(CBot *pBot, Vector vStart, int iYaw, CBotWeapon *pWeapon)
{
	Vector forward;
	QAngle angle = QAngle(0, iYaw, 0);

	AngleVectors(angle, &forward);
	m_vTarget = vStart + forward * 2000.0f;
	CBotGlobals::quickTraceline(pBot->getEdict(), vStart, m_vTarget);
	m_vTarget = CBotGlobals::getTraceResult()->endpos - forward;
	m_pWeapon = pWeapon;
	m_vStart  = vStart;

	m_fTime   = 0.0f;
}

CBotTF2Spam ::CBotTF2Spam(Vector vStart, Vector vTarget, CBotWeapon *pWeapon)
{
	m_vTarget = vTarget;
	m_pWeapon = pWeapon;
	m_vStart  = vStart;

	m_fTime   = 0.0f;
}

float CBotTF2Spam ::getDistance()
{
	return (m_vStart - m_vTarget).Length();
}

void CBotTF2Spam ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
	{
		fail();
		return;
	}

	if (pBot->recentlyHurt(1.0f))
	{
		// taking damage
		fail();
		pBot->updateUtilTime(pBot->getCurrentUtil());
		pBot->wantToShoot(true);
		return;
	}

	if (m_fTime == 0.0f)
	{
		if (m_pWeapon->getID() == TF2_WEAPON_GRENADELAUNCHER)
		{
			Vector vVisibleWaypoint = pSchedule->passedVector();

			if (vVisibleWaypoint.z > (m_vTarget.z + 32.0f)) // need to lob grenade
				m_vTarget.z += Util::GetGrenadeZ(pBot->getEdict(), nullptr, pBot->getOrigin(), m_vTarget,
				                                 m_pWeapon->getProjectileSpeed());
			else // mid point between waypoint and target as I won't see target
				m_vTarget = (m_vTarget + vVisibleWaypoint) / 2;
		}

		m_fNextAttack = engine->Time() + randomFloat(0.2f, 1.0f);
		m_fTime       = engine->Time() + randomFloat(12.0f, 24.0f);
	}
	else if (m_fTime < engine->Time())
	{
		complete();

		pBot->setLastEnemy(nullptr);

		// prevent bot from keeping doing this
		pBot->updateUtilTime(BOT_UTIL_SPAM_NEAREST_SENTRY);
		pBot->updateUtilTime(BOT_UTIL_SPAM_LAST_ENEMY);
		pBot->updateUtilTime(BOT_UTIL_SPAM_LAST_ENEMY_SENTRY);
	}

	if (pBot->distanceFrom(m_vStart) > 100)
		pBot->setMoveTo(m_vStart);
	else
		pBot->stopMoving();

	if (pBot->getCurrentWeapon() != m_pWeapon)
	{
		if (!pBot->selectBotWeapon(m_pWeapon))
			fail();
	}

	pBot->setLookVector(m_vTarget);
	pBot->setLookAtTask(LOOK_VECTOR);

	if (m_pWeapon->outOfAmmo(pBot))
	{
		pBot->setLastEnemy(nullptr);
		complete();
	}
	else if (m_pWeapon->needToReload(pBot))
	{
		if (randomInt(0, 1))
			pBot->reload();
	}
	else if (m_fNextAttack < engine->Time())
	{
		pBot->primaryAttack();
		m_fNextAttack = engine->Time() + randomFloat(0.2f, 1.0f);
	}
}