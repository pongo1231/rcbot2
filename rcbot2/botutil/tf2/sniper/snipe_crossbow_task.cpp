#include "snipe_crossbow_task.h"

#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint_visibility.h"
#include "bot_weapons.h"

CBotTF2SnipeCrossBow::CBotTF2SnipeCrossBow(Vector vOrigin, int iWpt)
{
	CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);
	m_iSnipeWaypoint     = iWpt;
	QAngle angle;
	m_fAimTime = 0.f;
	m_fTime    = 0.f;
	angle      = QAngle(0, pWaypoint->getAimYaw(), 0);
	AngleVectors(angle, &m_vAim);
	m_vAim       = vOrigin + (m_vAim * 4096);
	m_vOrigin    = vOrigin;
	m_fEnemyTime = 0.f;
	m_vEnemy     = m_vAim;
	m_iArea      = pWaypoint->getArea();
}

void CBotTF2SnipeCrossBow::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;
	CBotTF2 *pBotTF2;

	pBotTF2 = (CBotTF2 *)pBot;
	// Sniper should move if the point has changed, so he's not wasting time
	if (!CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iArea))
		fail(); // move up
	else if (m_iArea > 0)
	{
		if (CTeamFortress2Mod::isAttackDefendMap())
		{
			if (pBotTF2->getTeam() == TF2_TEAM_BLUE)
			{
				if (m_iArea != pBotTF2->getCurrentAttackArea())
					complete();
			}
			else if (m_iArea != pBotTF2->getCurrentDefendArea())
				complete();
		}
		else if ((m_iArea != pBotTF2->getCurrentAttackArea()) && (m_iArea != pBotTF2->getCurrentDefendArea()))
		{
			complete(); // move up
		}
	}

	// disable normal attack functions
	pBot->wantToShoot(false);
	pBotTF2->resetAttackingEnemy();
	// disable listening functions
	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	// change weapon to sniper if not already
	pBotWeapon = pBot->getCurrentWeapon();

	if (!pBotWeapon)
	{
		fail();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if (pWeapon->getID() != TF2_WEAPON_BOW)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_BOW)))
		{
			fail();
			return;
		}
	}

	// initialize
	if (m_fTime == 0.0f)
	{
		// distance from sniper point default
		m_fOriginDistance = 100.0f;
		m_fHideTime       = 0.0f;
		m_fEnemyTime      = 0.0f;
		m_fAimTime        = 0.0f;
		m_fTime           = engine->Time() + randomFloat(40.0f, 90.0f);
		// pBot->secondaryAttack();
		pBot->resetLookAroundTime();
		m_iPrevClip = pBotWeapon->getAmmo(pBot);

		CBotGlobals::quickTraceline(pBot->getEdict(), m_vOrigin, m_vAim);

		int iAimWpt =
		    CWaypointLocations::NearestWaypoint(CBotGlobals::getTraceResult()->endpos, 400.0f, -1, true, true);

		if (iAimWpt != -1)
		{
			CWaypoint *pWaypoint             = CWaypoints::getWaypoint(m_iSnipeWaypoint);
			CWaypointVisibilityTable *pTable = CWaypoints::getVisiblity();

			int iPathId;

			for (int i = 0; i < pWaypoint->numPaths(); i++)
			{
				iPathId = pWaypoint->getPath(i);

				// isn't visible to the target
				if (!pTable->GetVisibilityFromTo(iAimWpt, iPathId))
				{
					m_iHideWaypoint = iPathId;
					m_vHideOrigin   = CWaypoints::getWaypoint(iPathId)->getOrigin();
					break;
				}
			}

			if (m_iHideWaypoint == -1)
			{
				// can't find a useful hide waypoint -- choose a random one
				m_iHideWaypoint = pWaypoint->getPath(randomInt(0, pWaypoint->numPaths()));

				if (m_iHideWaypoint != -1)
				{
					CWaypoint *pHideWaypoint = CWaypoints::getWaypoint(m_iHideWaypoint);

					if (pHideWaypoint != nullptr)
					{
						m_vHideOrigin = pHideWaypoint->getOrigin();
					}
					else
						m_iHideWaypoint = -1;
				}
			}
		}

		// check wall time
		m_fCheckTime = engine->Time() + 0.15f;
	}
	else if (m_fTime < engine->Time())
	{
		// if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		complete();
		return;
	}

	// look at waypoint yaw and have random updates
	pBot->setLookAtTask(LOOK_SNIPE);

	// saw an enemy less than 5 secs ago
	if ((m_fEnemyTime + 5.0f) > engine->Time())
		pBot->setAiming(m_vEnemy);
	else
		pBot->setAiming(m_vAim);

	if (!pBot->isTF() || (((CBotFortress *)pBot)->getClass() != TF_CLASS_SNIPER) || (pBot->getHealthPercent() < 0.2))
	{
		// out of health -- finish
		// if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		fail();
		return;
	}
	else if (pBotWeapon->getAmmo(pBot) < 1)
	{
		// out of ammo -- finish
		// if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();

		complete();
	}
	else if (pBot->distanceFrom(m_vOrigin) > 400)
	{
		// if (CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
		//	pBot->secondaryAttack();
		//  too far away
		fail();
	}
	else if ((m_iHideWaypoint != -1)
	         && (((CBotFortress *)pBot)->incomingRocket(512.0f) || (m_fHideTime > engine->Time())))
	{
		// hide time -- move to hide origin
		pBot->setMoveTo(m_vHideOrigin);

		if (pBot->distanceFrom(m_vHideOrigin) < 100)
		{
			pBot->stopMoving();

			// if (pBotWeapon->needToReload(pBot))
			//	pBot->reload();
		}
	}
	else if (pBot->distanceFrom(m_vOrigin) > m_fOriginDistance)
	{
		// is too far from origin -- use normal attack code at this time otherwise bot will be a sitting duck
		pBot->setMoveTo(m_vOrigin);
		pBot->wantToShoot(true);
	}
	else
	{
		pBot->stopMoving();

		if (pBot->isHoldingPrimaryAttack() && pBot->hasEnemy())
		{
			if (pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			{
				m_vEnemy     = CBotGlobals::entityOrigin(pBot->getEnemy());
				m_fEnemyTime = engine->Time();

				if (m_fAimTime == 0.0f)
					m_fAimTime = engine->Time() + randomFloat(0.1f, 0.3f);

				// too close for sniper rifle
				if (pBot->distanceFrom(pBot->getEnemy()) < 200.0f)
				{
					complete();
					return;
				}
			}

			// careful that the round may have not started yet
			if (CTeamFortress2Mod::hasRoundStarted())
			{
				pBot->setMoveLookPriority(MOVELOOK_ATTACK);

				pBot->setLookAtTask(LOOK_ENEMY);

				float angle = pBot->DotProductFromOrigin(CBotGlobals::entityOrigin(pBot->getEnemy()));

				if (angle > 0.96f) // 16 degrees
				{
					if (m_fAimTime < engine->Time())
						pBot->handleAttack(pBotWeapon, pBot->getEnemy());

					if (m_iPrevClip > pBotWeapon->getAmmo(pBot))
					{
						bool bhide;

						// hide if player can see me, otherwise if not player hide if low health
						if (CBotGlobals::isPlayer(pBot->getEnemy()))
							bhide = CBotGlobals::DotProductFromOrigin(pBot->getEnemy(), pBot->getOrigin()) > 0.96f;
						else
							bhide = randomFloat(0.0f, 2.0f) <= (angle + (1.0f - pBot->getHealthPercent()));

						if (bhide)
							m_fHideTime = engine->Time() + randomFloat(1.5f, 2.5f);

						m_fAimTime  = 0.0f;
						m_iPrevClip = pBotWeapon->getAmmo(pBot);
					}
				}

				pBot->setMoveLookPriority(MOVELOOK_TASK);
			}
		}
		else
		{
			// time to check if bot is just looking at a wall or not
			if (m_fCheckTime < engine->Time())
			{
				CBotGlobals::quickTraceline(pBot->getEdict(), pBot->getOrigin(), pBot->getAiming());

				if (CBotGlobals::getTraceResult()->fraction < 0.1f)
				{
					m_fOriginDistance = 40.0f;
					m_fCheckTime      = engine->Time() + 0.3f;
				}
				else
				{
					m_fOriginDistance = 100.0f;
					m_fCheckTime      = engine->Time() + 0.15f;
				}
			}

			if (pBot->isHoldingPrimaryAttack())
			{
				// zoom in
				// if (!CTeamFortress2Mod::TF2_IsPlayerZoomed(pBot->getEdict()))
				//	pBot->secondaryAttack();
				// else
				m_fHideTime = 0.0f;
			}
		}
	}
}