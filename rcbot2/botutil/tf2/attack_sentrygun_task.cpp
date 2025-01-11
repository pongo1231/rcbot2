#include "attack_sentrygun_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_waypoint_locations.h"
#include "bot_waypoint_visibility.h"
#include "bot_weapons.h"

CBotTF2AttackSentryGunTask::CBotTF2AttackSentryGunTask(edict_t *pSentryGun, CBotWeapon *pWeapon)
{
	m_pSentryGun = pSentryGun;
	m_pWeapon    = pWeapon;
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

	if (m_fTime == 0.0f)
	{
		float fMinDist = 9999;
		float fDist;

		CWaypointVisibilityTable *table = CWaypoints::getVisiblity();

		m_fTime                         = engine->Time() + randomFloat(8.0f, 14.0f);

		m_iSentryWaypoint = CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun), 200.0f, -1);

		m_iStartingWaypoint =
		    CWaypointLocations::NearestWaypoint(pBot->getOrigin(), 200.0f, -1, true, true, true, nullptr);

		m_vStart        = pBot->getOrigin();
		m_vHide         = m_vStart;

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

	if (m_pWeapon->needToReload(pBot) || (CClassInterface::getSentryEnemy(m_pSentryGun) == pBot->getEdict()))
	{
		if (pBot->distanceFrom(m_vHide) > 80.0f)
			pBot->setMoveTo(m_vHide);
		else
		{
			if (m_pWeapon->needToReload(pBot))
			{
				if (randomInt(0, 1))
					pBot->reload();
			}
			pBot->stopMoving();
		}
	}
	else if (pBot->distanceFrom(m_vStart) > 80.0f)
		pBot->setMoveTo(m_vStart);
	else
		pBot->stopMoving();

	if (pBot->isVisible(m_pSentryGun))
	{
		// use this shooting method below
		pBot->wantToShoot(false);

		CBotTF2 *pTF2Bot = (CBotTF2 *)pBot;
		pTF2Bot->resetAttackingEnemy();
		// attack
		pBot->handleAttack(m_pWeapon, m_pSentryGun);
	}
}

void CBotTF2AttackSentryGunTask::debugString(char *string)
{
	sprintf(string, "CBotTF2AttackSentryGunTask");
}