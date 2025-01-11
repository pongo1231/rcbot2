#include "pipe_enemy_task.h"

#include "bot_mods.h"
#include "bot_weapons.h"
#include "botutil/base_sched.h"
#include "util/grenade.h"

#include <in_buttons.h>

CBotTF2DemomanPipeEnemy ::CBotTF2DemomanPipeEnemy(CBotWeapon *pPipeLauncher, Vector vEnemy, edict_t *pEnemy)
{
	m_vEnemy          = vEnemy;
	m_pEnemy          = MyEHandle(pEnemy);
	m_fTime           = 0.0f;
	m_vAim            = vEnemy;
	m_pPipeLauncher   = pPipeLauncher;
	m_fHoldAttackTime = 0.0f;
	m_fHeldAttackTime = 0.0f;
}

void CBotTF2DemomanPipeEnemy ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_pEnemy.get() == nullptr)
	{
		fail();
		return;
	}

	if (pBot->recentlyHurt(1.0f))
	{
		// taking damage
		fail();
		pBot->updateUtilTime(pBot->getCurrentUtil());
		return;
	}

	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
		fail();

	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	if (m_fTime == 0)
	{

		m_vStand              = CWaypoints::getWaypoint(pSchedule->passedInt())->getOrigin();
		Vector vOtherWaypoint = pSchedule->passedVector();
		((CBotTF2 *)pBot)->setStickyTrapType(m_vEnemy, TF_TRAP_TYPE_ENEMY);

		// Need to Lob my pipes
		if (vOtherWaypoint.z > (m_vEnemy.z + 32.0f))
		{
			m_vAim   = m_vEnemy; //(m_vEnemy - pBot->getOrigin())/2;
			m_vAim.z = m_vEnemy.z
			         + Util::GetGrenadeZ(
			               pBot->getEdict(), m_pEnemy, pBot->getOrigin(), m_vEnemy,
			               TF2_GRENADESPEED); //();//(sv_gravity->GetFloat() * randomFloat(0.9f,1.1f) * fFraction);
		}
		else
		{
			// otherwise just aim at the closest waypoint
			m_vAim = (vOtherWaypoint + m_vEnemy) / 2;
		}

		m_fHoldAttackTime = (pBot->distanceFrom(m_vEnemy) / 512.0f) - 1.0f;

		if (m_fHoldAttackTime < 0.0f)
			m_fHoldAttackTime = 0.0f;

		/*
		if ( sv_gravity )
		{
		    float fFraction = ((m_vAim-m_vStand).Length())/MAX_GREN_THROW_DIST;

		    m_vAim.z = m_vAim.z + (sv_gravity->GetFloat() * randomFloat(1.5f,2.5f) * fFraction);
		}*/

		m_fTime = engine->Time() + randomFloat(5.0f, 10.0f);
	}

	if (pBot->distanceFrom(m_vStand) > 200)
	{
		fail();
		pBot->setLastEnemy(nullptr);
	}

	if (!CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy) || (m_fTime < engine->Time()))
	{
		// blow up any grens before we finish
		// if ( m_pEnemy.get() && pBot->isVisible(m_pEnemy.get()) )
		((CBotTF2 *)pBot)->detonateStickies(true);

		complete();
		pBot->setLastEnemy(nullptr);
		return;
	}

	pBot->wantToShoot(false);

	if ((m_pPipeLauncher->getAmmo(pBot) + m_pPipeLauncher->getClip1(pBot)) == 0)
	{
		if (pBot->isVisible(m_pEnemy.get()))
			((CBotTF2 *)pBot)->detonateStickies(true);

		complete();
		pBot->setLastEnemy(nullptr);
		return;
	}

	if (pBot->getCurrentWeapon() != m_pPipeLauncher)
	{
		pBot->selectBotWeapon(m_pPipeLauncher);
		pBot->setLastEnemy(nullptr);
		return;
	}

	pBot->wantToChangeWeapon(false);

	if (m_pPipeLauncher->getClip1(pBot) == 0)
	{
		if (randomInt(0, 1))
			pBot->reload();
	}
	else if (pBot->distanceFrom(m_vStand) > 100.0f)
		pBot->setMoveTo(m_vStand);
	else
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vAim);
		pBot->stopMoving();

		if (pBot->DotProductFromOrigin(m_vAim) > 0.99)
		{
			float fTime = engine->Time();

			if (m_fHeldAttackTime == 0)
				m_fHeldAttackTime = fTime + m_fHoldAttackTime + randomFloat(0.0, 0.15);

			if (m_fHeldAttackTime > fTime)
				pBot->primaryAttack(true);
			else
			{
				if (m_fHeldAttackTime < (fTime - 0.1f))
					m_fHeldAttackTime = 0;

				pBot->letGoOfButton(IN_ATTACK);
			}

			((CBotTF2 *)pBot)->setStickyTrapType(m_vEnemy, TF_TRAP_TYPE_ENEMY);
		}
	}
}