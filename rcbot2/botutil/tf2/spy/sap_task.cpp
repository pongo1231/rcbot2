#include "sap_task.h"

#include "backstap_sched.h"
#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_weapons.h"

#include <in_buttons.h>

CBotTF2SpySap::CBotTF2SpySap(edict_t *pBuilding, eEngiBuild id)
{
	m_pBuilding       = MyEHandle(pBuilding);
	m_fTime           = 0.0f;
	m_fEvadeTime      = 0.0f;
	m_fStrafeTime     = 0.0f;
	m_fStateChangeTime = 0.0f;
	m_id              = id;
	m_iState          = SAP_APPROACH;
	m_bEvadeRight     = false;
	m_bSapperPlaced   = false;
	m_fDecloakTime    = 0.0f;
	m_fSapRetryTime   = 0.0f;
	m_iSapAttempts    = 0;
	m_vBuildingOrigin = CBotGlobals::entityOrigin(pBuilding);
}

bool CBotTF2SpySap::buildingIsSapped(edict_t *pBuilding)
{
	if (m_id == ENGI_SENTRY)
		return CTeamFortress2Mod::isSentrySapped(pBuilding);
	else if (m_id == ENGI_DISP)
		return CTeamFortress2Mod::isDispenserSapped(pBuilding);
	else if (m_id == ENGI_TELE)
		return CTeamFortress2Mod::isTeleporterSapped(pBuilding);
	else if (m_id == ENGI_ROBOT)
	{
		// Check if this robot is in our victim's sapped list
		CBotTF2 *tf2Bot = (CBotTF2 *)nullptr; // filled in execute()
		return false; // tracked via completion logic below
	}
	return false;
}

void CBotTF2SpySap::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pBuilding;
	CBotTF2 *tf2Bot = (CBotTF2 *)pBot;

	if (!pBot->isTF())
	{
		fail();
		return;
	}

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + randomFloat(4.0f, 6.0f);
		tf2Bot->resetCloakTime();
	}

	CBotWeapon *weapon;
	pBot->wantToShoot(false);

	pBuilding = m_pBuilding.get();

	if (!pBuilding)
	{
		complete();
		return;
	}

	// Update building origin for evade movement
	m_vBuildingOrigin = CBotGlobals::entityOrigin(pBuilding);

	if (m_iState == SAP_EVADE)
	{
		// Cloak after placing sapper — only once per 0.5s to avoid toggle oscillation
		if (!CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict())
		    && m_fDecloakTime < engine->Time())
		{
			tf2Bot->resetCloakTime();
			tf2Bot->spyCloak();
			m_fDecloakTime = engine->Time() + 0.5f;
		}

		// Switch to knife so spy isn't stuck holding the sapper
		if (pBot->getCurrentWeapon() && pBot->getCurrentWeapon()->getID() != TF2_WEAPON_KNIFE)
			pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE));

		// Sapper was removed -- go back to approach (with debounce to prevent oscillation)
		if (!buildingIsSapped(pBuilding) && (m_fStateChangeTime + 0.5f) < engine->Time())
		{
			m_iState           = SAP_APPROACH;
			m_fTime            = engine->Time() + randomFloat(4.0f, 6.0f);
			m_fStateChangeTime = engine->Time();
			return;
		}

		// Init evade timer on first entry
		if (m_fEvadeTime == 0.0f)
			m_fEvadeTime = engine->Time() + randomFloat(2.5f, 3.5f);

		// Complete after evade period -- let utility evaluation pick next action
		if (m_fEvadeTime < engine->Time())
		{
			// For robots: add backstab on the same target immediately after sap
			if (m_id == ENGI_ROBOT && CBotGlobals::entityIsAlive(pBuilding))
			{
				pBot->getSchedule()->freeMemory();
				pBot->getSchedule()->add(new CBotBackstabSched(pBuilding));
				complete();
				return;
			}

			// Chain into backstab if any visible enemy is nearby
			if (m_id != ENGI_ROBOT
			    && pBot->getEnemy()
			    && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
			    && CBotGlobals::isAlivePlayer(pBot->getEnemy())
			    && pBot->distanceFrom(pBot->getEnemy()) < 500.0f)
			{
				pBot->getSchedule()->add(new CBotBackstabSched(pBot->getEnemy()));
				complete();
				return;
			}

			// Continue moving away briefly after completing
			Vector vAway = pBot->getOrigin() - m_vBuildingOrigin;
			vAway.z      = 0;
			if (vAway.Length() < 200.0f)
			{
				vAway = vAway.Length() > 0.1f ? vAway / vAway.Length() : Vector(1, 0, 0);
				pBot->setMoveTo(pBot->getOrigin() + (vAway * 400.0f));
			}
			complete();
			return;
		}

		// If engineer is alone and sentry is sapped, let backstab utility take over
		if (pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
		    && CBotGlobals::isAlivePlayer(pBot->getEnemy())
		    && (CClassInterface::getTF2Class(pBot->getEnemy()) == TF_CLASS_ENGINEER
		        || CClassInterface::getTF2Class(pBot->getEnemy()) == TF_CLASS_MEDIC
		        || CClassInterface::getTF2Class(pBot->getEnemy()) == TF_CLASS_SNIPER))
		{
			pBot->updateCondition(CONDITION_CHANGED);
			complete();
			return;
		}

		// Strafe around near the building to stay mobile and avoid getting stuck
		if (m_fStrafeTime < engine->Time())
		{
			m_fStrafeTime = engine->Time() + randomFloat(1.5f, 2.5f);
			m_bEvadeRight = !m_bEvadeRight;
		}

		Vector vFromBuilding = pBot->getOrigin() - m_vBuildingOrigin;
		vFromBuilding.z      = 0;
		float fDist          = vFromBuilding.Length();

		if (fDist < 60.0f || fDist > 400.0f)
		{
			// Too close or too far -- escape to ~350 units from building
			Vector vDir = vFromBuilding;
			if (fDist < 0.1f)
				vDir = Vector(1, 0, 0);
			else
				vDir = vDir / fDist;

			pBot->setMoveTo(m_vBuildingOrigin + (vDir * 350.0f));
		}
		else
		{
			// Strafe around the building aggressively to evade
			Vector vPerp = vFromBuilding.Cross(Vector(0, 0, 1));
			if (vPerp.Length() > 0.1f)
			{
				vPerp          = vPerp / vPerp.Length();
				float fOff     = 120.0f;
				Vector vTarget = m_vBuildingOrigin + vFromBuilding;
				if (m_bEvadeRight)
					vTarget = vTarget + (vPerp * fOff);
				else
					vTarget = vTarget - (vPerp * fOff);
				pBot->setMoveTo(vTarget);
			}
		}

		pBot->setLookAtTask(LOOK_AROUND);
		return;
	}

	// SAP_APPROACH state
	// Transition to EVADE only when the building is confirmed sapped
	if (buildingIsSapped(pBuilding))
	{
		m_iState           = SAP_EVADE;
		m_fEvadeTime       = 0.0f;
		m_fStateChangeTime = engine->Time();
		m_bSapperPlaced    = false;

		// Alert teammates: sentry/building is sapped, push now!
		((CBotTF2 *)pBot)->addVoiceCommand(TF_VC_GOGOGO);
		CTeamFortress2Mod::onSapAtFocusPoint(m_vBuildingOrigin);
		return;
	}

	// For robots: transition after placing sapper (game doesn't report sap state)
	if (m_id == ENGI_ROBOT && m_bSapperPlaced)
	{
		// Add to victim's sapped robot list so other spies don't target it
		((CBotTF2 *)pBot)->addSappedRobot(pBuilding);
		m_iState           = SAP_EVADE;
		m_fEvadeTime       = 0.0f;
		m_fStateChangeTime = engine->Time();
		m_bSapperPlaced    = false;
		return;
	}

	// Give up after too many failed sap attempts
	if (m_iSapAttempts >= 5 || m_fTime < engine->Time())
	{
		fail();
		return;
	}

	pBot->lookAtEdict(pBuilding);
	pBot->setLookAtTask(LOOK_EDICT, 0.2f);
	weapon = tf2Bot->getCurrentWeapon();

	if (!weapon || (weapon->getID() != TF2_WEAPON_BUILDER))
	{
		helpers->ClientCommand(pBot->getEdict(), "build 3 0");
	}
	else if (pBot->distanceFrom(pBuilding) > 100)
	{
		if (!CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict())
		    && m_fDecloakTime < engine->Time())
		{
			tf2Bot->spyCloak();
			m_fDecloakTime = engine->Time() + 0.5f;
		}
		pBot->setMoveTo(m_vBuildingOrigin);
	}
	else
	{
		if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pBot->getEdict()))
		{
			if (m_fDecloakTime < engine->Time())
			{
				tf2Bot->resetCloakTime();
				tf2Bot->spyUnCloak();
				m_fDecloakTime = engine->Time() + 0.5f;
			}
		}
		else if (m_fSapRetryTime < engine->Time())
		{
			pBot->tapButton(IN_ATTACK);
			m_bSapperPlaced = true;
			m_fSapRetryTime = engine->Time() + 0.5f;
			m_iSapAttempts++;
		}
	}
}

void CBotTF2SpySap::debugString(char *string)
{
	sprintf(string, "CBotTF2SpySap");
}
