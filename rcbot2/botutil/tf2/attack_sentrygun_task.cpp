#include "attack_sentrygun_task.h"

#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint_visibility.h"
#include "bot_weapons.h"

CBotTF2AttackSentryGunTask::CBotTF2AttackSentryGunTask(edict_t *pSentryGun, CBotWeapon *pWeapon)
{
	m_pSentryGun        = pSentryGun;
	m_pWeapon           = pWeapon;
	m_fFiringWindowTime = 0.0f;
	m_iStallFrames      = 0;
	m_bStrafeRight      = false;
}

void CBotTF2AttackSentryGunTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);
	pBot->wantToInvestigateSound(false);

	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
		fail();

	if (m_pSentryGun.get() == nullptr)
	{
		fail();
		return;
	}

	if (m_pWeapon == nullptr || !m_pWeapon->hasWeapon())
	{
		fail();
		return;
	}

	if (m_fTime == 0.0f)
	{
		float fMinDist = 9999;
		float fDist;

		CWaypointVisibilityTable *table = CWaypoints::getVisiblity();

		m_fTime                         = engine->Time() + randomFloat(8.0f, 14.0f);

		m_iSentryWaypoint = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun), 200.0f, -1);

		m_iStartingWaypoint =
		    CWaypointLocations::NearestWaypoint(pBot->getOrigin(), 200.0f, -1, true, true, true, nullptr);

		m_vStart = pBot->getOrigin();

		if (m_pWeapon->primaryMaxRange() > TF2_MAX_SENTRYGUN_RANGE + 100
		    && pBot->distanceFrom(m_pSentryGun) < TF2_MAX_SENTRYGUN_RANGE)
		{
			Vector vAway = pBot->getOrigin() - CBotGlobals::entityOrigin(m_pSentryGun);
			vAway.z = 0;
			if (vAway.Length() > 0.1f)
			{
				vAway = vAway / vAway.Length();
				m_vStart = pBot->getOrigin() + (vAway * 384.0f);
			}
		}

		m_vHide         = m_vStart;
		m_iStallFrames  = 0;
		m_fFiringWindowTime = 0.0f;

		Vector vToSentry = CBotGlobals::entityOrigin(m_pSentryGun) - m_vStart;
		vToSentry.z      = 0;
		float fToLen     = vToSentry.Length();
		if (fToLen > 0.1f)
		{
			m_vPerpDir.x = -vToSentry.y / fToLen;
			m_vPerpDir.y =  vToSentry.x / fToLen;
			m_vPerpDir.z = 0;
		}
		else
			m_vPerpDir = Vector(1, 0, 0);
		m_bStrafeRight = (randomInt(0, 1) == 1);

		CWaypoint *pWpt = CWaypoints::getWaypoint(m_iStartingWaypoint);

		if (pWpt != nullptr)
		{
			for (int i = 0; i < pWpt->numPaths(); i++)
			{
				if (table->GetVisibilityFromTo(pWpt->getPath(i), m_iSentryWaypoint) == false)
				{
					CWaypoint *pPath = CWaypoints::getWaypoint(pWpt->getPath(i));

					if ((fDist = pPath->distanceFrom(m_vStart)) < fMinDist)
					{
						fMinDist = fDist;
						m_vHide  = pPath->getOrigin();
					}
				}

				pWpt->getPath(i);
			}
		}

		// hide waypoint =
	}
	else if (m_fTime < engine->Time())
	{
		if (CBotGlobals::entityIsValid(m_pSentryGun))
			CTeamFortress2Mod::updateFocusPoint(
			    CBotGlobals::entityOrigin(m_pSentryGun), true, false);
		complete();
	}

	if (pBot->getCurrentWeapon() != m_pWeapon)
	{
		pBot->selectBotWeapon(m_pWeapon);
		return;
	}

	pBot->wantToChangeWeapon(false);

	if (m_pWeapon->outOfAmmo(pBot))
		complete();

	pBot->lookAtEdict(m_pSentryGun);
	pBot->setLookAtTask(LOOK_EDICT);

	bool bTakingFire  = (CClassInterface::getSentryEnemy(m_pSentryGun) == pBot->getEdict());
	bool bOutOfRange  = pBot->distanceFrom(m_pSentryGun) > TF2_MAX_SENTRYGUN_RANGE;
	bool bOverhealed  = pBot->getHealthPercent() > 1.3f;
	bool bHurtBySentry = pBot->recentlyHurt(1.0f) && bTakingFire;

	float fSentryHealth    = CClassInterface::getSentryHealth(m_pSentryGun);
	float fSentryMaxHealth = CClassInterface::getTF2GetBuildingMaxHealth(m_pSentryGun);
	bool bSentryNearDeath  = (fSentryMaxHealth > 0
	                          && fSentryHealth / fSentryMaxHealth < 0.2f);
	bool bLowLevelSentry   = (CTeamFortress2Mod::getSentryLevel(m_pSentryGun) < 2);
	float fHideThreshold   = bLowLevelSentry ? 0.3f : (bSentryNearDeath ? 0.25f : 0.5f);

	float fDistToStart = pBot->distanceFrom(m_vStart);
	float fDistToHide  = pBot->distanceFrom(m_vHide);
	float fStartThreshold = (m_pWeapon->primaryMaxRange() <= TF2_MAX_SENTRYGUN_RANGE + 100) ? 200.0f : 80.0f;

	if (fDistToStart < fStartThreshold && m_fFiringWindowTime == 0.0f)
		m_fFiringWindowTime = engine->Time() + 1.0f;
	else if (fDistToStart >= fStartThreshold)
		m_fFiringWindowTime = 0.0f;

	bool bInFiringWindow = (m_fFiringWindowTime > engine->Time());

	// Retreat if taking too much damage
	if (bTakingFire && pBot->getHealthPercent() < fHideThreshold)
		fail();

	// Hide when reloading, taking fire (unless overhealed or out of range),
	// or recently hurt by sentry
	bool bShouldHide = m_pWeapon->needToReload(pBot)
	                   || (bTakingFire && !bOverhealed && !bOutOfRange
	                       && !bInFiringWindow)
	                   || (bHurtBySentry && !bSentryNearDeath && !bInFiringWindow);

	if (bShouldHide)
	{
		if (fDistToHide > 80.0f)
		{
			pBot->setMoveTo(m_vHide);
			m_iStallFrames = 0;
			m_vLastPos = pBot->getOrigin();
		}
		else
		{
			if (m_pWeapon->needToReload(pBot))
			{
				if (randomInt(0, 1))
					pBot->reload();
			}

			if (m_pWeapon->getID() == TF2_WEAPON_ROCKETLAUNCHER)
			{
				edict_t *pWepEnt = m_pWeapon->getWeaponEntity();
				if (pWepEnt
				    && CClassInterface::TF2_getItemDefinitionIndex(pWepEnt) == 730
				    && m_pWeapon->getClip1(pBot) < 3)
				{
					pBot->primaryAttack(true);
				}
			}

			pBot->stopMoving();
		}
	}
	else
	{
		m_iStallFrames = 0;

		if (fDistToStart > fStartThreshold)
		{
			pBot->setMoveTo(m_vStart);
		}
		else
		{
			Vector vTarget = m_vStart + m_vPerpDir * (m_bStrafeRight ? 150.0f : -150.0f);
			if (pBot->distanceFrom(vTarget) < 50.0f)
				m_bStrafeRight = !m_bStrafeRight;
			pBot->setMoveTo(vTarget);
		}
	}

	if (!bShouldHide && fDistToHide < 80.0f)
	{
		float fNewDist = (pBot->getOrigin() - m_vLastPos).Length();
		if (fNewDist < 10.0f)
			m_iStallFrames++;
		else
			m_iStallFrames = 0;
		m_vLastPos = pBot->getOrigin();
		if (m_iStallFrames > 3)
			pBot->setMoveTo(m_vStart);
	}

	if (pBot->isVisible(m_pSentryGun))
	{
		// use this shooting method below
		pBot->wantToShoot(false);

		CBotTF2 *pTF2Bot = (CBotTF2 *)pBot;
		pTF2Bot->resetAttackingEnemy();
		// attack
		if (!pBot->handleAttack(m_pWeapon, m_pSentryGun))
			complete();
	}
}

void CBotTF2AttackSentryGunTask::debugString(char *string)
{
	sprintf(string, "CBotTF2AttackSentryGunTask");
}