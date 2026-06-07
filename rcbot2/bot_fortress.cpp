/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "bot_fortress.h"

#include "bot.h"
#include "bot_buttons.h"
#include "bot_configfile.h"
#include "bot_cvars.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_mtrand.h"
#include "bot_navigator.h"
#include "bot_plugin_meta.h"
#include "bot_profile.h"
#include "bot_squads.h"
#include "bot_utility.h"
#include "bot_visibles.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_weapons.h"
#include "bot_wpt_dist.h"
#include "botutil/shared/tasks.h"
#include "botutil/tf2/tasks.h"

#include <in_buttons.h>
#include <ndebugoverlay.h>
// #include "bot_hooks.h"

// caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

extern IVDebugOverlay *debugoverlay;

#define TF2_SPY_CLOAK_BELIEF 40
#define TF2_HWGUY_REV_BELIEF 60
// extern float g_fBotUtilityPerturb [TF_CLASS_MAX][BOT_UTIL_MAX];

// Payload stuff by   The_Shadow

// #include "vstdlib/random.h" // for random functions

void CBroadcastOvertime::execute(CBot *pBot)
{
	pBot->updateCondition(CONDITION_CHANGED);
	pBot->updateCondition(CONDITION_PUSH);
}
void CBroadcastCapturedPoint::execute(CBot *pBot)
{
	((CBotTF2 *)pBot)->pointCaptured(m_iPoint, m_iTeam, m_szName);
}

CBroadcastCapturedPoint::CBroadcastCapturedPoint(int iPoint, int iTeam, const char *szName)
{
	m_iPoint = iPoint;
	m_iTeam  = iTeam;
	m_szName = CStrings::getString(szName);
}

void CBroadcastSpySap::execute(CBot *pBot)
{
	if (CTeamFortress2Mod::getTeam(m_pSpy) != pBot->getTeam())
	{
		if (pBot->isVisible(m_pSpy))
			((CBotTF2 *)pBot)->foundSpy(m_pSpy, CTeamFortress2Mod::getSpyDisguise(m_pSpy));
	}
}
// special delivery
void CBroadcastFlagReturned::execute(CBot *pBot)
{
	// if ( pBot->getTeam() == m_iTeam )
	//((CBotTF2*)pBot)->flagReturned_SD(m_vOrigin);
	// else
	//	((CBotTF2*)pBot)->teamFlagDropped(m_vOrigin);
}

void CBroadcastFlagDropped::execute(CBot *pBot)
{
	if (pBot->getTeam() == m_iTeam)
		((CBotTF2 *)pBot)->flagDropped(m_vOrigin);
	else
		((CBotTF2 *)pBot)->teamFlagDropped(m_vOrigin);
}
// flag picked up
void CBotTF2FunctionEnemyAtIntel::execute(CBot *pBot)
{
	if (m_iType == EVENT_CAPPOINT)
	{
		if (CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(m_iCapIndex) != pBot->getTeam())
			return;
	}

	pBot->updateCondition(CONDITION_PUSH);

	if (pBot->getTeam() != m_iTeam)
		((CBotTF2 *)pBot)->enemyAtIntel(m_vPos, m_iType, m_iCapIndex);
	else
		((CBotTF2 *)pBot)->teamFlagPickup();
}

void CBotTF2::hearVoiceCommand(edict_t *pPlayer, byte cmd)
{
	switch (cmd)
	{
	case TF_VC_SPY:
		// someone shouted spy, HACK the bot to think they saw a spy here too
		// for spy checking purposes
		if (isVisible(pPlayer))
		{
			m_vLastSeeSpy = CBotGlobals::entityOrigin(pPlayer);
			m_fSeeSpyTime = engine->Time() + randomFloat(3.0f, 6.0f);
			// m_pPrevSpy = pPlayer; // HACK
		}
		break;
	// somebody shouted "MEDIC!"
	case TF_VC_MEDIC:
		medicCalled(pPlayer);
		break;
	case TF_VC_SENTRYHERE: // hear 'put sentry here'
		// Also handles non-carrying case via handleBuildRequest
		if (getClass() == TF_CLASS_ENGINEER)
		{
			if (m_bIsCarryingObj && m_bIsCarryingSentry)
			{
				if (isVisible(pPlayer) && (distanceFrom(pPlayer) < 512))
				{
					if (randomInt(0, 100) > 75)
						addVoiceCommand(TF_VC_YES);
					primaryAttack();
					m_pSchedules->removeSchedule(SCHED_TF2_ENGI_MOVE_BUILDING);
				}
				else if (randomInt(0, 100) > 75)
					addVoiceCommand(TF_VC_NO);
			}
			else
				handleBuildRequest(ENGI_SENTRY, CWaypointTypes::W_FL_SENTRY, pPlayer);
		}
		break;
	case TF_VC_DISPENSERHERE:
		handleBuildRequest(ENGI_DISP, CWaypointTypes::W_FL_SENTRY, pPlayer);
		break;
	case TF_VC_TELEPORTERHERE:
		// Only place teleporter exit if not near spawn
	{
		int iSpawnWpt = CWaypointLocations::NearestWaypoint(
			CBotGlobals::entityOrigin(pPlayer), 800, -1, false, false, false,
			nullptr, false, 0, true, false, Vector(0, 0, 0), CWaypointTypes::W_FL_TELE_EXIT);
		if (iSpawnWpt == -1 || (CBotGlobals::entityOrigin(pPlayer)
			- CWaypoints::getWaypoint(iSpawnWpt)->getOrigin()).Length() > 512.0f)
		{
			handleBuildRequest(ENGI_EXIT, CWaypointTypes::W_FL_TELE_EXIT, pPlayer);
		}
	}
		break;
	case TF_VC_HELP:
		break;
	case TF_VC_GOGOGO:
		// if bot is nesting, or waiting for something, it will go
		if (distanceFrom(pPlayer) > 512)
			return;

		updateCondition(CONDITION_PUSH);

		if (randomFloat(0, 1.0) > 0.75f)
			m_nextVoicecmd = TF_VC_YES;

		// don't break // flow down to uber if medic
	case TF_VC_ACTIVATEUBER:
		if (CTeamFortress2Mod::hasRoundStarted() && (getClass() == TF_CLASS_MEDIC))
		{
			if (m_pHeal == pPlayer)
			{
				if (!CTeamFortress2Mod::isFlagCarrier(pPlayer))
					secondaryAttack();
				else if (randomFloat(0, 1.0) > 0.5f)
					m_nextVoicecmd = TF_VC_NO;
			}
		}
		break;
	case TF_VC_MOVEUP:

		if (distanceFrom(pPlayer) > 1000)
			return;

		updateCondition(CONDITION_PUSH);

		if (randomFloat(0, 1.0) > 0.75f)
			m_nextVoicecmd = TF_VC_YES;

		break;
	default:
		break;
	}
}

void CBroadcastFlagCaptured::execute(CBot *pBot)
{
	if (pBot->getTeam() == m_iTeam)
		((CBotTF2 *)pBot)->flagReset();
	else
		((CBotTF2 *)pBot)->teamFlagReset();
}

void CBroadcastRoundStart::execute(CBot *pBot)
{
	((CBotTF2 *)pBot)->roundReset(m_bFullReset);
}

CBotFortress::CBotFortress()
{
	CBot();

	m_iLastFailSentryWpt      = -1;
	m_iLastFailTeleExitWpt    = -1;

	// remember prev spy disguised in game while playing
	m_iPrevSpyDisguise        = (TF_Class)0;

	m_fSentryPlaceTime        = 0;
	m_iSentryKills            = 0;
	m_fSnipeAttackTime        = 0;
	m_pAmmo                   = nullptr;
	m_pHealthkit              = nullptr;
	m_pFlag                   = nullptr;
	m_pHeal                   = nullptr;
	m_fCallMedic              = 0;
	m_fTauntTime              = 0;
	m_fLastKnownFlagTime      = 0.0f;
	m_bHasFlag                = false;
	m_pSentryGun              = nullptr;
	m_pDispenser              = nullptr;
	m_pTeleExit               = nullptr;
	m_pTeleEntrance           = nullptr;
	m_pNearestDisp            = nullptr;
	m_pNearestEnemySentry     = nullptr;
	m_pNearestEnemyTeleporter = nullptr;
	m_pNearestEnemyDisp       = nullptr;
	m_pNearestPipeGren        = nullptr;
	m_pPrevSpy                = nullptr;
	m_fSeeSpyTime             = 0.0f;
	m_bEntranceVectorValid    = false;
	m_pLastCalledMedic        = nullptr;
	m_fLastCalledMedicTime    = 0.0f;
	m_bIsBeingHealed          = false;
	m_bCanBeUbered            = false;
	m_bRevived                = false;
}

void CBotFortress::checkDependantEntities()
{
	CBot::checkDependantEntities();
}

void CBotFortress::init(bool bVarInit)
{
	CBot::init(bVarInit);

	m_bCheckClass = false;
	m_bHasFlag    = false;
	m_iClass      = TF_CLASS_MAX; // important
}

void CBotFortress::setup()
{
	CBot::setup();

	// Bot comfort settings
	helpers->ClientCommand(m_pEdict, "cl_autoreload 1");
	helpers->ClientCommand(m_pEdict, "hud_medicautocallers 1");
	if (getClass() == TF_CLASS_MEDIC)
		helpers->ClientCommand(m_pEdict, "tf_medigun_autoheal 1");
}

bool CBotFortress::someoneCalledMedic()
{
	return (getClass() == TF_CLASS_MEDIC) && (m_pLastCalledMedic.get() != nullptr)
	    && ((m_fLastCalledMedicTime + 30.0f) > engine->Time());
}

bool CBotTF2::sentryRecentlyHadEnemy()
{
	return (m_fLastSentryEnemyTime + 15.0f) > engine->Time();
}

bool CBotFortress::startGame()
{
	int team = m_pPlayerInfo->GetTeamIndex();

	m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);

	// Hijacked bots: preserve the player's class, don't change it
	if (m_bHijacked)
		return (team == TF2_TEAM_BLUE || team == TF2_TEAM_RED) && m_iClass != TF_CLASS_MAX;

	if ((team != TF2_TEAM_BLUE) && (team != TF2_TEAM_RED))
		selectTeam();
	else if (m_iDesiredClass == -1) // invalid class
		chooseClass();
	else if ((m_iDesiredClass > 0 && (m_iClass != m_iDesiredClass)) || (m_iClass == TF_CLASS_MAX))
		selectClass();
	else
		return true;

	return false;
}

void CBotFortress::pickedUpFlag()
{
	m_bHasFlag = true;
	// clear tasks
	m_pSchedules->freeMemory();
}

void CBotFortress::checkHealingValid()
{
	if (m_pHeal)
	{
		if (!CBotGlobals::entityIsValid(m_pHeal) || !CBotGlobals::entityIsAlive(m_pHeal))
		{
			m_pHeal = nullptr;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if (!isVisible(m_pHeal))
		{
			m_pHeal = nullptr;
			removeCondition(CONDITION_SEE_HEAL);
		}
		else if (getHealFactor(m_pHeal) == 0.0f)
		{
			m_pHeal = nullptr;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}
	else
		removeCondition(CONDITION_SEE_HEAL);
}

float CBotFortress::getHealFactor(edict_t *pPlayer)
{
	// Factors are
	// 1. health
	// 2. max health
	// 3. ubercharge
	// 4. player class
	// 5. etc
	float fFactor          = 0.0f;
	float fLastCalledMedic = 0.0f;
	bool bHeavyClass       = false;
	edict_t *pMedigun      = CTeamFortress2Mod::getMediGun(m_pEdict);
	float fHealthPercent;
	Vector vVel       = Vector(0, 0, 0);
	int iHighestScore = CTeamFortress2Mod::getHighestScore();
	// adds extra factor to players who have recently shouted MEDIC!
	if (!CBotGlobals::isPlayer(pPlayer))
	{
		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && pPlayer)
		{
			if (strcmp(pPlayer->GetClassName(), "entity_revive_marker") == 0)
			{
				float fDistance = distanceFrom(pPlayer);

				if (fDistance < 0.1)
					return 1000;

				float fFactor = 200.0f / fDistance;

				// In danger mode, deprioritize markers so living teammates under 50% HP get healed first
				bool bInDanger = (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());
				if (bInDanger)
					fFactor *= 0.33f;

				return fFactor;
			}
		}

		return 0;
	}
	CClassInterface::getVelocity(pPlayer, &vVel);

	IPlayerInfo *p  = playerinfomanager->GetPlayerInfo(pPlayer);
	TF_Class iclass = (TF_Class)CClassInterface::getTF2Class(pPlayer);

	if (!CBotGlobals::entityIsAlive(pPlayer) || !p || p->IsDead() || p->IsObserver() || !p->IsConnected())
		return 0.0f;

	// Ignore AFK players at spawn so medics don't stand around healing idle players
	if (isPlayerAFK(pPlayer))
		return 0.0f;

	if (CClassInterface::getTF2NumHealers(pPlayer) > 1)
		return 0.0f;

	fHealthPercent = (p->GetHealth() / p->GetMaxHealth());

	switch (iclass)
	{
	case TF_CLASS_MEDIC:

		if (fHealthPercent >= 1.0f)
			return 0.0f;

		fFactor = 0.1f;

		break;
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_HWGUY:
	case TF_CLASS_SOLDIER:
	case TF_CLASS_PYRO:
	{
		bHeavyClass = true;

		fFactor += 1.0f;

		if (pMedigun)
		{
			// overheal HWGUY/SOLDIER/DEMOMAN
			fFactor += (float)(CClassInterface::getUberChargeLevel(pMedigun)) / 100;

			if (CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)) // uber deployed
				fFactor += (1.0f - ((float)(CClassInterface::getUberChargeLevel(pMedigun)) / 100));
		}
	}
		// drop down
	case TF_CLASS_SPY:
		if (iclass == TF_CLASS_SPY)
		{
			int iClass, iTeam, iIndex, iHealth;

			if (CClassInterface::getTF2SpyDisguised(pPlayer, &iClass, &iTeam, &iIndex, &iHealth))
			{
				if (iTeam != m_iTeam)
					return 0.0f;
			}

			if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer))
				return 0.0f;
		}
	default:

		if (!bHeavyClass && pMedigun) // add more factor bassed on uber charge level - bot can gain more uber charge
		{
			fFactor += (0.1f - ((float)(CClassInterface::getUberChargeLevel(pMedigun)) / 1000));

			if ((m_StatsCanUse.stats.m_iTeamMatesVisible == 1) && (fHealthPercent >= 1.0f))
				return 0.0f; // find another guy
		}

		if (bHeavyClass)
		{
			IPlayerInfo *p;

			p = playerinfomanager->GetPlayerInfo(pPlayer);

			if (p)
			{
				if (p->GetLastUserCommand().buttons & IN_ATTACK)
					fFactor += 1.0f;
			}
		}

		fFactor += 1.0f - fHealthPercent;

		if (CTeamFortress2Mod::TF2_IsPlayerOnFire(pPlayer))
			fFactor += 2.0f;

		// higher movement better
		fFactor += (vVel.Length() / 1000);

		// favour big guys
		fFactor += ((float)p->GetMaxHealth()) / 200;

		// favour those with bigger scores
		if (iHighestScore == 0)
			iHighestScore = 1;

		fFactor += (((float)CClassInterface::getTF2Score(pPlayer)) / iHighestScore) / 2;

		if ((fLastCalledMedic = m_fCallMedicTime[ENTINDEX(pPlayer) - 1]) > 0)
			fFactor += MAX(0.0f, 1.0f - ((engine->Time() - fLastCalledMedic) / 5));
		if (((m_fLastCalledMedicTime + 5.0f) > engine->Time()) && (m_pLastCalledMedic == pPlayer))
			fFactor += 0.5f;

		// More priority to flag carriers and cappers
	if (CTeamFortress2Mod::isFlagCarrier(pPlayer) || CTeamFortress2Mod::isCapping(pPlayer))
		fFactor *= 1.5f;

	// In danger mode, boost players below 50% HP to out-prioritize revive markers
	if (fHealthPercent < 0.5f && CTeamFortress2Mod::isMapType(TF_MAP_MVM))
	{
		bool bInDanger = (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());
		if (bInDanger)
			fFactor *= 2.0f;
	}

	// When carrying the flag, only heal teammates below max health
	if (hasFlag() && fHealthPercent >= 1.0f)
		return 0.0f;
	}

	return fFactor;
}

/////////////////////////////////////////////////////////////////////
//
// When a new Entity becomes visible or Invisible this is called
//
// bVisible = true when pEntity is Visible
// bVisible = false when pEntity becomes inVisible
bool CBotFortress::setVisible(edict_t *pEntity, bool bVisible)
{
	bool bValid = CBot::setVisible(pEntity, bVisible);

	// check for people to heal
	if (m_iClass == TF_CLASS_MEDIC)
	{
		if (bValid && bVisible)
		{
			if (CBotGlobals::isPlayer(pEntity)) // player
			{
				CBotWeapon *pMedigun = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MEDIGUN));
				bool bIsSpy          = CClassInterface::getTF2Class(pEntity) == TF_CLASS_SPY;
				int iDisguise        = 0;

				if (bIsSpy)
					CClassInterface::getTF2SpyDisguised(pEntity, &iDisguise, nullptr, nullptr, nullptr);

				if (pMedigun && pMedigun->hasWeapon()
				    && ( // Heal my team member or a spy if I think he is on my team
				        (CBotGlobals::getTeam(pEntity) == getTeam())
				        || ((bIsSpy && !thinkSpyIsEnemy(pEntity, (TF_Class)iDisguise)))))
				{
					Vector vPlayer = CBotGlobals::entityOrigin(pEntity);

					if (distanceFrom(vPlayer) <= CWaypointLocations::REACHABLE_RANGE)
					{
						float fFactor;

						if ((fFactor = getHealFactor(pEntity)) > 0)
						{
							if (m_pHeal.get() != nullptr)
							{
								if (m_pHeal != pEntity)
								{
									bool bHealingMarker = (m_pHeal.get() != nullptr
									    && !CBotGlobals::isPlayer(m_pHeal));
									bool bUnderThreat   = (m_pEnemy
									    && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());
									if (bHealingMarker)
									{
										// If beam is connected to marker, finish the revive -- don't switch
										edict_t *pGunEdict = pMedigun ? pMedigun->getWeaponEntity() : nullptr;
										bool bBeamConnected = pGunEdict
										    && (CClassInterface::getMedigunTarget(pGunEdict) == m_pHeal.get());
										if (bBeamConnected)
											return true;

										// Not yet beaming -- stick with revive unless under threat or player is critically wounded
										IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEntity);
										float fHP = pInfo ? (pInfo->GetHealth() / pInfo->GetMaxHealth()) : 1.0f;
										if (!bUnderThreat && fHP >= 0.5f)
											return true;
									}

									if (fFactor > m_fHealFactor)
									{
										m_fHealFactor = fFactor;
										m_pHeal       = pEntity;
										updateCondition(CONDITION_SEE_HEAL);
										notePlayerEngaged(pEntity);
									}
								}
								else
								{
									// not healing -- what am I doing?
									if (!m_pSchedules->hasSchedule(SCHED_HEAL))
									{
										// not healing -- what am I doing?
										m_pSchedules->freeMemory();
										m_pSchedules->addFront(new CBotTF2HealSched(m_pHeal));
			}
		}

		// Also avoid known sentry positions we're not actively engaging
		if (!m_pSchedules->hasSchedule(SCHED_ATTACK_SENTRY_GUN)
		    && !m_pSchedules->isCurrentSchedule(SCHED_ATTACK_SENTRY_GUN))
		{
			for (auto &h : m_KnownSentries)
			{
				edict_t *pKnown = h.get();
				if (!pKnown || !CBotGlobals::entityIsValid(pKnown)
				    || !CBotGlobals::entityIsAlive(pKnown)) continue;
				float fDist = distanceFrom(pKnown);
				if (fDist < TF2_MAX_SENTRYGUN_RANGE)
				{
					Vector vPos = CBotGlobals::entityOrigin(pKnown);
					Vector vAway = getOrigin() - vPos;
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(getOrigin() + (vAway * 384.0f));
						break;
					}
				}
			}
		}
	}
							else
							{
								m_fHealFactor = fFactor;
								m_pHeal       = pEntity;
								updateCondition(CONDITION_SEE_HEAL);
								notePlayerEngaged(pEntity);

								if (!m_pSchedules->hasSchedule(SCHED_HEAL))
								{
									// not healing -- what am I doing?
									m_pSchedules->freeMemory();
									m_pSchedules->addFront(new CBotTF2HealSched(m_pHeal));
								}
							}
						}
					}
				}
			}
			else
			{
				// I can see player recently shouted MEDIC! (and this is an MVM map)
				if (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
				    && strcmp(pEntity->GetClassName(), "entity_revive_marker") == 0)
				{
					// Skip if another medic is already reviving this marker
					bool bOtherMedicHasIt = false;
					for (int i = 1; i <= CBotGlobals::maxClients(); i++)
					{
						edict_t *pOther = INDEXENT(i);
						if (pOther && pOther != m_pEdict && CBotGlobals::entityIsValid(pOther)
						    && CClassInterface::getTeam(pOther) == getTeam()
						    && CClassInterface::getTF2Class(pOther) == TF_CLASS_MEDIC
						    && CBotGlobals::entityIsAlive(pOther))
						{
							CBot *pOtherBot = CBots::getBotPointer(pOther);
							if (pOtherBot && ((CBotTF2 *)pOtherBot)->getHealingEntity() == pEntity)
							{
								bOtherMedicHasIt = true;
								break;
							}
						}
					}
					if (bOtherMedicHasIt)
						return true;

					float fFactor = getHealFactor(pEntity);
					// add extra factor and ensure this guys actually being healed
					if (!m_pHeal || (m_pHeal == pEntity) || (fFactor > m_fHealFactor))
					{
						m_fHealFactor = fFactor;
						m_pHeal       = pEntity;
						updateCondition(CONDITION_SEE_HEAL);

						if (!m_pSchedules->hasSchedule(SCHED_HEAL))
						{
							// not healing -- what am I doing?
							m_pSchedules->freeMemory();
							m_pSchedules->addFront(new CBotTF2HealSched(m_pHeal));
						}
					}
				}
			}
		}
		else if (m_pHeal == pEntity
			&& strcmp(pEntity->GetClassName(), "entity_revive_marker") != 0)
		{
			m_pHeal = nullptr;
			removeCondition(CONDITION_SEE_HEAL);
		}
	}
	// else if ( m_iClass == TF_CLASS_SPY ) // Fix
	//{
	//  Look for nearest sentry to sap!!!
	if (bValid && bVisible)
	{
		float fEntDist = distanceFrom(pEntity);

		if (CTeamFortress2Mod::isSentry(pEntity, CTeamFortress2Mod::getEnemyTeam(getTeam())))
		{
			if ((m_iClass != TF_CLASS_ENGINEER) || !CClassInterface::isObjectCarried(pEntity))
			{
				edict_t *pNearest = m_pNearestEnemySentry.get();
				if (!pNearest
				    || ((pEntity != pNearest)
				        && (fEntDist < distanceFrom(pNearest))))
				{
					m_pNearestEnemySentry = pEntity;
				}
			}
		}
		else if (CTeamFortress2Mod::isTeleporter(pEntity, CTeamFortress2Mod::getEnemyTeam(getTeam())))
		{
			edict_t *pNearest = m_pNearestEnemyTeleporter.get();
			if (!pNearest
			    || ((pEntity != pNearest)
			        && (fEntDist < distanceFrom(pNearest))))
			{
				m_pNearestEnemyTeleporter = pEntity;
			}
		}
		else if (CTeamFortress2Mod::isDispenser(pEntity, CTeamFortress2Mod::getEnemyTeam(getTeam())))
		{
			edict_t *pNearest = m_pNearestEnemyDisp.get();
			if (!pNearest
			    || ((pEntity != pNearest) && (fEntDist < distanceFrom(pNearest))))
			{
				m_pNearestEnemyDisp = pEntity;
			}
		}
		else if (CTeamFortress2Mod::isHurtfulPipeGrenade(pEntity, m_pEdict, false))
		{
			edict_t *pNearest = m_pNearestPipeGren.get();
			if (!pNearest
			    || ((pEntity != pNearest) && (fEntDist < distanceFrom(pNearest))))
			{
			m_pNearestPipeGren  = pEntity;
			m_NearestEnemyGrenade = pEntity;
		}
		}
	}
	else if (pEntity == m_pNearestEnemySentry.get())
	{
		m_pNearestEnemySentry = nullptr;
	}
	else if (pEntity == m_pNearestEnemyTeleporter.get())
	{
		m_pNearestEnemyTeleporter = nullptr;
	}
	else if (pEntity == m_pNearestEnemyDisp.get())
	{
		m_pNearestEnemyDisp = nullptr;
	}
	else if (pEntity == m_pNearestPipeGren.get())
	{
		m_pNearestPipeGren = nullptr;
	}

	//}
	// Check for nearest Dispenser for health/ammo & flag
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW)) // EF_NODRAW == invisible
	{
		if ((m_pFlag.get() != pEntity) && CTeamFortress2Mod::isFlag(pEntity, getTeam()))
			m_pFlag = pEntity;
		else if ((m_pNearestAllySentry.get() != pEntity) && CTeamFortress2Mod::isSentry(pEntity, getTeam()))
		{
			edict_t *pNearest = m_pNearestAllySentry.get();
			if (!pNearest || (distanceFrom(pEntity) < distanceFrom(pNearest)))
				m_pNearestAllySentry = pEntity;
		}
		else if ((m_pNearestDisp.get() != pEntity) && CTeamFortress2Mod::isDispenser(pEntity, getTeam()))
		{
			edict_t *pNearest = m_pNearestDisp.get();
			if (!pNearest || (distanceFrom(pEntity) < distanceFrom(pNearest)))
				m_pNearestDisp = pEntity;
		}
		else if ((m_pNearestTeleEntrance.get() != pEntity) && CTeamFortress2Mod::isTeleporterEntrance(pEntity, getTeam()))
		{
			edict_t *pNearest = m_pNearestTeleEntrance.get();
			if (!pNearest || (distanceFrom(pEntity) < distanceFrom(pNearest)))
				m_pNearestTeleEntrance = pEntity;
		}
		else if ((m_pAmmo.get() != pEntity) && CTeamFortress2Mod::isAmmo(pEntity))
		{
			static float fDistance;
			fDistance = distanceFrom(pEntity);
			if (fDistance <= 512)
			{
				edict_t *pAmmo = m_pAmmo.get();
				if (!pAmmo || (fDistance < distanceFrom(pAmmo)))
					m_pAmmo = pEntity;
			}
		}
		else if ((m_pHealthkit.get() != pEntity) && CTeamFortress2Mod::isHealthKit(pEntity))
		{
			static float fDistance;
			fDistance = distanceFrom(pEntity);
			if (fDistance <= 200)
			{
				edict_t *pHealth = m_pHealthkit.get();
				if (!pHealth || (fDistance < distanceFrom(pHealth)))
					m_pHealthkit = pEntity;
			}
		}
	}
	else
	{
		if (pEntity == m_pFlag.get())
			m_pFlag = nullptr;
		else if (pEntity == m_pNearestDisp.get())
			m_pNearestDisp = nullptr;
		else if (pEntity == m_pAmmo.get())
			m_pAmmo = nullptr;
		else if (pEntity == m_pHealthkit.get())
			m_pHealthkit = nullptr;
		else if (pEntity == m_pHeal.get())
			m_pHeal = nullptr;
		else if (pEntity == m_pNearestPipeGren.get())
			m_pNearestPipeGren = nullptr;
	}

	return bValid;
}

void CBotFortress::medicCalled(edict_t *pPlayer)
{
	bool bGoto = true;

	if (pPlayer == m_pEdict)
		return; // can't heal self!
	if (m_iClass != TF_CLASS_MEDIC)
		return;                       // nothing to do
	if (distanceFrom(pPlayer) > 1024) // a bit far away
		return;                       // ignore
	if ((CBotGlobals::getTeam(pPlayer) == getTeam())
	    || (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY)
	           && thinkSpyIsEnemy(pPlayer, CTeamFortress2Mod::getSpyDisguise(pPlayer)))
	{

		m_pLastCalledMedic                      = pPlayer;
		m_fLastCalledMedicTime                  = engine->Time();
		m_fCallMedicTime[ENTINDEX(pPlayer) - 1] = m_fLastCalledMedicTime;

		if (m_pHeal == pPlayer)
			return; // already healing

		if (m_pHeal && CBotGlobals::isPlayer(m_pHeal))
		{
			if (CClassInterface::getPlayerHealth(pPlayer) >= CClassInterface::getPlayerHealth(m_pHeal))
				bGoto = false;
		}

		if (bGoto)
			m_pHeal = pPlayer;

		m_pLastHeal = m_pHeal;
	}
}

void CBotFortress::waitBackstab()
{
	m_fBackstabTime = engine->Time() + randomFloat(5.0f, 10.0f);
	m_pLastEnemy    = nullptr;
}

bool CBotFortress::isAlive()
{
	return !m_pPlayerInfo->IsDead() && !m_pPlayerInfo->IsObserver();
}

// hurt enemy player
void CBotFortress::seeFriendlyHurtEnemy(edict_t *pTeammate, edict_t *pEnemy, CWeapon *pWeapon)
{
	if (CBotGlobals::isPlayer(pEnemy) && (CClassInterface::getTF2Class(pEnemy) == TF_CLASS_SPY))
		m_fSpyAttackedList[ENTINDEX(pEnemy) - 1] = engine->Time();
}

void CBotFortress::shot(edict_t *pEnemy)
{
	seeFriendlyHurtEnemy(m_pEdict, pEnemy, nullptr);
}

void CBotFortress::killed(edict_t *pVictim, char *weapon)
{
	CBot::killed(pVictim, weapon);

	return;
}

void CBotFortress::died(edict_t *pKiller, const char *pszWeapon)
{
	CBot::died(pKiller, pszWeapon);

	if (CBotGlobals::isPlayer(pKiller) && (CClassInterface::getTF2Class(pKiller) == TF_CLASS_SPY))
		foundSpy(pKiller, (TF_Class)0);

	droppedFlag();

	if (randomInt(0, 1))
		m_pButtons->attack();
	else
		m_pButtons->letGo(IN_ATTACK);

	m_bCheckClass = true;
}

void CBotTF2::buildingDestroyed(int iType, edict_t *pAttacker, edict_t *pEdict)
{
	eEngiBuild type = (eEngiBuild)iType;

	switch (type)
	{
	case ENGI_DISP:
		m_pDispenser            = nullptr;
		m_bDispenserVectorValid = false;
		m_iDispenserArea        = 0;

		break;
	case ENGI_SENTRY:
		m_pSentryGun            = nullptr;
		m_bSentryGunVectorValid = false;
		m_iSentryArea           = 0;

		break;
	case ENGI_ENTRANCE:
		m_pTeleEntrance        = nullptr;
		m_bEntranceVectorValid = false;
		m_iTeleEntranceArea    = 0;

		break;
	case ENGI_EXIT:
		m_pTeleExit                = nullptr;
		m_bTeleportExitVectorValid = false;
		m_iTeleExitArea            = 0;
		break;
	}

	m_pSchedules->freeMemory();

	if (pEdict && CBotGlobals::entityIsValid(pEdict) && pAttacker && CBotGlobals::entityIsValid(pAttacker))
	{
		Vector vSentry   = CBotGlobals::entityOrigin(pEdict);
		Vector vAttacker = CBotGlobals::entityOrigin(pAttacker);

		m_pNavigator->belief(vSentry, vAttacker, bot_beliefmulti.GetFloat(), (vAttacker - vSentry).Length(),
		                     BELIEF_DANGER);
	}
}

void CBotFortress::wantToDisguise(bool bSet)
{
	if (rcbot_tf2_debug_spies_cloakdisguise.GetBool())
		if (bSet)
			m_fSpyDisguiseTime = 0.0f;
		else
			m_fSpyDisguiseTime = engine->Time() + 2.0f;
	else
		m_fSpyDisguiseTime = engine->Time() + 10.0f;
}

void CBotFortress::detectedAsSpy(edict_t *pDetector, bool bDisguiseComprimised)
{
	if (bDisguiseComprimised)
	{
		float fTime  = engine->Time() - m_fDisguiseTime;
		float fTotal = 0;

		if ((m_fDisguiseTime < 1) || (fTime < 3.0f))
			return;

		if (m_fClassDisguiseTime[m_iDisguiseClass] == 0)
			m_fClassDisguiseTime[m_iDisguiseClass] = fTime;
		else
			m_fClassDisguiseTime[m_iDisguiseClass] = (m_fClassDisguiseTime[m_iDisguiseClass] * 0.5f) + (fTime * 0.5f);

		for (unsigned short int i = 0; i < 10; i++)
			fTotal += m_fClassDisguiseTime[i];

		for (unsigned short int i = 0; i < 10; i++)
			if (m_fClassDisguiseTime[i] > 0)
				m_fClassDisguiseFitness[i] = (m_fClassDisguiseTime[i] / fTotal);

		m_fDisguiseTime = 0.0f;
	}
	else // go for cover
	{
		m_pAvoidEntity = pDetector;

		if (!m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT))
		{
			m_pSchedules->freeMemory();
			m_pSchedules->addFront(new CGotoHideSpotSched(this, m_pAvoidEntity));
		}
	}

	// Cloak and escape if revealed
	if (isTF() && m_iClass == TF_CLASS_SPY && !((CBotTF2 *)this)->isCloaked()
	    && !m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT))
	{
		((CBotTF2 *)this)->spyCloak();
		m_pSchedules->freeMemory();
		m_pSchedules->addFront(new CGotoHideSpotSched(this, pDetector));
	}
}

void CBotFortress::spawnInit()
{
	CBot::spawnInit();

	m_fLastSentryEnemyTime = 0.0f;

	m_fHealFactor          = 0.0f;

	m_pHealthkit           = MyEHandle(nullptr);
	m_pFlag                = MyEHandle(nullptr);
	m_pNearestDisp         = MyEHandle(nullptr);
	m_pAmmo                = MyEHandle(nullptr);
	m_pHeal                = MyEHandle(nullptr);
	m_pNearestPipeGren     = MyEHandle(nullptr);

	m_pMessAroundInviter   = MyEHandle(nullptr);

	// m_bWantToZoom = false;

	memset(m_fCallMedicTime, 0, sizeof(float) * MAX_PLAYERS);
	m_fWaitTurnSentry = 0.0f;

	m_pLastSeeMedic.reset();

	memset(m_fSpyAttackedList, 0, sizeof(float) * MAX_PLAYERS);
	memset(m_fSpyLastUncloakedList, 0, sizeof(float) * MAX_PLAYERS);

	memset(m_iReflectCount, 0, sizeof(m_iReflectCount));
	memset(m_fNextPyroShotTime, 0, sizeof(m_fNextPyroShotTime));
	memset(m_iLastKnownEnemyClass, 0, sizeof(m_iLastKnownEnemyClass));
	m_iLastReflectedRocket  = 0;
	m_iLastReflectedGrenade = 0;

	m_fTaunting                = 0.0f; // bots not moving FIX

	m_fMedicUpdatePosTime      = 0.0f;
	m_bShouldCrouchCover       = false;
	m_vLastMedicPatientOrigin  = Vector(0, 0, 0);

	m_pLastHeal                = nullptr;

	m_fDisguiseTime            = 0.0f;

	m_pNearestEnemyTeleporter  = nullptr;
	m_pNearestTeleEntrance     = nullptr;
	m_fBackstabTime            = 0.0f;
	m_fPickupTime              = 0.0f;
	m_fDefendTime              = 0.0f;
	m_fLookAfterSentryTime     = 0.0f;

	m_fSnipeAttackTime         = 0.0f;
	m_fSpyCloakTime            = 0.0f; // engine->Time();// + randomFloat(5.0f,10.0f);
	m_fSpyUncloakTime          = 0.0f;

	m_fLastSaySpy              = 0.0f;
	m_fSpyDisguiseTime         = 0.0f;
	m_pHeal                    = nullptr;
	m_pNearestDisp             = nullptr;
	m_pNearestEnemySentry      = nullptr;
	m_pNearestAllySentry       = nullptr;
	m_bHasFlag                 = false;
	// m_pPrevSpy = nullptr;
	// m_fSeeSpyTime = 0.0f;

	m_bSentryGunVectorValid    = false;
	m_bDispenserVectorValid    = false;
	m_bTeleportExitVectorValid = false;

	m_pSentryGun               = nullptr;
	m_pDispenser               = nullptr;
	m_pTeleEntrance            = nullptr;
	m_pTeleExit                = nullptr;
	m_pAttackingEnemy          = nullptr;
	m_pNearestEnemyDisp        = nullptr;
	m_pPrevSpy                 = nullptr;
	m_pLastEnemySentry         = nullptr;
}

bool CBotFortress::isBuilding(edict_t *pBuilding)
{
	return (pBuilding == m_pSentryGun.get()) || (pBuilding == m_pDispenser.get());
}

// return 0 : fail
// return 1 : built ok
// return 2 : next state
// return 3 : another try -- restart
int CBotFortress::engiBuildObject(int *iState, eEngiBuild iObject, float *fTime, int *iTries)
{
	// can't build while standing on my building!
	if (isBuilding(CClassInterface::getGroundEntity(m_pEdict)))
		return 0;

	if (m_fWaitTurnSentry > engine->Time())
		return 2;

	switch (*iState)
	{
	case 0:
	{
		// initialise
		if (hasEngineerBuilt(iObject))
			engineerBuild(iObject, ENGI_DESTROY);

		*iState = 1;
	}
	break;
	case 1:
	{
		CTraceFilterWorldAndPropsOnly filter;
		QAngle eyes = CBotGlobals::playerAngles(m_pEdict);
		QAngle turn;
		Vector forward;
		Vector building;
		Vector vchosen;
		Vector v_right, v_up, v_left;
		Vector v_src       = getEyePosition();
		// find best place to turn it to
		trace_t *tr        = CBotGlobals::getTraceResult();
		int iNextState     = 2;

		//		CBotWeapon *pWeapon = getCurrentWeapon();

		float bestfraction = 0.0f;

		m_fWaitTurnSentry  = 0.0f;
		// unselect current weapon
		selectWeapon(0);

		engineerBuild(iObject, ENGI_BUILD);
		/*
		if (pWeapon->getID() != TF2_WEAPON_BUILDER)
		{
		    if (*iTries > 9)
		        return 0; // fail

		    *iTries = *iTries + 1;

		    return 1; // continue;
		}*/

		*iTries    = 0;
		iNextState = 8;
		eyes.x     = 0; // nullify pitch / we want yaw only
		AngleVectors(eyes, &forward, &v_right, &v_up);
		building = v_src + (forward * 100);
		//////////////////////////////////////////

		// forward
		CBotGlobals::traceLine(building, building + forward * 4096.0, MASK_SOLID_BRUSHONLY, &filter);

		iNextState   = 8;
		bestfraction = tr->fraction;

		////////////////////////////////////////

		v_left       = -v_right;

		// left
		CBotGlobals::traceLine(building, building - v_right * 4096.0, MASK_SOLID_BRUSHONLY, &filter);

		if (tr->fraction > bestfraction)
		{
			iNextState   = 6;
			bestfraction = tr->fraction;
			vchosen      = building + v_right * 4096.0;
		}
		////////////////////////////////////////
		// back
		CBotGlobals::traceLine(building, building - forward * 4096.0, MASK_SOLID_BRUSHONLY, &filter);

		if (tr->fraction > bestfraction)
		{
			iNextState   = 4;
			bestfraction = tr->fraction;
			vchosen      = building - forward * 4096.0;
		}
		///////////////////////////////////
		// right
		CBotGlobals::traceLine(building, building + v_right * 4096.0, MASK_SOLID_BRUSHONLY, &filter);

		if (tr->fraction > bestfraction)
		{
			iNextState   = 2;
			bestfraction = tr->fraction;
			vchosen      = building + v_right * 4096.0;
		}
		////////////////////////////////////
		*iState = iNextState;

#ifndef __linux__
		if (CClients::clientsDebugging(BOT_DEBUG_THINK) && !engine->IsDedicatedServer())
		{
			debugoverlay->AddTriangleOverlay(v_src - v_left * 32.0f, v_src + v_left * 32.0f, v_src + (building - v_src),
			                                 255, 50, 50, 255, false, 60.0f);
			debugoverlay->AddLineOverlay(building, vchosen, 255, 50, 50, false, 60.0f);
			debugoverlay->AddTextOverlayRGB(building + Vector(0, 0, 25), 0, 60.0f, 255, 255, 255, 255,
			                                "Chosen State: %d", iNextState);
		}
#endif
	}
	case 2:
	{
		// let go
		m_pButtons->letGo(IN_ATTACK2);
		*iState = *iState + 1;
	}
	break;
	case 3:
	{
		tapButton(IN_ATTACK2);
		*iState           = *iState + 1;
		m_fWaitTurnSentry = engine->Time() + 0.33f;
		*fTime            = *fTime + 0.33f;
	}
	break;
	case 4:
	{
		// let go
		m_pButtons->letGo(IN_ATTACK2);
		*iState = *iState + 1;
	}
	break;
	case 5:
	{
		tapButton(IN_ATTACK2);
		*iState           = *iState + 1;
		m_fWaitTurnSentry = engine->Time() + 0.33f;
		*fTime            = *fTime + 0.33f;
	}
	break;
	case 6:

	{
		// let go
		m_pButtons->letGo(IN_ATTACK2);
		*iState = *iState + 1;
	}
	break;
	case 7:
	{
		tapButton(IN_ATTACK2);
		*iState           = *iState + 1;
		m_fWaitTurnSentry = engine->Time() + 0.33f;
		*fTime            = *fTime + 0.33f;
	}
	break;
	case 8:
	{
		// let go (wait)
		m_pButtons->letGo(IN_ATTACK2);
		*iState = *iState + 1;
	}
	break;
	case 9:
	{
		tapButton(IN_ATTACK);

		*fTime  = engine->Time() + randomFloat(0.5f, 1.0f);
		*iState = *iState + 1;
	}
	break;
	case 10:
	{
		// Check if sentry built OK
		// Wait for Built object message

		m_pButtons->tap(IN_ATTACK);
		duck(true); // crouch too

		if (*fTime < engine->Time())
		{
			// hasbuiltobject
			if (hasEngineerBuilt(iObject))
			{
				*iState = *iState + 1;
				// OK, set up whacking time!
				*fTime  = engine->Time() + randomFloat(5.0f, 10.0f);

				if (iObject == ENGI_SENTRY)
				{
					m_bSentryGunVectorValid = false;
					m_fLastSentryEnemyTime  = 0.0f;
				}
				else
				{
					if (iObject == ENGI_DISP)
						m_bDispenserVectorValid = false;
					else if (iObject == ENGI_EXIT)
	m_bTeleportExitVectorValid = false;
	m_iBestObscureTeleExit     = -1;

					removeCondition(CONDITION_COVERT);
					return 1;
				}
			}
			else if (*iTries > 3)
			{
				if (iObject == ENGI_SENTRY)
					m_bSentryGunVectorValid = false;
				else if (iObject == ENGI_DISP)
					m_bDispenserVectorValid = false;
				else if (iObject == ENGI_EXIT)
					m_bTeleportExitVectorValid = false;

				return 0;
			}
			else
			{
				//*fTime = engine->Time() + randomFloat(0.5,1.0);
				*iTries = *iTries + 1;
				*iState = 1;

				return 3;
			}
		}
	}
	break;
	case 11:
	{
		// whack it for a while
		if (*fTime < engine->Time())
		{
			removeCondition(CONDITION_COVERT);
			return 1;
		}
		else
		{
			tapButton(IN_ATTACK);
			duck(true); // crouch too
		}

		// someone blew my sentry before I built it!
		if (!hasEngineerBuilt(iObject))
			return 1;
	}
	break;
	}

	return 2;
}

void CBotFortress::setClass(TF_Class _class)
{
	m_iClass = _class;
}

bool CBotFortress::thinkSpyIsEnemy(edict_t *pEdict, TF_Class iDisguise)
{
	return ((m_fSeeSpyTime > engine->Time()) && // if bot is in spy check mode
	        (m_pPrevSpy == pEdict) &&           // and its the last spy we saw
	        // and its the same disguise or we last saw the spy just a couple of seconds ago
	        ((m_iPrevSpyDisguise == iDisguise) || ((engine->Time() - m_fLastSeeSpyTime) < 3.0f)));
}

bool CBotTF2::thinkSpyIsEnemy(edict_t *pEdict, TF_Class iDisguise)
{
	return CBotFortress::thinkSpyIsEnemy(pEdict, iDisguise)
	    || (m_pCloakedSpy && (m_pCloakedSpy == pEdict)
	        && !CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pCloakedSpy)); // maybe i put him on fire
}

bool CBotFortress::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
	if (pEdict == m_pEdict)
		return false;

	if (!ENTINDEX(pEdict) || (ENTINDEX(pEdict) > CBotGlobals::maxClients()))
		return false;

	if (CBotGlobals::getTeam(pEdict) == getTeam())
		return false;

	return true;
}

bool CBotFortress::needAmmo()
{
	return false;
}

bool CBotFortress::needHealth()
{
	// don't need health if I'm being ubered or healed
	return !m_bIsBeingHealed && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
	    && ((getHealthPercent() < 0.7) || CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict));
}

bool CBotTF2::needAmmo()
{
	if (getClass() == TF_CLASS_ENGINEER)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));

		if (pWeapon)
		{
			int iMetal = pWeapon->getAmmo(this);

			if ((m_pSentryGun.get() == nullptr) || (CClassInterface::getTF2UpgradeLevel(m_pSentryGun) < 3))
				return (iMetal < 200); // need 200 to upgrade sentry
			else
				return iMetal < 125; // need 125 for other stuff (e.g. teleporters)
		}
	}
	else if (getClass() == TF_CLASS_SOLDIER)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));

		if (pWeapon)
			return (pWeapon->getAmmo(this) < 1);
	}
	else if (getClass() == TF_CLASS_DEMOMAN)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));
		CBotWeapon *pSticky = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
		if (pWeapon)
			return (pWeapon->getAmmo(this) < 1 && (!pSticky || pSticky->getAmmo(this) < 1));
	}
	else if (getClass() == TF_CLASS_HWGUY)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MINIGUN));

		if (pWeapon)
			return (pWeapon->getAmmo(this) < 1);
	}
	else if (getClass() == TF_CLASS_PYRO)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));

		if (pWeapon)
			return (pWeapon->getAmmo(this) < 1);
	}
	else if (getClass() == TF_CLASS_SCOUT)
	{
		CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_SCATTERGUN));

		if (pWeapon)
			return (pWeapon->getAmmo(this) < 1);
	}

	return false;
}

void CBotFortress::currentlyDead()
{
	CBot::currentlyDead();

	m_bRevived     = true;
	m_fUpdateClass = engine->Time() + 0.1f;
}

void CBotFortress::modThink()
{
	// get class
	m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);
	m_iTeam  = getTeam();
	// updateClass();

	if (needHealth())
		updateCondition(CONDITION_NEED_HEALTH);
	else
		removeCondition(CONDITION_NEED_HEALTH);

	if (needAmmo())
		updateCondition(CONDITION_NEED_AMMO);
	else
		removeCondition(CONDITION_NEED_AMMO);

	// if ( !hasSomeConditions(CONDITION_PUSH) )
	///{
	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) || CTeamFortress2Mod::TF2_IsPlayerCritBoosted(m_pEdict))
		updateCondition(CONDITION_PUSH);
	//}

	if (m_fCallMedic < engine->Time())
	{
		if (getHealthPercent() < 0.5)
		{
			m_fCallMedic = engine->Time() + randomFloat(10.0f, 30.0f);

			callMedic();
		}
	}

	if ((m_fUseTeleporterTime < engine->Time()) && !hasFlag() && m_pNearestTeleEntrance)
	{
		if (isTeleporterUseful(m_pNearestTeleEntrance))
		{
			if (!m_pSchedules->isCurrentSchedule(SCHED_USE_TELE))
			{
				m_pSchedules->freeMemory();
				// m_pSchedules->removeSchedule(SCHED_USE_TELE);
				m_pSchedules->addFront(new CBotUseTeleSched(m_pNearestTeleEntrance));

				m_fUseTeleporterTime = engine->Time() + randomFloat(25.0f, 35.0f);
				return;
			}
		}
	}

	// Check redundant tasks
	if (!hasSomeConditions(CONDITION_NEED_AMMO) && m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_AMMO))
		m_pSchedules->removeSchedule(SCHED_TF2_GET_AMMO);

	if (!hasSomeConditions(CONDITION_NEED_HEALTH) && m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_HEALTH))
		m_pSchedules->removeSchedule(SCHED_TF2_GET_HEALTH);

	checkHealingValid();

	if (m_bInitAlive)
	{
		Vector vOrigin  = getOrigin();
		CWaypoint *pWpt = CWaypoints::getWaypoint(
		    CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_TELE_ENTRANCE, vOrigin, 4096, m_iTeam));

		if (pWpt)
		{
			// Get the nearest waypoint outside spawn (flagged as a teleporter entrance)
			// useful for Engineers and medics who want to camp for players
			m_vTeleportEntrance =
			    pWpt->getOrigin(); // +
			                       // Vector(randomFloat(-pWpt->getRadius(),pWpt->getRadius()),randomFloat(-pWpt->getRadius(),pWpt->getRadius()),0);
			m_bEntranceVectorValid = true;
		}
	}

	if ((m_fLastSeeEnemy == 0.0f) || ((m_fLastSeeEnemy + 5.0f) < engine->Time()))
	{
		m_fLastSeeEnemy = 0.0f;

		if (randomInt(0, 1))
			m_pButtons->tap(IN_RELOAD);
	}

	if (!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
	    && !CTeamFortress2Mod::TF2_IsPlayerCritBoosted(m_pEdict)
	    && (m_pNearestPipeGren.get() != nullptr))
	{
		if (!m_pSchedules->hasSchedule(SCHED_GOOD_HIDE_SPOT) && (distanceFrom(m_pNearestPipeGren) < 400.0f))
		{
			CGotoHideSpotSched *pSchedule = new CGotoHideSpotSched(this, m_pNearestPipeGren.get(), true);

			m_pSchedules->addFront(pSchedule);
		}
	}
}

bool CBotFortress::isTeleporterUseful(edict_t *pTele)
{
	edict_t *pExit = CTeamFortress2Mod::getTeleporterExit(pTele);

	if (pExit)
	{
		if (!CTeamFortress2Mod::isTeleporterSapped(pTele) && !CTeamFortress2Mod::isTeleporterSapped(pExit)
		    && !CClassInterface::isObjectBeingBuilt(pExit) && !CClassInterface::isObjectBeingBuilt(pTele)
		    && !CClassInterface::isObjectCarried(pExit) && !CClassInterface::isObjectCarried(pTele))
		{
			float fEntranceDist = distanceFrom(pTele);
			Vector vExit        = CBotGlobals::entityOrigin(pExit);
			Vector vEntrance    = CBotGlobals::entityOrigin(pTele);

			int countplayers    = CBotGlobals::countTeamMatesNearOrigin(vEntrance, 80.0f, m_iTeam, m_pEdict);

			int iCurWpt         = (m_pNavigator->getCurrentWaypointID() == -1)
			                        ? CWaypointLocations::NearestWaypoint(getOrigin(), 200.0f, -1, true)
			                        : m_pNavigator->getCurrentWaypointID();
			int iTeleWpt        = CTeamFortress2Mod::getTeleporterWaypoint(pExit);

			int iGoalId         = m_pNavigator->getCurrentGoalID();

			float fGoalDistance = ((iCurWpt == -1) || (iGoalId == -1))
			                        ? ((m_vGoal - vEntrance).Length() + fEntranceDist)
			                        : CWaypointDistances::getDistance(iCurWpt, iGoalId);
			float fTeleDistance = ((iTeleWpt == -1) || (iGoalId == -1))
			                        ? ((m_vGoal - vExit).Length() + fEntranceDist)
			                        : CWaypointDistances::getDistance(iTeleWpt, iGoalId);

			// still need to run to entrance then from exit
			if ((fEntranceDist + fTeleDistance)
			    < fGoalDistance) // ((m_vGoal - CBotGlobals::entityOrigin(pExit)).Length()+distanceFrom(pTele)) <
			                     // fGoalDistance )
			{
				// now check wait time based on how many players are waiting for the teleporter
				float fMaxSpeed = CClassInterface::getMaxSpeed(m_pEdict);
				float fDuration = CClassInterface::getTF2TeleRechargeDuration(pTele);

				edict_t *pOwner = CTeamFortress2Mod::getBuildingOwner(ENGI_TELE, ENTINDEX(pTele));
				float fRunTime;
				float fWaitTime;
				float fTime             = engine->Time();

				float fTeleRechargeTime = CTeamFortress2Mod::getTeleportTime(pOwner);

				fWaitTime               = (fEntranceDist / fMaxSpeed);

				// Teleporter never been used before and is ready to teleport
				if (fTeleRechargeTime > 0)
				{
					fWaitTime = (fEntranceDist / fMaxSpeed) + MAX(0, (fTeleRechargeTime - fTime) + fDuration);

					if (countplayers > 0)
						fWaitTime += fDuration * countplayers;
				}

				// see if i can run it myself in this time
				fRunTime = fGoalDistance / fMaxSpeed;

				return (fWaitTime < fRunTime);
			}
		}
	}

	return false;
}

void CBotFortress::selectTeam()
{
	helpers->ClientCommand(m_pEdict, "jointeam auto");
}

void CBotFortress::selectClass()
{
	const char *cmd;
	TF_Class _class;

	if (m_iDesiredClass == 0)
		_class = (TF_Class)randomInt(1, 9);
	else
		_class = (TF_Class)m_iDesiredClass;

	// only request class change if it doesn't match what the game is expecting
	if (CClassInterface::getTF2DesiredClass(m_pEdict) == m_iDesiredClass)
		return;

	m_iClass = _class;
	if (_class == TF_CLASS_SCOUT)
		cmd = "joinclass scout";
	else if (_class == TF_CLASS_ENGINEER)
		cmd = "joinclass engineer";
	else if (_class == TF_CLASS_DEMOMAN)
		cmd = "joinclass demoman";
	else if (_class == TF_CLASS_SOLDIER)
		cmd = "joinclass soldier";
	else if (_class == TF_CLASS_HWGUY)
		cmd = "joinclass heavyweapons";
	else if (_class == TF_CLASS_MEDIC)
		cmd = "joinclass medic";
	else if (_class == TF_CLASS_SPY)
		cmd = "joinclass spy";
	else if (_class == TF_CLASS_PYRO)
		cmd = "joinclass pyro";
	else
		cmd = "joinclass sniper";
	helpers->ClientCommand(m_pEdict, cmd);

	m_fChangeClassTime = engine->Time() + randomFloat(bot_min_cc_time.GetFloat(), bot_max_cc_time.GetFloat());
}

bool CBotFortress::waitForFlag(Vector *vOrigin, float *fWait, bool bFindFlag)
{
	// job calls!
	if (someoneCalledMedic())
		return false;

	if (seeFlag(false) != nullptr)
	{
		edict_t *m_pFlag = seeFlag(false);

		if (CBotGlobals::entityIsValid(m_pFlag))
		{
			lookAtEdict(m_pFlag);
			setLookAtTask(LOOK_EDICT);
			*vOrigin = CBotGlobals::entityOrigin(m_pFlag);
			*fWait   = engine->Time() + 5.0f;
		}
		else
			seeFlag(true);
	}
	else
		setLookAtTask(LOOK_AROUND);

	if (distanceFrom(*vOrigin) > 48)
		setMoveTo(*vOrigin);
	else
	{
		if (!bFindFlag && ((getClass() == TF_CLASS_SPY) && isDisguised()))
		{
			if (!CTeamFortress2Mod::isFlagCarried(m_iTeam))
				primaryAttack();
		}

		stopMoving();
	}

	return true;

	// taunt();
}

void CBotFortress::foundSpy(edict_t *pEdict, TF_Class iDisguise)
{
	m_pPrevSpy        = pEdict;
	m_fSeeSpyTime     = engine->Time() + randomFloat(9.0f, 18.0f);
	m_vLastSeeSpy     = CBotGlobals::entityOrigin(pEdict);
	m_fLastSeeSpyTime = engine->Time();
	if (iDisguise && (m_iPrevSpyDisguise != iDisguise))
		m_iPrevSpyDisguise = iDisguise;

	// m_fFirstSeeSpy = engine->Time(); // to do, add delayed action
};

// got shot by someone
bool CBotTF2::hurt(edict_t *pAttacker, int iHealthNow, bool bDontHide)
{
	if (!pAttacker)
		return false;

	// Taking damage forces a re-evaluation of current task
	updateCondition(CONDITION_CHANGED);

	if ((m_iClass != TF_CLASS_MEDIC) || (!m_pHeal))
	{
		if (CBot::hurt(pAttacker, iHealthNow, true))
		{
			if (m_bIsBeingHealed || m_bCanBeUbered)
			{
				// don't hide if I am being healed or can be ubered

				if (m_bCanBeUbered && !hasFlag()) // hack
					m_nextVoicecmd = TF_VC_ACTIVATEUBER;
			}
			else if (!bDontHide)
			{
				if (wantToNest())
				{
					CBotSchedule *pSchedule = new CBotSchedule();

					pSchedule->setID(SCHED_GOOD_HIDE_SPOT);

					// run at flank while shooting
					CFindPathTask *pHideGoalPoint = new CFindPathTask();
					Vector vOrigin                = CBotGlobals::entityOrigin(pAttacker);

					pSchedule->addTask(new CFindGoodHideSpot(vOrigin));
					pSchedule->addTask(pHideGoalPoint);
					pSchedule->addTask(new CBotNest());

					// no interrupts, should be a quick waypoint path anyway
					pHideGoalPoint->setNoInterruptions();
					// get vector from good hide spot task
					pHideGoalPoint->getPassedVector();
					pHideGoalPoint->setInterruptFunction(new CBotTF2CoverInterrupt());

					m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
					m_pSchedules->addFront(pSchedule);
				}
				else
				{
					m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
					m_pSchedules->addFront(new CGotoHideSpotSched(this, m_vHurtOrigin, new CBotTF2CoverInterrupt()));
				}

				if (CBotGlobals::isPlayer(pAttacker) && (m_iClass == TF_CLASS_SPY)
				    && (iHealthNow < rcbot_spy_runaway_health.GetInt()))
				{
					// cloak and run
					if (!isCloaked())
					{
						spyCloak();
						// hide and find health
						// updateCondition(CONDITION_CHANGED);
						wantToShoot(false);
						m_fFrenzyTime = 0.0f;
						if (hasEnemy())
							setLastEnemy(m_pEnemy);
						m_pEnemy = nullptr; // reset enemy
					}
				}
			}

			return true;
		}
	}

	if (pAttacker)
	{
		if ((m_iClass == TF_CLASS_SPY) && !isCloaked()
		    && !CTeamFortress2Mod::isSentry(pAttacker, CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
		{

			// TO DO : make sure I'm not just caught in crossfire
			// search for other team members
			if (!m_StatsCanUse.stats.m_iTeamMatesVisible || !m_StatsCanUse.stats.m_iTeamMatesInRange)
				m_fFrenzyTime = engine->Time() + randomFloat(2.0f, 6.0f);

			if (isDisguised())
				detectedAsSpy(pAttacker, true);

			if (CBotGlobals::isPlayer(pAttacker) && (iHealthNow < rcbot_spy_runaway_health.GetInt())
			    && (CClassInterface::getTF2SpyCloakMeter(m_pEdict) > 0.3f))
			{
				// cloak and run
				spyCloak();
				// hide and find health
				m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
				m_pSchedules->addFront(new CGotoHideSpotSched(this, m_vHurtOrigin, new CBotTF2CoverInterrupt()));
				wantToShoot(false);
				m_fFrenzyTime = 0.0f;

				if (hasEnemy())
					setLastEnemy(m_pEnemy);

				m_pEnemy = nullptr; // reset enemy

				return true;
			}
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// TEAM FORTRESS 2

void CBotTF2::spawnInit()
{
	CBotFortress::spawnInit();

	m_iDesiredResistType  = 0;
	m_fUseBuffItemTime    = 0.0f;
	// m_bHatEquipped = false;
	m_iTrapCPIndex        = -1;
	m_pHealer             = nullptr;
	m_fCallMedic          = engine->Time() + 10.0f;
	m_fCarryTime          = 0.0f;
	m_fNextBowIgnite       = 0.0f;
	m_bBowIgnitePending    = false;
	m_iLastBowIgniteSniper = -1;
	m_fNextCrossbowHeal    = 0.0f;
	m_bCrossbowPending     = false;
	m_fLastEnemyNearBomb   = engine->Time();
	m_fLastEurekaTeleport  = 0.0f;
	m_iBestObscureTeleExit = -1;
	m_iGuardSlot           = ENTINDEX(m_pEdict) % 4;

	m_bIsCarryingTeleExit = false;
	m_bIsCarryingSentry   = false;
	;
	m_bIsCarryingDisp = false;
	;
	m_bIsCarryingTeleEnt = false;
	;
	m_bIsCarryingObj = false;
	;

	m_nextVoicecmd        = TF_VC_INVALID;
	m_fAttackPointTime    = 0.0f;
	m_fNextRevMiniGunTime = 0.0f;
	m_fRevMiniGunTime     = 0.0f;

	m_pCloakedSpy         = nullptr;

	m_fRemoveSapTime      = 0.0f;
	m_fExtinguishTime     = 0.0f;
	m_fBaitCallTime       = 0.0f;
	m_fVoiceBuildTime     = 0.0f;
	m_fStuckSpyTime       = 0.0f;

	// stickies destroyed now
	m_iTrapType           = TF_TRAP_TYPE_NONE;

	// m_fBlockPushTime = 0.0f;

	m_iTeam               = getTeam();
	// update current areas
	updateAttackDefendPoints();
	// CPoints::getAreas(m_iTeam,&m_iCurrentDefendArea,&m_iCurrentAttackArea);

	m_fDoubleJumpTime         = 0.0f;
	m_fFrenzyTime             = 0.0f;
	m_fUseTeleporterTime      = 0.0f;
	m_fSpySapTime             = 0.0f;

	m_pDefendPayloadBomb      = nullptr;
	m_pPushPayloadBomb        = nullptr;
	m_pRedPayloadBomb         = nullptr;
	m_pBluePayloadBomb        = nullptr;
	m_NearestEnemyRocket        = nullptr;
	m_SecondNearestEnemyRocket   = nullptr;
	m_NearestEnemyGrenade        = nullptr;
	m_pLastEnemySentry        = nullptr;

	m_iPrevWeaponSelectFailed = 0;

	m_fCheckNextCarrying      = 0.0;
}

// return true if we don't want to hang around on the point
bool CBotTF2::checkAttackPoint()
{
	if (CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE))
	{
		m_fAttackPointTime = engine->Time() + randomFloat(5.0f, 15.0f);
		return true;
	}

	return false;
}

void CBotTF2::setClass(TF_Class _class)
{
	m_iClass = _class;
}

void CBotTF2::highFivePlayer(edict_t *pPlayer, float fYaw)
{
	if (!m_pSchedules->isCurrentSchedule(SCHED_TAUNT))
		m_pSchedules->addFront(new CBotTauntSchedule(pPlayer, fYaw));
}

// bOverride will be true in messaround mode
void CBotTF2::taunt(bool bOverride)
{
	// haven't taunted for a while, no emeny, not ubered, OK! Taunt!
	if (bOverride
	    || (!m_bHasFlag && rcbot_taunt.GetBool() && !CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict) && !m_pEnemy
	        && (m_fTauntTime < engine->Time()) && (!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))))
	{
		helpers->ClientCommand(m_pEdict, "taunt");
		m_fTauntTime = engine->Time() + randomFloat(40.0, 100.0); // Don't taunt for another minute or two
		m_fTaunting  = engine->Time() + 5.0;
	}
}

void CBotTF2::healedPlayer(edict_t *pPlayer, float fAmount)
{
	if (m_iClass == TF_CLASS_ENGINEER) // my dispenser was used
		m_fDispenserHealAmount += fAmount;
}
// useful for quick check of mod entities (especially ones which I need to find quickly
// e.g. sentry gun -- if I dont see it quickly it might kill me
edict_t *CBotFortress::getVisibleSpecial()
{
	static edict_t *pPlayer;
	static edict_t *pReturn;

	// this is a special visible which will return something important
	// that should be visible quickly, e.g. an enemy sentry gun
	// or teleporter entrance for bots to make decisions quickly
	if ((signed int)m_iSpecialVisibleId >= gpGlobals->maxClients)
		m_iSpecialVisibleId = 0;

	pPlayer = INDEXENT(m_iSpecialVisibleId + 1);
	pReturn = nullptr;

	if (pPlayer && (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_ENGINEER))
	{
		edict_t *pSentry = CTeamFortress2Mod::getSentryGun(m_iSpecialVisibleId);

		// more interested in enemy sentries to sap and shoot!
		pReturn          = pSentry;

		if (CClassInterface::getTeam(pPlayer) == m_iTeam)
		{
			// more interested in teleporters on my team
			edict_t *pTele = CTeamFortress2Mod::getTeleEntrance(m_iSpecialVisibleId);

			if (pTele)
				pReturn = pTele;

			if (getClass() == TF_CLASS_ENGINEER)
			{
				// be a bit more random with engi's as they both might be important
				// to repair other team members sentries!
				if (!pTele || randomInt(0, 1))
					pReturn = pSentry;
			}
		}
	}

	m_iSpecialVisibleId++;

	return pReturn;
}
/*
lambda-
NEW COMMAND SYNTAX:
- "build 2 0" - Build sentry gun
- "build 0 0" - Build dispenser
- "build 1 0" - Build teleporter entrance
- "build 1 1" - Build teleporter exit
*/
void CBotTF2::engineerBuild(eEngiBuild iBuilding, eEngiCmd iEngiCmd)
{
	// char buffer[16];
	// char cmd[256];

	if (iEngiCmd == ENGI_BUILD)
	{
		// strcpy(buffer,"build");

		switch (iBuilding)
		{
		case ENGI_DISP:
			// m_pDispenser = nullptr;
			helpers->ClientCommand(m_pEdict, "build 0 0");
			break;
		case ENGI_SENTRY:
			// m_pSentryGun = nullptr;
			helpers->ClientCommand(m_pEdict, "build 2 0");
			break;
		case ENGI_ENTRANCE:
			// m_pTeleEntrance = nullptr;
			helpers->ClientCommand(m_pEdict, "build 1 0");
			break;
		case ENGI_EXIT:
			// m_pTeleExit = nullptr;
			helpers->ClientCommand(m_pEdict, "build 1 1");
			break;
		default:
			return;
			break;
		}
	}
	else
	{
		// strcpy(buffer,"destroy");

		switch (iBuilding)
		{
		case ENGI_DISP:
			m_pDispenser            = nullptr;
			m_bDispenserVectorValid = false;
			m_iDispenserArea        = 0;
			helpers->ClientCommand(m_pEdict, "destroy 0 0");
			break;
		case ENGI_SENTRY:
			m_pSentryGun            = nullptr;
			m_bSentryGunVectorValid = false;
			m_iSentryArea           = 0;
			helpers->ClientCommand(m_pEdict, "destroy 2 0");
			break;
		case ENGI_ENTRANCE:
			m_pTeleEntrance        = nullptr;
			m_bEntranceVectorValid = false;
			m_iTeleEntranceArea    = 0;
			helpers->ClientCommand(m_pEdict, "destroy 1 0");
			break;
		case ENGI_EXIT:
			m_pTeleExit                = nullptr;
			m_bTeleportExitVectorValid = false;
			m_iTeleExitArea            = 0;
			helpers->ClientCommand(m_pEdict, "destroy 1 1");
			break;
		default:
			return;
			break;
		}
	}

	// sprintf(cmd,"%s %d 0",buffer,iBuilding); // added extra value to end

	// helpers->ClientCommand(m_pEdict,cmd);
}

void CBotTF2::updateCarrying()
{
	if ((m_bIsCarryingObj = CClassInterface::isCarryingObj(m_pEdict)) == true)
	{
		edict_t *pCarriedObj = CClassInterface::getCarriedObj(m_pEdict);

		if (pCarriedObj)
		{
			m_bIsCarryingTeleExit = CTeamFortress2Mod::isTeleporterExit(pCarriedObj, m_iTeam, true);
			m_bIsCarryingSentry   = CTeamFortress2Mod::isSentry(pCarriedObj, m_iTeam, true);
			m_bIsCarryingDisp     = CTeamFortress2Mod::isDispenser(pCarriedObj, m_iTeam, true);
			m_bIsCarryingTeleEnt  = CTeamFortress2Mod::isTeleporterEntrance(pCarriedObj, m_iTeam, true);
		}
	}
	else
	{
		m_bIsCarryingTeleExit = false;
		m_bIsCarryingSentry   = false;
		m_bIsCarryingDisp     = false;
		m_bIsCarryingTeleEnt  = false;
	}
}

float CBotTF2::evaluateBuildSpot(CWaypoint *pWpt, int iBuildingType)
{
	if (!pWpt) return 0.0f;

	float fDom         = CTeamFortress2Mod::getTeamDominance(m_iTeam);

	// Dominance gate: only use obscure placement when not dominated
	if (fDom < -0.2f && iBuildingType != 0) // BUILD_TELE=0 always uses this path
		return 0.0f;

	float fTraversal   = (float)pWpt->peekTraversalCount();
	float fObscurity   = 1.0f - (fTraversal / (fTraversal + 3.0f));

	// Proximity to flag or capture point
	Vector vFlag = getOrigin();
	if (!CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE, &vFlag))
		CTeamFortress2Mod::getMVMCapturePoint(&vFlag);
	float fDist = (pWpt->getOrigin() - vFlag).Length();
	float fProximity = 1500.0f / (1500.0f + fDist);

	// Base weights vary by building type
	float fObsWeight = 0.7f, fProxWeight = 0.3f;
	if (iBuildingType == 1) // sentry: less obscure, more proximity
	{
		fObsWeight = 0.4f; fProxWeight = 0.6f;
	}
	else if (iBuildingType == 2) // dispenser: balanced, hidden but useful
	{
		fObsWeight = 0.6f; fProxWeight = 0.4f;
	}

	float fScore = fObscurity * fObsWeight + fProximity * fProxWeight;
	if (fDom > 0.0f)
		fScore = fObscurity * (fObsWeight - fDom * 0.3f) + fProximity * (fProxWeight + fDom * 0.3f);

	if (CTeamFortress2Mod::buildingNearby(m_iTeam, pWpt->getOrigin()))
		fScore *= 0.3f;

	return fScore;
}

float CBotTF2::evaluateTeleExitSpot(CWaypoint *pWpt)
{
	return evaluateBuildSpot(pWpt, 0);
}

void CBotTF2::checkBuildingsValid(bool bForce) // force check carrying
{
	if (m_pSentryGun)
	{
		if (!CBotGlobals::entityIsValid(m_pSentryGun) || !CBotGlobals::entityIsAlive(m_pSentryGun)
		    || !CTeamFortress2Mod::isSentry(m_pSentryGun, m_iTeam))
		{
			m_pSentryGun       = nullptr;
			m_prevSentryHealth = 0;
			m_iSentryArea      = 0;
		}
		else if (CClassInterface::getSentryEnemy(m_pSentryGun) != nullptr)
			m_fLastSentryEnemyTime = engine->Time();
	}

	if (m_pDispenser)
	{
		if (!CBotGlobals::entityIsValid(m_pDispenser) || !CBotGlobals::entityIsAlive(m_pDispenser)
		    || !CTeamFortress2Mod::isDispenser(m_pDispenser, m_iTeam))
		{
			m_pDispenser     = nullptr;
			m_prevDispHealth = 0;
			m_iDispenserArea = 0;
		}
	}

	if (m_pTeleEntrance)
	{
		if (!CBotGlobals::entityIsValid(m_pTeleEntrance) || !CBotGlobals::entityIsAlive(m_pTeleEntrance)
		    || !CTeamFortress2Mod::isTeleporterEntrance(m_pTeleEntrance, m_iTeam))
		{
			m_pTeleEntrance     = nullptr;
			m_prevTeleEntHealth = 0;
			m_iTeleEntranceArea = 0;
		}
	}

	if (m_pTeleExit)
	{
		if (!CBotGlobals::entityIsValid(m_pTeleExit) || !CBotGlobals::entityIsAlive(m_pTeleExit)
		    || !CTeamFortress2Mod::isTeleporterExit(m_pTeleExit, m_iTeam))
		{
			m_pTeleExit         = nullptr;
			m_prevTeleExtHealth = 0;
			m_iTeleExitArea     = 0;
		}
	}
}

// Find the EDICT_T of the building that the engineer just built...
edict_t *CBotTF2::findEngineerBuiltObject(eEngiBuild iBuilding, int index)
{
	int team       = getTeam();

	edict_t *pBest = INDEXENT(index);

	if (pBest)
	{
		if (iBuilding == ENGI_TELE)
		{
			if (CTeamFortress2Mod::isTeleporterEntrance(pBest, team))
				iBuilding = ENGI_ENTRANCE;
			else if (CTeamFortress2Mod::isTeleporterExit(pBest, team))
				iBuilding = ENGI_EXIT;
		}

		switch (iBuilding)
		{
		case ENGI_DISP:
			m_pDispenser = pBest;
			break;
		case ENGI_ENTRANCE:
			m_pTeleEntrance = pBest;
			// m_vTeleportEntrance = CBotGlobals::entityOrigin(pBest);
			// m_bEntranceVectorValid = true;
			break;
		case ENGI_EXIT:
			m_pTeleExit = pBest;
			break;
		case ENGI_SENTRY:
			m_pSentryGun = pBest;
			break;
		default:
			return nullptr;
		}
	}

	return pBest;
}

void CBotTF2::died(edict_t *pKiller, const char *pszWeapon)
{
	CBotFortress::died(pKiller, pszWeapon);

	int iWpt = CWaypointLocations::NearestWaypoint(getOrigin(), 256.0f, -1);
	if (iWpt >= 0)
	{
		CWaypoint *pWpt = CWaypoints::getWaypoint(iWpt);
		if (pWpt)
		{
			m_iLastDeathArea = pWpt->getArea();
			m_fLastDeathTime = engine->Time();
		}
	}

	if (pKiller)
	{
		if (CBotGlobals::entityIsValid(pKiller))
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller), getEyePosition(), bot_beliefmulti.GetFloat(),
			                     distanceFrom(pKiller), BELIEF_DANGER);

		if (!strncmp(pszWeapon, "obj_sentrygun", 13) || !strncmp(pszWeapon, "obj_minisentry", 14))
		{
			m_pLastEnemySentry = pKiller;
			addKnownSentry(pKiller);
		}
		}
	}
}

void CBotTF2::killed(edict_t *pVictim, char *weapon)
{
	CBotFortress::killed(pVictim, weapon);

	if ((m_pSentryGun.get() != nullptr) && (m_iClass == TF_CLASS_ENGINEER) && weapon && *weapon
	    && (strncmp(weapon, "obj_sentry", 10) == 0))
	{
		m_iSentryKills++;

		if (pVictim && CBotGlobals::entityIsValid(pVictim))
		{
			Vector vSentry = CBotGlobals::entityOrigin(m_pSentryGun);
			Vector vVictim = CBotGlobals::entityOrigin(pVictim);

			m_pNavigator->belief(vVictim, vSentry, bot_beliefmulti.GetFloat(), (vSentry - vVictim).Length(),
			                     BELIEF_SAFETY);
		}
	}
	else if (pVictim && CBotGlobals::entityIsValid(pVictim))
		m_pNavigator->belief(CBotGlobals::entityOrigin(pVictim), getEyePosition(), bot_beliefmulti.GetFloat(),
		                     distanceFrom(pVictim), BELIEF_SAFETY);

	if (CBotGlobals::isPlayer(pVictim) && (CClassInterface::getTF2Class(pVictim) == TF_CLASS_SPY))
	{
		if (m_pPrevSpy == pVictim)
		{
			m_pPrevSpy        = nullptr;
			m_fLastSeeSpyTime = 0.0f;
		}

		removeCondition(CONDITION_PARANOID);
	}

	taunt();
}

void CBotTF2::capturedFlag()
{
	taunt();
}

void CBotTF2::spyDisguise(int iTeam, int iClass)
{
	// char cmd[256];

	if (iTeam == 3)
		m_iImpulse = 230 + iClass;
	else if (iTeam == 2)
		m_iImpulse = 220 + iClass;

	m_fDisguiseTime  = engine->Time();
	m_iDisguiseClass = iClass;
	m_fFrenzyTime    = 0.0f; // reset frenzy time

	// moooo

	// sprintf(cmd,"disguise %d %d",iClass,iTeam);

	// helpers->ClientCommand(m_pEdict,cmd);
}
// Test
bool CBotTF2::isCloaked()
{
	return CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict);
}
// Test
bool CBotTF2::isDisguised()
{
	int _class, _team, _index, _health;

	if (CClassInterface::getTF2SpyDisguised(m_pEdict, &_class, &_team, &_index, &_health))

	{
		if (_class > 0)
			return true;
	}

	return false;
}

void CBotTF2::updateClass()
{
	if (m_fUpdateClass && (m_fUpdateClass < engine->Time()))
	{
		/*const char *model = m_pPlayerInfo->GetModelName();

		if ( strcmp(model,"soldier") )
		    m_iClass = TF_CLASS_SOLDIER;
		else if ( strcmp(model,"sniper") )
		    m_iClass = TF_CLASS_SNIPER;
		else if ( strcmp(model,"heavyweapons") )
		    m_iClass = TF_CLASS_HWGUY;
		else if ( strcmp(model,"medic") )
		    m_iClass = TF_CLASS_MEDIC;
		else if ( strcmp(model,"pyro") )
		    m_iClass = TF_CLASS_PYRO;
		else if ( strcmp(model,"spy") )
		    m_iClass = TF_CLASS_SPY;
		else if ( strcmp(model,"scout") )
		    m_iClass = TF_CLASS_SCOUT;
		else if ( strcmp(model,"engineer") )
		    m_iClass = TF_CLASS_ENGINEER;
		else if ( strcmp(model,"demoman") )
		    m_iClass = TF_CLASS_DEMOMAN;
		else
		    m_iClass = TF_CLASS_CIVILIAN;
		    */

		m_fUpdateClass = 0;
	}
}

TF_Class CBotTF2::getClass()
{
	return m_iClass;
}

void CBotTF2::setup()
{
	CBotFortress::setup();
}

void CBotTF2::seeFriendlyKill(edict_t *pTeamMate, edict_t *pDied, CWeapon *pWeapon)
{
	if (CBotGlobals::isPlayer(pDied) && CClassInterface::getTF2Class(pDied) == TF_CLASS_SPY)
	{
		if (m_pPrevSpy == pDied)
		{
			m_pPrevSpy        = nullptr;
			m_fLastSeeSpyTime = 0.0f;
		}

		removeCondition(CONDITION_PARANOID);
	}
}

void CBotTF2::seeFriendlyDie(edict_t *pDied, edict_t *pKiller, CWeapon *pWeapon)
{
	if (pKiller && !m_pEnemy && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		bool shouldReact = !isDisguised() && !isCloaked();
		// if ( pWeapon )
		//{
		//	DOD_Class pclass = (DOD_Class)CClassInterface::getPlayerClassDOD(pKiller);

		if (pWeapon && (pWeapon->getID() == TF2_WEAPON_SENTRYGUN))
		{
			if (shouldReact)
				addVoiceCommand(TF_VC_SENTRYAHEAD);
			updateCondition(CONDITION_COVERT);
			m_fCurrentDanger += 100.0f;
			m_pLastEnemySentry = CTeamFortress2Mod::getMySentryGun(pKiller);
			m_vLastDiedOrigin  = CBotGlobals::entityOrigin(pDied);

			addKnownSentry(m_pLastEnemySentry.get());
			if (CTeamFortress2Mod::getMySentryGun(pKiller))
				addKnownSentry(CTeamFortress2Mod::getMySentryGun(pKiller));

			if ((m_iClass == TF_CLASS_DEMOMAN) || (m_iClass == TF_CLASS_SPY))
			{
				// perhaps pipe it!
				updateCondition(CONDITION_CHANGED);
			}
		}
		else
		{
			if (shouldReact)
				addVoiceCommand(TF_VC_INCOMING);
			updateCondition(CONDITION_COVERT);
			m_fCurrentDanger += 50.0f;
		}

		//}

		// encourage bots to snoop out enemy or throw grenades
		m_fLastSeeEnemy              = engine->Time();
		m_pLastEnemy                 = pKiller;
		m_fLastUpdateLastSeeEnemy    = 0;
		m_vLastSeeEnemy              = CBotGlobals::entityOrigin(m_pLastEnemy);
		m_vLastSeeEnemyBlastWaypoint = m_vLastSeeEnemy;

		CWaypoint *pWpt              = CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(
            m_vLastSeeEnemy, getOrigin(), 4096.0, -1, true, true, false, false, 0, false));

		if (pWpt)
			m_vLastSeeEnemyBlastWaypoint = pWpt->getOrigin();

		updateCondition(CONDITION_CHANGED);
	}
}

void CBotTF2::addKnownSentry(edict_t *pSentry)
{
	if (!pSentry)
		return;

	// Prune dead entries from the list
	for (size_t i = 0; i < m_KnownSentries.size();)
	{
		edict_t *pExisting = m_KnownSentries[i].get();
		if (!pExisting || !CBotGlobals::entityIsValid(pExisting) || !CBotGlobals::entityIsAlive(pExisting))
			m_KnownSentries.erase(m_KnownSentries.begin() + i);
		else
			i++;
	}

	// Don't add duplicates
	for (size_t i = 0; i < m_KnownSentries.size(); i++)
	{
		if (m_KnownSentries[i].get() == pSentry)
			return;
	}

	// Cap the list at 16 entries to prevent memory bloat
	if (m_KnownSentries.size() >= 16)
		return;

	m_KnownSentries.push_back(MyEHandle(pSentry));

	// Share the sentry position team-wide
	CTeamFortress2Mod::addTeamKnownSentry(CBotGlobals::entityOrigin(pSentry));
}

void CBotTF2::addKnownEnemyTeleporter(edict_t *pTele)
{
	if (!pTele) return;
	for (size_t i = 0; i < m_KnownEnemyTeleporters.size();)
	{
		edict_t *pE = m_KnownEnemyTeleporters[i].get();
		if (!pE || !CBotGlobals::entityIsValid(pE) || !CBotGlobals::entityIsAlive(pE))
			m_KnownEnemyTeleporters.erase(m_KnownEnemyTeleporters.begin() + i);
		else i++;
	}
	for (size_t i = 0; i < m_KnownEnemyTeleporters.size(); i++)
		if (m_KnownEnemyTeleporters[i].get() == pTele) return;
	if (m_KnownEnemyTeleporters.size() >= 16) return;
	m_KnownEnemyTeleporters.push_back(MyEHandle(pTele));
	CTeamFortress2Mod::addTeamKnownTeleporter(CBotGlobals::entityOrigin(pTele));
}

void CBotTF2::addKnownEnemyDispenser(edict_t *pDisp)
{
	if (!pDisp) return;
	for (size_t i = 0; i < m_KnownEnemyDispensers.size();)
	{
		edict_t *pE = m_KnownEnemyDispensers[i].get();
		if (!pE || !CBotGlobals::entityIsValid(pE) || !CBotGlobals::entityIsAlive(pE))
			m_KnownEnemyDispensers.erase(m_KnownEnemyDispensers.begin() + i);
		else i++;
	}
	for (size_t i = 0; i < m_KnownEnemyDispensers.size(); i++)
		if (m_KnownEnemyDispensers[i].get() == pDisp) return;
	if (m_KnownEnemyDispensers.size() >= 16) return;
	m_KnownEnemyDispensers.push_back(MyEHandle(pDisp));
}

void CBotTF2::pruneKnownEnemyBuildings()
{
	for (size_t i = 0; i < m_KnownEnemyTeleporters.size();)
	{
		edict_t *pE = m_KnownEnemyTeleporters[i].get();
		if (!pE || !CBotGlobals::entityIsValid(pE) || !CBotGlobals::entityIsAlive(pE))
			m_KnownEnemyTeleporters.erase(m_KnownEnemyTeleporters.begin() + i);
		else i++;
	}
	for (size_t i = 0; i < m_KnownEnemyDispensers.size();)
	{
		edict_t *pE = m_KnownEnemyDispensers[i].get();
		if (!pE || !CBotGlobals::entityIsValid(pE) || !CBotGlobals::entityIsAlive(pE))
			m_KnownEnemyDispensers.erase(m_KnownEnemyDispensers.begin() + i);
		else i++;
	}
}

void CBotTF2::engiBuildSuccess(eEngiBuild iBuilding, int index)
{
	edict_t *pEntity = findEngineerBuiltObject(iBuilding, index);

	if (iBuilding == ENGI_SENTRY)
	{
		m_fSentryPlaceTime = engine->Time();
		m_iSentryKills     = 0;
	}
	else if (iBuilding == ENGI_DISP)
	{
		m_fDispenserPlaceTime  = engine->Time();
		m_fDispenserHealAmount = 0.0f;
	}
	else if (iBuilding == ENGI_TELE)
	{
		if (CTeamFortress2Mod::isTeleporterEntrance(pEntity, m_iTeam))
		{
			m_fTeleporterEntPlacedTime = engine->Time();

			// if ( m_pTeleExit.get() != nullptr ) // already has exit built
			m_iTeleportedPlayers       = 0;
		}
		else if (CTeamFortress2Mod::isTeleporterExit(pEntity, m_iTeam))
		{
			m_fTeleporterExtPlacedTime = engine->Time();

			if (m_pTeleEntrance.get() == nullptr) // doesn't have entrance built
				m_iTeleportedPlayers = 0;
		}
	}
}

bool CBotTF2::hasEngineerBuilt(eEngiBuild iBuilding)
{
	switch (iBuilding)
	{
	case ENGI_SENTRY:
		return m_pSentryGun != nullptr; // TODO
		break;
	case ENGI_DISP:
		return m_pDispenser != nullptr; // TODO
		break;
	case ENGI_ENTRANCE:
		return m_pTeleEntrance != nullptr; // TODO
		break;
	case ENGI_EXIT:
		return m_pTeleExit != nullptr; // TODO
		break;
	}

	return false;
}

// ENEMY Flag dropped
void CBotFortress::flagDropped(Vector vOrigin)
{
	m_vLastKnownFlagPoint = vOrigin;
	m_fLastKnownFlagTime  = engine->Time() + 60.0f;

	if (m_pSchedules->hasSchedule(SCHED_RETURN_TO_INTEL))
		m_pSchedules->removeSchedule(SCHED_RETURN_TO_INTEL);
}

void CBotFortress::teamFlagDropped(Vector vOrigin)
{
	m_vLastKnownTeamFlagPoint = vOrigin;

	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		m_fLastKnownTeamFlagTime = engine->Time() + 60.0f;
	else
		m_fLastKnownTeamFlagTime = engine->Time() + 60.0f;

	// FIX
	if (m_pSchedules->hasSchedule(SCHED_TF2_GET_FLAG))
		m_pSchedules->removeSchedule(SCHED_TF2_GET_FLAG);
}

void CBotFortress::callMedic()
{
	helpers->ClientCommand(m_pEdict, "saveme");
}

bool CBotTF2::canGotoWaypoint(Vector vPrevWaypoint, CWaypoint *pWaypoint, CWaypoint *pPrev)
{
	static edict_t *pSentry;

	if (CBot::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev))
	{
		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_OWNER_ONLY))
		{
			int area     = pWaypoint->getArea();
			int capindex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[area];

			if (area && (CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(capindex) != m_iTeam))
				return false;
		}

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_AREAONLY))
		{
			if (!CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(pWaypoint->getArea(),
			                                                                pWaypoint->getFlags()))
				return false;
		}

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_ROCKET_JUMP))
		{
			CBotWeapons *pWeapons = getWeapons();
			CBotWeapon *pWeapon;

			// only roccket jump if more than 50% health
			if (getHealthPercent() > 0.5)
			{
				// only soldiers or demomen can use these
				if (getClass() == TF_CLASS_SOLDIER)
				{
					pWeapon = pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));

					if (pWeapon)
						return (pWeapon->getAmmo(this) > 0);
				}
				else if ((getClass() == TF_CLASS_DEMOMAN) && rcbot_demo_jump.GetBool())
				{
					pWeapon = pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

					if (pWeapon)
					{
						edict_t *pWeaponEdict = pWeapon->getWeaponEntity();

						// if it isnt the scottish resistance pipe launcher
						if (!pWeaponEdict || (CClassInterface::TF2_getItemDefinitionIndex(pWeaponEdict) != 130))
							return (pWeapon->getClip1(this) > 0);
					}
				}
			}

			return false;
		}

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_DOUBLEJUMP))
			return (getClass() == TF_CLASS_SCOUT);

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_WAIT_GROUND))
			return pWaypoint->checkGround();

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_NO_FLAG))
			return (!hasFlag());

		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_FLAGONLY))
			return hasFlag();

		pSentry = nullptr;

		if (m_iClass == TF_CLASS_ENGINEER)
			pSentry = m_pSentryGun.get();
		else if (m_iClass == TF_CLASS_SPY)
			pSentry = m_pNearestEnemySentry.get();

		if (pSentry != nullptr)
		{
			Vector vWaypoint = pWaypoint->getOrigin();
			Vector vWptMin   = vWaypoint.Min(vPrevWaypoint) - Vector(32, 32, 32);
			Vector vWptMax   = vWaypoint.Max(vPrevWaypoint) + Vector(32, 32, 32);

			Vector vSentry   = CBotGlobals::entityOrigin(pSentry);
			Vector vMax      = vSentry + pSentry->GetCollideable()->OBBMaxs();
			Vector vMin      = vSentry + pSentry->GetCollideable()->OBBMins();

			if (vSentry.WithinAABox(vWptMin, vWptMax))
			{
				Vector vSentryComp = vSentry - vPrevWaypoint;

				float fSentryDist  = vSentryComp.Length();
				// float fWaypointDist = pWaypoint->distanceFrom(vPrevWaypoint);

				// if ( fSentryDist < fWaypointDist )
				//{
				Vector vComp       = pWaypoint->getOrigin() - vPrevWaypoint;

				vComp              = vPrevWaypoint + ((vComp / vComp.Length()) * fSentryDist);

				// Path goes through sentry -- can't walk through here
				if (vComp.WithinAABox(vMin, vMax))
					return false;
				//}
			}

			// check if waypoint goes through sentry (can't walk through)
		}

		if (CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE))
		{
			if (m_pRedPayloadBomb.get() != nullptr)
			{
				edict_t *pSentry = m_pRedPayloadBomb.get();
				// check path doesn't go through pay load bomb

				Vector vWaypoint = pWaypoint->getOrigin();
				Vector vWptMin   = vWaypoint.Min(vPrevWaypoint) - Vector(32, 32, 32);
				Vector vWptMax   = vWaypoint.Max(vPrevWaypoint) + Vector(32, 32, 32);

				Vector vSentry   = CBotGlobals::entityOrigin(pSentry);
				Vector vMax      = vSentry + Vector(32, 32, 32);
				Vector vMin      = vSentry - Vector(32, 32, 32);

				if (vSentry.WithinAABox(vWptMin, vWptMax))
				{
					Vector vSentryComp = vSentry - vPrevWaypoint;

					float fSentryDist  = vSentryComp.Length();
					// float fWaypointDist = pWaypoint->distanceFrom(vPrevWaypoint);

					// if ( fSentryDist < fWaypointDist )
					//{
					Vector vComp       = pWaypoint->getOrigin() - vPrevWaypoint;

					vComp              = vPrevWaypoint + ((vComp / vComp.Length()) * fSentryDist);

					// Path goes through sentry -- can't walk through here
					if (vComp.WithinAABox(vMin, vMax))
						return false;
					//}
				}
			}

			if (m_pBluePayloadBomb.get() != nullptr)
			{
				edict_t *pSentry = m_pBluePayloadBomb.get();
				Vector vWaypoint = pWaypoint->getOrigin();
				Vector vWptMin   = vWaypoint.Min(vPrevWaypoint) - Vector(32, 32, 32);
				Vector vWptMax   = vWaypoint.Max(vPrevWaypoint) + Vector(32, 32, 32);

				Vector vSentry   = CBotGlobals::entityOrigin(pSentry);
				Vector vMax      = vSentry + Vector(32, 32, 32);
				Vector vMin      = vSentry - Vector(32, 32, 32);

				if (vSentry.WithinAABox(vWptMin, vWptMax))
				{
					Vector vSentryComp = vSentry - vPrevWaypoint;

					float fSentryDist  = vSentryComp.Length();
					// float fWaypointDist = pWaypoint->distanceFrom(vPrevWaypoint);

					// if ( fSentryDist < fWaypointDist )
					//{
					Vector vComp       = pWaypoint->getOrigin() - vPrevWaypoint;

					vComp              = vPrevWaypoint + ((vComp / vComp.Length()) * fSentryDist);

					// Path goes through sentry -- can't walk through here
					if (vComp.WithinAABox(vMin, vMax))
						return false;
					//}
				}
			}
		}

		return true;
	}
	else if (pWaypoint->hasFlag(CWaypointTypes::W_FL_FALL))
	{
		return (getClass() == TF_CLASS_SCOUT);
	}

	return false;
}

void CBotTF2::callMedic()
{
	addVoiceCommand(TF_VC_MEDIC);
}

void CBotFortress::waitCloak()
{
	m_fSpyCloakTime = engine->Time() + randomFloat(2.0f, 6.0f);
}

bool CBotFortress::wantToCloak()
{
	// MvM: when on fire, go kamikaze instead of cloaking
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	    && CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict))
		return false;

	if (rcbot_tf2_debug_spies_cloakdisguise.GetBool())
	{
		if ((m_fFrenzyTime < engine->Time()) && (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)))
		{
			return ((!m_bStatsCanUse || (m_StatsCanUse.stats.m_iEnemiesVisible > 0))
			        && (CClassInterface::getTF2SpyCloakMeter(m_pEdict) > 90.0f)
			        && (m_fCurrentDanger > TF2_SPY_CLOAK_BELIEF));
		}
	}

	return false;
}

bool CBotFortress::wantToUnCloak()
{
	if (wantToShoot() && m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		// hopefully the enemy can't see me
		if (CBotGlobals::isAlivePlayer(m_pEnemy)
		    && (fabs(CBotGlobals::yawAngleFromEdict(m_pEnemy, getOrigin())) > bot_spyknifefov.GetFloat()))
			return true;
		else if (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			return (m_fCurrentDanger < 1.0f);
	}

	return (m_bStatsCanUse && (m_StatsCanUse.stats.m_iEnemiesVisible == 0));
}

void CBotTF2::spyUnCloak()
{
	if (CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict) && (m_fSpyCloakTime < engine->Time()))
	{
		secondaryAttack();

		m_fSpyCloakTime = engine->Time() + randomFloat(2.0f, 4.0f);
		// m_fSpyCloakTime = m_fSpyUncloakTime;
	}
}

void CBotTF2::spyCloak()
{
	if (!CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict) && (m_fSpyCloakTime < engine->Time()))
	{
		m_fSpyCloakTime = engine->Time() + randomFloat(2.0f, 4.0f);
		// m_fSpyUncloakTime = m_fSpyCloakTime;

		secondaryAttack();
	}
}

bool CBotTF2::tryExtinguishTeammates()
{
	if (m_iClass != TF_CLASS_PYRO)
		return false;

	// Cooldown to prevent spam
	if (m_fExtinguishTime > engine->Time())
		return false;

	// Check for manmelter (secondary slot, item 595) -- preferred: no ammo cost, stores crit
	CBotWeapon *pManmelter = nullptr;
	bool bHasFlame          = false;
	int iFItem              = 0;
	int iNeedAmmo           = 0;
	CBotWeapon *pFlame      = nullptr;

	CBotWeapon *pSec = m_pWeapons->getCurrentWeaponInSlot(TF2_SLOT_SCNDR);
	if (pSec)
	{
		edict_t *pSecEnt = pSec->getWeaponEntity();
		if (pSecEnt && CClassInterface::TF2_getItemDefinitionIndex(pSecEnt) == 595)
			pManmelter = pSec;
	}

	pFlame = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
	if (pFlame && pFlame->hasWeapon())
	{
		edict_t *pFEnt = pFlame->getWeaponEntity();
		iFItem          = pFEnt ? CClassInterface::TF2_getItemDefinitionIndex(pFEnt) : 0;
		if (iFItem != 594) // phlog can't airblast
		{
			iNeedAmmo = (iFItem == 40) ? 50 : (iFItem == 215) ? 25
			          : (iFItem == 1178 || iFItem == 1099) ? 5 : 20;
			if (pFlame->getAmmo(this) >= iNeedAmmo)
				bHasFlame = true;
		}
	}

	if (!pManmelter && !bHasFlame)
		return false;

	// Find the closest burning teammate
	edict_t *pBest   = nullptr;
	float fBestDist  = 1024.0f;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pT = INDEXENT(i);
		if (!pT || pT == m_pEdict) continue;
		if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
		if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
		if (!CTeamFortress2Mod::TF2_IsPlayerOnFire(pT)) continue;

		float fDist = distanceFrom(pT);
		if (fDist < fBestDist && FVisible(pT))
		{
			fBestDist = fDist;
			pBest     = pT;
		}
	}

	if (!pBest)
		return false;

	// Coordinate: if another pyro is significantly closer, let them handle it
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pT = INDEXENT(i);
		if (!pT || pT == m_pEdict || !CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
		if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
		if (CClassInterface::getTF2Class(pT) != TF_CLASS_PYRO) continue;

		float fOtherDist = (CBotGlobals::entityOrigin(pT)
		                    - CBotGlobals::entityOrigin(pBest)).Length();
		if (fOtherDist < fBestDist - 80.0f)
			return false;
	}

	// In airblast range: extinguish now
	if (fBestDist < 250.0f)
	{
		if (pManmelter)
		{
			select_CWeapon(pManmelter->getWeaponInfo());
			secondaryAttack();
		}
		else
		{
			CBotWeapon *pCurrent = getCurrentWeapon();
			if (!pCurrent || !pCurrent->canDeflectRockets())
				select_CWeapon(pFlame->getWeaponInfo());
			secondaryAttack();
		}

		m_fDegreaserSwapBack = 0;
		m_iDegreaserPrevSlot = 0;
		m_fExtinguishTime    = engine->Time() + 0.5f;
		return true;
	}

	// In pursuit range: move toward them, override any schedule
	setMoveLookPriority(MOVELOOK_ATTACK);
	setMoveTo(CBotGlobals::entityOrigin(pBest));
	setLookVector(CBotGlobals::entityOrigin(pBest));
	setLookAtTask(LOOK_VECTOR);
	setMoveLookPriority(MOVELOOK_MODTHINK);
	return false;
}

bool CBotTF2::tryIgniteSniperBow()
{
	if (m_iClass != TF_CLASS_PYRO)
		return false;

	CBotWeapon *pFlame = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
	if (!pFlame || !pFlame->hasWeapon())
		return false;

	if (m_bBowIgnitePending)
	{
		if (getCurrentWeapon() != pFlame)
			return false;

		edict_t *pT = INDEXENT(m_iLastBowIgniteSniper);
		if (!pT || !CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT))
		{
			m_bBowIgnitePending = false;
			return false;
		}
		if (CTeamFortress2Mod::getTeam(pT) != m_iTeam
		    || CClassInterface::getTF2Class(pT) != TF_CLASS_SNIPER
		    || !FVisible(pT) || distanceFrom(pT) > 200.0f)
		{
			m_bBowIgnitePending = false;
			return false;
		}

		Vector vSniper = CBotGlobals::entityOrigin(pT);
		vSniper.z += 48.0f;
		Vector vLook = vSniper - getEyePosition();
		QAngle angTarget;
		VectorAngles(vLook, angTarget);
		angTarget.x = clamp(angTarget.x, -89.0f, 89.0f);
		m_vViewAngles = angTarget;

		m_pButtons->holdButton(IN_ATTACK, 0, 0.2f, 0.1f);
		m_iButtons          = m_pButtons->getBitMask();
		m_bBowIgnitePending = false;
		m_fNextBowIgnite    = engine->Time() + 3.0f;

		return true;
	}

	if (m_fNextBowIgnite > engine->Time())
		return false;

	if (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot())
		return false;

	if (pFlame->outOfAmmo(this))
		return false;

	edict_t *pBest  = nullptr;
	float fBestDist = 200.0f;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pT = INDEXENT(i);
		if (!pT || pT == m_pEdict) continue;
		if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
		if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
		if (CClassInterface::getTF2Class(pT) != TF_CLASS_SNIPER) continue;
		if (CClassInterface::getWaterLevel(pT) > 1) continue;

		edict_t *pWep = CClassInterface::TF2_getActiveWeapon(pT);
		if (!pWep || pWep->IsFree()) continue;
		if (strcmp(pWep->GetClassName(), "tf_weapon_compound_bow") != 0)
			continue;

		float fDist = distanceFrom(pT);
		if (fDist < fBestDist && FVisible(pT))
		{
			fBestDist = fDist;
			pBest     = pT;
		}
	}

	if (!pBest)
		return false;

	int iSniperIdx = ENTINDEX(pBest);
	if (iSniperIdx == m_iLastBowIgniteSniper)
		return false;

	if (getCurrentWeapon() != pFlame)
	{
		select_CWeapon(pFlame->getWeaponInfo());
		m_bBowIgnitePending    = true;
		m_iLastBowIgniteSniper = iSniperIdx;
		return false;
	}

	Vector vSniper = CBotGlobals::entityOrigin(pBest);
	vSniper.z += 48.0f;
	Vector vLook = vSniper - getEyePosition();
	QAngle angTarget;
	VectorAngles(vLook, angTarget);
	angTarget.x = clamp(angTarget.x, -89.0f, 89.0f);
	m_vViewAngles = angTarget;

	m_pButtons->holdButton(IN_ATTACK, 0, 0.2f, 0.1f);
	m_iButtons             = m_pButtons->getBitMask();
	m_fNextBowIgnite       = engine->Time() + 3.0f;
	m_iLastBowIgniteSniper = iSniperIdx;

	return true;
}

bool CBotTF2::tryCrossbowHeal()
{
	if (m_iClass != TF_CLASS_MEDIC)
		return false;

	if (m_fNextCrossbowHeal > engine->Time())
		return false;

	if (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot())
		return false;

	CBotWeapon *pCrossbow = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_CROSSBOW));
	if (!pCrossbow || !pCrossbow->hasWeapon() || pCrossbow->outOfAmmo(this))
		return false;

	edict_t *pBest   = nullptr;
	float fBestHpPct = 0.5f;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pT = INDEXENT(i);
		if (!pT || pT == m_pEdict) continue;
		if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
		if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;

		IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pT);
		if (!pInfo) continue;
		int iHp    = pInfo->GetHealth();
		int iMaxHp = pInfo->GetMaxHealth();
		float fPct = (float)iHp / (float)iMaxHp;
		if (fPct >= 0.5f || fPct <= 0.0f) continue;

		float fDist = distanceFrom(pT);
		if (fDist < 800.0f || fDist > 2500.0f) continue;
		if (!FVisible(pT)) continue;

		if (fPct < fBestHpPct)
		{
			fBestHpPct = fPct;
			pBest       = pT;
		}
	}

	if (!pBest)
		return false;

	if (getCurrentWeapon() != pCrossbow)
	{
		select_CWeapon(pCrossbow->getWeaponInfo());
		m_bCrossbowPending = true;
		return false;
	}

	if (m_bCrossbowPending)
		m_bCrossbowPending = false;

	Vector vAim = getAimVector(pBest);
	Vector vLook = vAim - getEyePosition();
	QAngle angTarget;
	VectorAngles(vLook, angTarget);
	angTarget.x = clamp(angTarget.x, -89.0f, 89.0f);
	m_vViewAngles = angTarget;

	m_pButtons->holdButton(IN_ATTACK, 0, 0.15f, 0.1f);
	m_iButtons = m_pButtons->getBitMask();
	m_fNextCrossbowHeal = engine->Time() + 2.0f;

	return true;
}

float CBotTF2::MvmTargetPriority(edict_t *pEnemy)
{
	if (!pEnemy) return 0.0f;
	if (!CBotGlobals::entityIsValid(pEnemy) || !CBotGlobals::entityIsAlive(pEnemy))
		return 0.0f;

	float fPri = 0.0f;

	// Bomb carrier is highest priority
	edict_t *pCarrier = CTeamFortress2Mod::getFlagCarrier(TF2_TEAM_BLUE);
	if (pCarrier && CBotGlobals::entityIsAlive(pCarrier) && pEnemy == pCarrier)
	{
		Vector vHatch;
		if (CTeamFortress2Mod::getMVMCapturePoint(&vHatch))
		{
			float fDist = (CBotGlobals::entityOrigin(pCarrier) - vHatch).Length();
			if (fDist < 2048.0f)
				fPri += 3.0f * (1.0f - (fDist / 2048.0f)); // up to +3.0 near hatch
			else
				fPri += 0.5f;
		}
		else
			fPri += 2.0f;
	}

	// Tank close to hatch is urgent
	if (CTeamFortress2Mod::isTankBoss(pEnemy))
	{
		Vector vHatch;
		if (CTeamFortress2Mod::getMVMCapturePoint(&vHatch))
		{
			float fDist = (CBotGlobals::entityOrigin(pEnemy) - vHatch).Length();
			if (fDist < 2048.0f)
				fPri += 2.5f * (1.0f - (fDist / 2048.0f)); // up to +2.5
		}
		else
			fPri += 1.0f;
	}

	return fPri;
}

float CBotFortress::m_fAFKIdleSince[MAX_PLAYERS + 1];
bool CBotFortress::m_bAFKStarted[MAX_PLAYERS + 1];
std::vector<MyEHandle> CBotFortress::m_SappedRobots;

void CBotTF2::handleBuildRequest(eEngiBuild iBuilding, int iWaypointFlag, edict_t *pCaller)
{
	if (getClass() != TF_CLASS_ENGINEER)
		return;

	// Cooldown to prevent spam
	if (m_fVoiceBuildTime > engine->Time())
		return;

	Vector vCallerOrigin = CBotGlobals::entityOrigin(pCaller);

	// Already carrying this building -- drop it immediately
	bool bCarrying = false;
	if (iBuilding == ENGI_SENTRY && m_bIsCarryingObj && m_bIsCarryingSentry) bCarrying = true;
	if (iBuilding == ENGI_DISP && m_bIsCarryingObj && m_bIsCarryingDisp) bCarrying = true;
	if (iBuilding == ENGI_EXIT && m_bIsCarryingObj && m_bIsCarryingTeleExit) bCarrying = true;

	if (bCarrying)
	{
		if (isVisible(pCaller) && (distanceFrom(pCaller) < 512))
		{
			setMoveTo(vCallerOrigin);
			primaryAttack();
			m_pSchedules->removeSchedule(SCHED_TF2_ENGI_MOVE_BUILDING);
			if (randomInt(0, 100) > 75)
				addVoiceCommand(TF_VC_YES);
			m_fVoiceBuildTime = engine->Time() + 15.0f;
		}
		else if (randomInt(0, 100) > 75)
			addVoiceCommand(TF_VC_NO);
		return;
	}

	// Find a suitable waypoint near the caller
	int iWpt = CWaypointLocations::NearestWaypoint(vCallerOrigin, 800, -1, true, false, true,
		nullptr, false, getTeam(), true, false, Vector(0, 0, 0), iWaypointFlag);

	if (iWpt == -1)
	{
		if (randomInt(0, 100) > 75)
			addVoiceCommand(TF_VC_NO);
		return;
	}

	CWaypoint *pWaypoint = CWaypoints::getWaypoint(iWpt);

	// Check if we already have this building built near enough
	bool bAlreadyBuilt = false;
	edict_t *pExisting = nullptr;
	if (iBuilding == ENGI_SENTRY)
		pExisting = m_pSentryGun.get();
	else if (iBuilding == ENGI_DISP)
		pExisting = m_pDispenser.get();
	else if (iBuilding == ENGI_EXIT)
		pExisting = m_pTeleExit.get();

	if (pExisting && CBotGlobals::entityIsValid(pExisting))
	{
		float fExistingDist = (CBotGlobals::entityOrigin(pExisting) - vCallerOrigin).Length();
		if (fExistingDist < 400.0f)
			bAlreadyBuilt = true;
	}

	if (bAlreadyBuilt)
	{
		if (randomInt(0, 100) > 75)
			addVoiceCommand(TF_VC_YES);
		m_fVoiceBuildTime = engine->Time() + 5.0f;
		return;
	}

	// Destroy existing if present and rebuild at new location
	CBotTFEngiBuild *pSchedule = new CBotTFEngiBuild(this, iBuilding, pWaypoint);
	m_pSchedules->addFront(pSchedule);

	if (randomInt(0, 100) > 75)
		addVoiceCommand(TF_VC_YES);
	m_fVoiceBuildTime = engine->Time() + 15.0f;
}

void CBotTF2::handleSpecialAbilities()
{
	// In danger = active enemy visible and wanting to fight
	bool bInDanger = (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());

	// --- Pyro Thermal Thruster (1179): travel launch toward destination ---
	if (m_iClass == TF_CLASS_PYRO && !bInDanger
	    && m_fThermalThrustTime < engine->Time())
	{
		edict_t *pThrustEnt = nullptr;
		CBaseHandle *pList = CClassInterface::getWeaponList(m_pEdict);
		if (pList)
		{
			for (int i = 0; i < MAX_WEAPONS; i++)
			{
				CBaseHandle &h = pList[i];
				if (!h.IsValid()) continue;
				edict_t *pWep = INDEXENT(h.GetEntryIndex());
				if (pWep && !pWep->IsFree()
				    && CClassInterface::TF2_getItemDefinitionIndex(pWep) == 1179)
				{
					pThrustEnt = pWep;
					break;
				}
			}
		}

		if (pThrustEnt)
		{
			float *pMeter = CClassInterface::getItemChargeMeter(m_pEdict);
			bool bHasCharge = pMeter && (pMeter[1] >= 50.0f);
			if (!bHasCharge)
			{
				edict_t *pActive = CClassInterface::getCurrentWeapon(m_pEdict);
				if (pActive && CClassInterface::TF2_getItemDefinitionIndex(pActive) == 1179)
					selectWeapon(engine->IndexOfEdict(CWeapons::findWeapon(m_pEdict, "tf_weapon_flamethrower")));
				if (m_bThrusterSwitchPending)
					m_bThrusterSwitchPending = false;
				m_fThermalThrustTime = engine->Time() + 0.5f;
			}
			else
			{
			Vector vDst = m_bMoveToIsValid ? m_vMoveTo
			    : (getOrigin() + (m_vLookAt - getOrigin()));
			float fDist = (getOrigin() - vDst).Length2D();
			if (fDist > 350.0f)
			{
				Vector vHead = getOrigin() + Vector(0, 0, 72.0f);
				CTraceFilterWorldAndPropsOnly filter;
				CBotGlobals::traceLine(vHead, vHead + Vector(0, 0, 512.0f),
				    MASK_SOLID_BRUSHONLY, &filter);
				if (CBotGlobals::getTraceResult()->fraction >= 1.0f)
				{
					if (!m_bThrusterSwitchPending)
					{
						selectWeapon(engine->IndexOfEdict(pThrustEnt));
						m_fThermalThrustTime = engine->Time() + 1.2f;
						m_bThrusterSwitchPending = true;
					}
					else
					{
						edict_t *pCurWep = CClassInterface::getCurrentWeapon(m_pEdict);
						if (pCurWep && CClassInterface::TF2_getItemDefinitionIndex(pCurWep) == 1179)
						{
							Vector vDir = vDst - getOrigin();
							vDir.z = 0;
							float fLen = vDir.Length();
							if (fLen > 0.1f)
							{
								vDir = vDir / fLen;
								Vector vAim = vDir + Vector(0, 0, 1.2f);
								QAngle aimAngles;
								VectorAngles(vAim, aimAngles);
								m_vViewAngles = aimAngles;
							}
							primaryAttack(true, 0.5f);
							doButtons();
							float *pMeterPost = CClassInterface::getItemChargeMeter(m_pEdict);
							if (!pMeterPost || pMeterPost[1] < 50.0f)
								selectWeapon(engine->IndexOfEdict(CWeapons::findWeapon(m_pEdict, "tf_weapon_flamethrower")));
						}
						m_fThermalThrustTime = engine->Time() + 2.5f;
						m_bThrusterSwitchPending = false;
					}
				}
			}
			}
		}
	}

	// --- Pyro Thermal Thruster (1179): combat launch toward enemy ---
	if (m_iClass == TF_CLASS_PYRO && bInDanger && m_pEnemy
	    && m_fThermalThrustTime < engine->Time())
	{
		float fEnemyDist = distanceFrom(m_pEnemy);
		if (fEnemyDist > 600.0f)
		{
			edict_t *pThrustEnt = nullptr;
			CBaseHandle *pList = CClassInterface::getWeaponList(m_pEdict);
			if (pList)
			{
				for (int i = 0; i < MAX_WEAPONS; i++)
				{
					CBaseHandle &h = pList[i];
					if (!h.IsValid()) continue;
					edict_t *pWep = INDEXENT(h.GetEntryIndex());
					if (pWep && !pWep->IsFree()
					    && CClassInterface::TF2_getItemDefinitionIndex(pWep) == 1179)
					{
						pThrustEnt = pWep;
						break;
					}
				}
			}

			if (pThrustEnt)
			{
				float *pMeter = CClassInterface::getItemChargeMeter(m_pEdict);
				bool bHasCharge = pMeter && (pMeter[1] >= 50.0f);
				if (!bHasCharge)
				{
					edict_t *pActive = CClassInterface::getCurrentWeapon(m_pEdict);
					if (pActive && CClassInterface::TF2_getItemDefinitionIndex(pActive) == 1179)
						selectWeapon(engine->IndexOfEdict(CWeapons::findWeapon(m_pEdict, "tf_weapon_flamethrower")));
					if (m_bThrusterSwitchPending)
						m_bThrusterSwitchPending = false;
					m_fThermalThrustTime = engine->Time() + 0.5f;
				}
				else
				{
				Vector vDst = CBotGlobals::entityOrigin(m_pEnemy);
				if (!m_bThrusterSwitchPending)
				{
					selectWeapon(engine->IndexOfEdict(pThrustEnt));
					m_fThermalThrustTime = engine->Time() + 0.8f;
					m_bThrusterSwitchPending = true;
				}
				else
				{
					if (distanceFrom(m_pEnemy) < 400.0f)
					{
						selectWeapon(engine->IndexOfEdict(CWeapons::findWeapon(m_pEdict, "tf_weapon_flamethrower")));
						m_bThrusterSwitchPending = false;
						m_fThermalThrustTime = engine->Time() + 1.0f;
					}
					else
					{
					edict_t *pCurWep = CClassInterface::getCurrentWeapon(m_pEdict);
					if (pCurWep && CClassInterface::TF2_getItemDefinitionIndex(pCurWep) == 1179)
					{
						Vector vDir = vDst - getOrigin();
						vDir.z = 0;
						float fLen = vDir.Length();
						if (fLen > 0.1f)
						{
							vDir = vDir / fLen;
							Vector vAim = vDir + Vector(0, 0, 1.2f);
							QAngle aimAngles;
							VectorAngles(vAim, aimAngles);
							m_vViewAngles = aimAngles;
						}
						primaryAttack(true, 0.5f);
						doButtons();
						float *pMeterPost = CClassInterface::getItemChargeMeter(m_pEdict);
						if (!pMeterPost || pMeterPost[1] < 50.0f)
							selectWeapon(engine->IndexOfEdict(CWeapons::findWeapon(m_pEdict, "tf_weapon_flamethrower")));
					}
					m_fThermalThrustTime = engine->Time() + 4.0f;
					m_bThrusterSwitchPending = false;
					}
				}
				}
			}
		}
	}

	edict_t *pActiveWep = CClassInterface::getCurrentWeapon(m_pEdict);
	int iActiveItem     = pActiveWep ? CClassInterface::TF2_getItemDefinitionIndex(pActiveWep) : 0;

	CBotWeapon *pCurWep = getCurrentWeapon();
	if (!pCurWep) return;
	int iActiveSlot = pCurWep->getWeaponInfo()->getSlot();

	// --- Charge-based weapons: use secondary fire when meter is full ---
	// Soda Popper (448): use hype charge
	if (iActiveItem == 448 && iActiveSlot == TF2_SLOT_PRMRY && bInDanger)
	{
		if (CClassInterface::getHypeMeter(m_pEdict) > 99.9f)
		{
			secondaryAttack();
			return;
		}
	}
	// Cow Mangler (441): use energy charged shot
	if (iActiveItem == 441 && iActiveSlot == TF2_SLOT_PRMRY && bInDanger)
	{
		edict_t *pWep = CClassInterface::getCurrentWeapon(m_pEdict);
		if (pWep && CClassInterface::getWeaponEnergy(pWep) > 19.9f && randomInt(0, 1) == 0)
		{
			secondaryAttack();
			return;
		}
	}

	// --- Phlogistinator (594): auto-taunt when rage is full ---
	if (iActiveItem == 594 && bInDanger)
	{
		if (CClassInterface::getRageMeter(m_pEdict) > 99.9f)
			helpers->ClientCommand(m_pEdict, "taunt");
	}

	// --- Wrangler variants (140/1086/30668): hold secondary fire for shield ---
	if ((iActiveItem == 140 || iActiveItem == 1086 || iActiveItem == 30668)
	    && m_pSentryGun.get() && CBotGlobals::entityIsAlive(m_pSentryGun.get()))
	{
		if (iActiveSlot == TF2_SLOT_SCNDR && bInDanger)
			secondaryAttack();
	}

	// --- Wrangler: unequip if no sentry or too far from it ---
	if ((iActiveItem == 140 || iActiveItem == 1086 || iActiveItem == 30668)
	    && iActiveSlot == TF2_SLOT_SCNDR
	    && (!m_pSentryGun.get() || !CBotGlobals::entityIsAlive(m_pSentryGun.get())
	        || distanceFrom(m_pSentryGun.get()) > 256.0f))
	{
		if (randomInt(0, 1) == 0)
		{
			CBotWeapon *pWrench = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));
			if (pWrench && pWrench->hasWeapon())
				select_CWeapon(pWrench->getWeaponInfo());
		}
	}

	// --- Cleaner's Carbine (751): random secondary fire ---
	if (iActiveItem == 751 && iActiveSlot == TF2_SLOT_SCNDR && bInDanger && randomInt(0, 1) == 0)
		secondaryAttack();

	// --- Ball weapons (Sandman 44, Wrap Assassin 648): fire secondary ---
	if ((iActiveItem == 44 || iActiveItem == 648) && iActiveSlot == TF2_SLOT_MELEE && bInDanger)
		secondaryAttack();

	// --- Heavy food items: eat when low HP, throw otherwise ---
	// 42 = Sandvich, 863 = Dalokohs, 1002 = Fishcake, 159/433/1190 = other lunchbox items
	if ((iActiveItem == 42 || iActiveItem == 863 || iActiveItem == 1002
	     || iActiveItem == 159 || iActiveItem == 433 || iActiveItem == 1190)
	    && iActiveSlot == TF2_SLOT_SCNDR)
	{
		if (getHealthPercent() < 0.5f && !bInDanger)
		{
			// Eat it
			primaryAttack();
		}
		else if (bInDanger && randomInt(0, 1) == 0)
		{
			// Throw it for teammates
			secondaryAttack();
		}
	}

	// --- Buffalo Steak (311): eat when low ammo or randomly ---
	if (iActiveItem == 311 && iActiveSlot == TF2_SLOT_SCNDR && bInDanger && randomInt(0, 1) == 0)
		primaryAttack();

	// --- Eureka Effect (589): teleport home when low HP and not in danger ---
	if (iActiveItem == 589 && iActiveSlot == TF2_SLOT_MELEE
	    && getHealthPercent() < 0.4f && !bInDanger
	    && m_fLastEurekaTeleport < engine->Time())
	{
		helpers->ClientCommand(m_pEdict, "eureka_teleport");
		m_fLastEurekaTeleport = engine->Time() + 5.0f;
	}

	// --- Demoman: shield charge when no secondary weapon (shield in that slot) ---
	if (m_iClass == TF_CLASS_DEMOMAN && iActiveSlot == TF2_SLOT_MELEE && bInDanger)
	{
		CBotWeapon *pSec = m_pWeapons->getCurrentWeaponInSlot(TF2_SLOT_SCNDR);
		if (!pSec && randomInt(0, 1) == 0)
			secondaryAttack();
	}

	// --- Medic Amputator (304): taunt for AoE heal when low HP and not in danger ---
	if (iActiveItem == 304 && iActiveSlot == TF2_SLOT_MELEE
	    && getHealthPercent() < 0.5f && !bInDanger)
	{
		helpers->ClientCommand(m_pEdict, "taunt");
	}

	// --- Medic crossbow: randomly fire ---
	if (m_iClass == TF_CLASS_MEDIC && iActiveSlot == TF2_SLOT_PRMRY && bInDanger && randomInt(0, 1) == 0)
		primaryAttack();

	// --- Scout: random double-jump in combat ---
	if (m_iClass == TF_CLASS_SCOUT && bInDanger && randomInt(1, 25) == 1)
		tapButton(IN_JUMP);

	// --- Sniper / Scout: don't throw Jarate/Milk at already debuffed targets or tanks ---
	if ((iActiveItem == 58 || iActiveItem == 1083 || iActiveItem == 1105) // Jarate variants
	    && m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
	    && !CTeamFortress2Mod::isTankBoss(m_pEnemy))
	{
		int iCond = CClassInterface::getTF2Conditions(m_pEnemy);
		if (iCond & (1 << 24)) // already Jarated
			return;
	}
	if ((iActiveItem == 222 || iActiveItem == 1121) // Mad Milk variants
	    && m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
	    && !CTeamFortress2Mod::isTankBoss(m_pEnemy))
	{
		int iCond = CClassInterface::getTF2Conditions(m_pEnemy);
		if (iCond & (1 << 27)) // already Milked
			return;
	}

	// --- Engineer Short Circuit (528): destroy incoming projectiles ---
	if (m_iClass == TF_CLASS_ENGINEER && bInDanger)
	{
		CBotWeapon *pShortCircuit = m_pWeapons->getCurrentWeaponInSlot(TF2_SLOT_SCNDR);
		if (pShortCircuit)
		{
			edict_t *pSCEnt = pShortCircuit->getWeaponEntity();
			if (pSCEnt && CClassInterface::TF2_getItemDefinitionIndex(pSCEnt) == 528
			    && pShortCircuit->getAmmo(this) >= 65)
			{
				edict_t *pProj = m_NearestEnemyRocket.get();
				if (!pProj) pProj = m_NearestEnemyGrenade.get();
				if (pProj && CBotGlobals::entityIsAlive(pProj)
				    && incomingRocket(300.0f))
				{
					select_CWeapon(pShortCircuit->getWeaponInfo());
					secondaryAttack();
				}
			}
		}
}
}

void CBotFortress::notePlayerEngaged(edict_t *pPlayer)
{
	if (!pPlayer) return;
	int idx = ENTINDEX(pPlayer);
	if (idx > 0 && idx <= MAX_PLAYERS)
		m_bAFKStarted[idx] = true;
}

void CBotFortress::addSappedRobot(edict_t *pRobot)
{
	if (!pRobot) return;
	m_SappedRobots.push_back(MyEHandle(pRobot));
}

bool CBotFortress::isPlayerAFK(edict_t *pPlayer)
{
	if (!pPlayer || !CBotGlobals::entityIsValid(pPlayer))
		return false;

	int idx = ENTINDEX(pPlayer);
	if (idx <= 0 || idx > MAX_PLAYERS)
		return false;

	// Only track players that a medic engaged with or a bot sought out
	if (!m_bAFKStarted[idx])
		return false;

	IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pPlayer);
	if (!pInfo)
		return false;

	const CBotCmd &cmd = pInfo->GetLastUserCommand();
	bool bActive = (cmd.buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_ATTACK)) != 0;

	if (!bActive)
	{
		Vector vVel;
		CClassInterface::getVelocity(pPlayer, &vVel);
		if (vVel.Length() > 10.0f)
			bActive = true;
	}

	// Bots that have an enemy are never AFK
	if (!bActive)
	{
		CBot *pOtherBot = CBots::getBotPointer(pPlayer);
		if (pOtherBot && pOtherBot->hasEnemy())
			bActive = true;
	}

	// Player moved -- reset everything
	if (bActive)
	{
		m_fAFKIdleSince[idx] = 0.0f;
		m_bAFKStarted[idx]   = false;
		return false;
	}

	// Track idle time
	if (m_fAFKIdleSince[idx] == 0.0f)
		m_fAFKIdleSince[idx] = engine->Time();

	return (engine->Time() - m_fAFKIdleSince[idx]) > 10.0f;
}

void CBotFortress::chooseClass()
{
	const int _forcedClass = rcbot_force_class.GetInt();
	if (_forcedClass > 0 && _forcedClass < 10)
	{
		switch (_forcedClass)
		{
		case 1:
			m_iDesiredClass = TF_CLASS_SCOUT;
			break;
		case 2:
			m_iDesiredClass = TF_CLASS_SOLDIER;
			break;
		case 3:
			m_iDesiredClass = TF_CLASS_PYRO;
			break;
		case 4:
			m_iDesiredClass = TF_CLASS_DEMOMAN;
			break;
		case 5:
			m_iDesiredClass = TF_CLASS_HWGUY;
			break;
		case 6:
			m_iDesiredClass = TF_CLASS_ENGINEER;
			break;
		case 7:
			m_iDesiredClass = TF_CLASS_MEDIC;
			break;
		case 8:
			m_iDesiredClass = TF_CLASS_SNIPER;
			break;
		case 9:
			m_iDesiredClass = TF_CLASS_SPY;
			break;
		}
	}
	else
	{
		float fClassFitness[10];
		float fTotalFitness = 0;
		float fRandom;

		int iNumMedics = 0;
		int i          = 0;
		int iTeam      = getTeam();
		int iClass;
		edict_t *pPlayer;

		for (i = 1; i < 10; i++)
			fClassFitness[i] = 1.0f;

		if ((m_iClass >= 0) && (m_iClass < 10))
			fClassFitness[m_iClass] = 0.1f;

		for (i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = INDEXENT(i);

			if (CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
			{
				iClass = CClassInterface::getTF2Class(pPlayer);

				if (iClass == TF_CLASS_MEDIC)
					iNumMedics++;

				if ((iClass >= 0) && (iClass < 10))
					fClassFitness[iClass] *= 0.6f;
			}
		}

		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		{
			fClassFitness[TF_CLASS_ENGINEER] *= 1.5;
			fClassFitness[TF_CLASS_SPY] *= 0.6;
			fClassFitness[TF_CLASS_SCOUT] *= 0.6;
			fClassFitness[TF_CLASS_HWGUY] *= 1.5;
			fClassFitness[TF_CLASS_MEDIC] *= 1.1;
			fClassFitness[TF_CLASS_SOLDIER] *= 1.5;
			fClassFitness[TF_CLASS_DEMOMAN] *= 1.4;
			fClassFitness[TF_CLASS_PYRO] *= 1.6;
			// attacking team?
		}
		else if (CTeamFortress2Mod::isAttackDefendMap())
		{
			if (getTeam() == TF2_TEAM_BLUE)
			{
				fClassFitness[TF_CLASS_ENGINEER] *= 0.75;
				fClassFitness[TF_CLASS_SPY] *= 1.25;
				fClassFitness[TF_CLASS_SCOUT] *= 1.05;
			}
			else
			{
				fClassFitness[TF_CLASS_ENGINEER] *= 2.0;
				fClassFitness[TF_CLASS_SCOUT] *= 0.5;
				fClassFitness[TF_CLASS_HWGUY] *= 1.5;
				fClassFitness[TF_CLASS_MEDIC] *= 1.1;
			}
		}
		else if (CTeamFortress2Mod::isMapType(TF_MAP_CP))
			fClassFitness[TF_CLASS_SCOUT] *= 1.2f;

		if (m_pLastEnemySentry.get() != nullptr || !m_KnownSentries.empty())
		{
			fClassFitness[TF_CLASS_SPY] *= 1.25;
			fClassFitness[TF_CLASS_DEMOMAN] *= 1.3;
		}

		if (iNumMedics == 0)
			fClassFitness[TF_CLASS_MEDIC] *= 2.0f;

		for (int i = 1; i < 10; i++)
			fTotalFitness += fClassFitness[i];

		fRandom         = randomFloat(0, fTotalFitness);

		fTotalFitness   = 0;

		m_iDesiredClass = 0;

		for (int i = 1; i < 10; i++)
		{
			fTotalFitness += fClassFitness[i];

			if (fRandom <= fTotalFitness)
			{
				m_iDesiredClass = i;
				break;
			}
		}
	}
}

void CBotFortress::updateConditions()
{
	CBot::updateConditions();

	if (CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::withinEndOfRound(29.0f))
		updateCondition(CONDITION_PUSH);

	if (m_iClass == TF_CLASS_ENGINEER)
	{
		if (CTeamFortress2Mod::isMySentrySapped(getEdict()) || CTeamFortress2Mod::isMyTeleporterSapped(getEdict())
		    || CTeamFortress2Mod::isMyDispenserSapped(getEdict()))
		{
			updateCondition(CONDITION_BUILDING_SAPPED);
			updateCondition(CONDITION_PARANOID);

			/*if ( (m_fLastSeeSpyTime + 10.0f) > engine->Time() )
			{
			    m_vLastSeeSpy = m_vSentryGun;
			    m_fLastSeeSpyTime = engine->Time();
			}*/
		}
		else
			removeCondition(CONDITION_BUILDING_SAPPED);
	}
}

void CBotTF2::onInventoryApplication()
{
}

bool m_classWasForced = false;

void CBotTF2::modThink()
{
	CTeamFortress2Mod::computeTeamDominance();

	static bool bNeedHealth;
	static bool bNeedAmmo;
	static bool bIsCloaked;

	// FIX: MUST Update class
	m_iClass = (TF_Class)CClassInterface::getTF2Class(m_pEdict);

	if (CTeamFortress2Mod::isLosingTeam(m_iTeam))
		wantToShoot(false);
	// if ( m_pWeapons ) // done in bot.cpp
	//	m_pWeapons->update(false); // don't override ammo types from engine

	bNeedHealth = hasSomeConditions(CONDITION_NEED_HEALTH);
	bNeedAmmo   = hasSomeConditions(CONDITION_NEED_AMMO);

	// mod specific think code here
	CBotFortress::modThink();

	// Tank: keep moving toward the tank even when visibility drops
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	    && (m_iClass == TF_CLASS_PYRO || m_iClass == TF_CLASS_SOLDIER
	        || m_iClass == TF_CLASS_DEMOMAN || m_iClass == TF_CLASS_SCOUT || m_iClass == TF_CLASS_HWGUY))
	{
		edict_t *pTank = CTeamFortress2Mod::getNearestTank();
		if (pTank && CBotGlobals::entityIsAlive(pTank))
		{
			Vector vTankPos = CBotGlobals::entityOrigin(pTank);
			if ((vTankPos - getOrigin()).Length() < 400.0f)
				setMoveTo(vTankPos);
		}
	}

	// Update team-shared enemy approach direction when we see an enemy near an objective
	if (m_pEnemy && CBotGlobals::entityIsValid(m_pEnemy) && CBotGlobals::entityIsAlive(m_pEnemy)
	    && (m_iCurrentDefendArea > 0 || m_iCurrentAttackArea > 0))
	{
		Vector vObj = getOrigin();
		bool bHasObj = false;
		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		{
			bHasObj = CTeamFortress2Mod::getMVMCapturePoint(&vObj);
		}
		else if (CTeamFortress2Mod::isMapType(TF_MAP_CTF))
		{
			bHasObj = CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE, &vObj);
		}
		if (bHasObj)
			CTeamFortress2Mod::UpdateEnemyApproachDir(CBotGlobals::entityOrigin(m_pEnemy), vObj, m_iTeam);
	}

	// Count reflected projectiles by enemy Pyros for firing-pattern adaptation
	{
		edict_t *pReflected = m_NearestEnemyRocket.get();
		if (!pReflected) pReflected = m_pNearestPipeGren.get();

		if (pReflected && CBotGlobals::entityIsValid(pReflected)
		    && (!m_iLastReflectedRocket || ENTINDEX(pReflected) != m_iLastReflectedRocket)
		    && (!m_iLastReflectedGrenade || ENTINDEX(pReflected) != m_iLastReflectedGrenade)
		    && incomingRocket(800.0f))
		{
			// Find nearest visible enemy Pyro within 800u
			edict_t *pPyro = nullptr;
			for (int i = 1; i <= CBotGlobals::maxClients(); i++)
			{
				edict_t *pEd = INDEXENT(i);
				if (!CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
				if (CClassInterface::getTF2Class(pEd) != TF_CLASS_PYRO) continue;
				if (CClassInterface::getTeam(pEd) == getTeam()) continue;
				float fDist = distanceFrom(pEd);
				if (fDist < 800.0f)
				{
					pPyro = pEd;
					break;
				}
			}

			if (pPyro)
			{
				int iIdx = ENTINDEX(pPyro) - 1;
				if (iIdx >= 0 && iIdx < MAX_PLAYERS)
					m_iReflectCount[iIdx]++;
			}

			if (pReflected == m_NearestEnemyRocket.get())
				m_iLastReflectedRocket  = ENTINDEX(pReflected);
			else
				m_iLastReflectedGrenade = ENTINDEX(pReflected);
		}
	}

	// Per-frame projectile dodge: works even when not in active combat,
	// e.g. while pathing, retreating, or repositioning
	if (m_fStrafeTime < engine->Time())
	{
		edict_t *pIncoming = m_NearestEnemyRocket.get();
		if (!pIncoming)
			pIncoming = m_pNearestPipeGren.get();

		if (pIncoming && CBotGlobals::entityIsValid(pIncoming)
		    && CBotGlobals::entityIsAlive(pIncoming)
		    && incomingRocket(800.0f))
		{
			Vector vOrigin  = getOrigin();
			Vector vProjOrg = CBotGlobals::entityOrigin(pIncoming);
			Vector vDodge   = vOrigin - vProjOrg;

			// If a second projectile threatens from a different angle,
			// dodge perpendicular to avoid walking into it
			edict_t *pSecond = m_SecondNearestEnemyRocket.get();
			if (pSecond && pSecond != pIncoming
			    && CBotGlobals::entityIsValid(pSecond)
			    && CBotGlobals::entityIsAlive(pSecond)
			    && distanceFrom(pSecond) < 800.0f)
			{
				Vector vProj2Org = CBotGlobals::entityOrigin(pSecond);
				Vector vToBot2   = vOrigin - vProj2Org;
				vToBot2.z        = 0;
				vDodge.z         = 0;
				if (vToBot2.Length2D() > 0.1f && vDodge.Length2D() > 0.1f)
				{
					vToBot2 = vToBot2 / vToBot2.Length2D();
					vDodge  = vDodge / vDodge.Length2D();
					// If dodging away from one would move toward the other (angle > 120°)
					if (vDodge.Dot(vToBot2) < -0.5f)
					{
						float fTemp = vDodge.x;
						vDodge.x    = -vDodge.y;
						vDodge.y    = fTemp;
					}
				}
			}

			vDodge.z        = 0;
			float fLen      = vDodge.Length2D();
			if (fLen > 0.1f)
			{
				vDodge        = vDodge / fLen;
				Vector vDest  = vOrigin + vDodge * (BLAST_RADIUS + 128.0f);

			// Trace-validate dodge destination; try lateral alternatives if blocked
			CTraceFilterWorldAndPropsOnly filter;
			CBotGlobals::traceLine(vOrigin, vDest, MASK_SOLID_BRUSHONLY, &filter);
			trace_t *tr = CBotGlobals::getTraceResult();
			if (tr->fraction < 1.0f)
			{
				Vector vAlt1(-vDodge.y, vDodge.x, 0);
				Vector vAlt2(vDodge.y, -vDodge.x, 0);
				Vector vDest1 = vOrigin + vAlt1 * (BLAST_RADIUS + 128.0f);
				Vector vDest2 = vOrigin + vAlt2 * (BLAST_RADIUS + 128.0f);

				CBotGlobals::traceLine(vOrigin, vDest1, MASK_SOLID_BRUSHONLY, &filter);
				tr = CBotGlobals::getTraceResult();
				if (tr->fraction < 1.0f)
				{
					CBotGlobals::traceLine(vOrigin, vDest2, MASK_SOLID_BRUSHONLY, &filter);
					tr    = CBotGlobals::getTraceResult();
					vDest = (tr->fraction >= 1.0f) ? vDest2 : vOrigin;
				}
				else
					vDest = vDest1;
			}

				if ((vDest - vOrigin).Length2D() > 10.0f)
				{
					setMoveTo(vDest);
					m_fStrafeTime = engine->Time() + 0.2f;
					m_fSideSpeed  = (vDodge.y > 0 ? 1.0f : -1.0f) * m_fIdealMoveSpeed;
				}
			}
		}
	}

	checkBeingHealed();

	if (wantToListen())
	{
		if ((m_pNearestAllySentry.get() != nullptr)
		    && (CClassInterface::getSentryEnemy(m_pNearestAllySentry) != nullptr))
		{
			m_PlayerListeningTo    = m_pNearestAllySentry;
			m_bListenPositionValid = true;
			m_fListenTime          = engine->Time() + randomFloat(1.0f, 2.0f);
			setLookAtTask(LOOK_NOISE);
			m_fLookSetTime    = m_fListenTime;
			m_vListenPosition = CBotGlobals::entityOrigin(m_pNearestAllySentry.get());
		}
	}

	if (CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE))
	{
		if (getTeam() == TF2_TEAM_BLUE)
		{
			m_pDefendPayloadBomb = m_pRedPayloadBomb;
			m_pPushPayloadBomb   = m_pBluePayloadBomb;
		}
		else
		{
			m_pDefendPayloadBomb = m_pBluePayloadBomb;
			m_pPushPayloadBomb   = m_pRedPayloadBomb;
		}
	}
	else if (CTeamFortress2Mod::isMapType(TF_MAP_CART))
	{
		if (getTeam() == TF2_TEAM_BLUE)
		{
			m_pPushPayloadBomb   = m_pBluePayloadBomb;
			m_pDefendPayloadBomb = nullptr;
		}
		else
		{
			m_pPushPayloadBomb   = nullptr;
			m_pDefendPayloadBomb = m_pBluePayloadBomb;
		}
	}

	// Keep known sentries pruned of dead entries
	if (!m_KnownSentries.empty())
	{
		for (size_t i = 0; i < m_KnownSentries.size();)
		{
			edict_t *pSentry = m_KnownSentries[i].get();
			if (!pSentry || !CBotGlobals::entityIsValid(pSentry)
			    || !CBotGlobals::entityIsAlive(pSentry))
			{
				m_KnownSentries.erase(m_KnownSentries.begin() + i);
				continue;
			}
			i++;
		}
	}

	pruneKnownEnemyBuildings();

	// MvM sentry buster avoidance
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		edict_t *pBuster   = nullptr;
		float fBusterDist  = 9999.0f;
		bool bTaunting     = false;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t *pEnt = INDEXENT(i);
			if (!pEnt || pEnt->IsFree() || !pEnt->GetUnknown()) continue;
			if (!CBotGlobals::entityIsValid(pEnt) || !CBotGlobals::entityIsAlive(pEnt)) continue;

			IServerEntity *pServerEnt = pEnt->GetIServerEntity();
			if (!pServerEnt) continue;
			const char *szModel = pServerEnt->GetModelName().ToCStr();
			if (!szModel || strstr(szModel, "sentry_buster") == nullptr) continue;

			float fDist = distanceFrom(pEnt);
			if (fDist < fBusterDist)
			{
				fBusterDist = fDist;
				pBuster     = pEnt;
				int iConds  = CClassInterface::getTF2Conditions(pEnt);
				bTaunting   = (iConds & (1 << 7)) != 0; // TFCond_Taunting
			}
		}

		const float fBlastRadius = 600.0f;

		if (pBuster && bTaunting && fBusterDist < fBlastRadius)
		{
			// Engineer: try to rescue sentry before fleeing
			if (m_iClass == TF_CLASS_ENGINEER && m_pSentryGun.get()
			    && CBotGlobals::entityIsValid(m_pSentryGun)
			    && !m_bIsCarryingObj
			    && distanceFrom(m_pSentryGun) < 250.0f
			    && FVisible(m_pSentryGun))
			{
				CBotWeapon *pRescue = m_pWeapons->getWeapon(
				    CWeapons::getWeapon(TF2_WEAPON_SHOTGUN_PRIMARY));
				edict_t *pRescueEnt = pRescue ? pRescue->getWeaponEntity() : nullptr;
				if (pRescueEnt && CClassInterface::TF2_getItemDefinitionIndex(pRescueEnt) == 997
				    && pRescue->getAmmo(this) >= 60)
				{
					lookAtEdict(m_pSentryGun);
					select_CWeapon(pRescue->getWeaponInfo());
					secondaryAttack();
					doButtons();
					resetCarryTime();
				}
				else
				{
					CBotWeapon *pWrench = m_pWeapons->getWeapon(
					    CWeapons::getWeapon(TF2_WEAPON_WRENCH));
					if (pWrench && pWrench->hasWeapon())
					{
						select_CWeapon(pWrench->getWeaponInfo());
						secondaryAttack();
						doButtons();
						resetCarryTime();
					}
				}
			}

			// All classes: flee using proper hide-spot navigation
			m_pSchedules->freeMemory();
			m_pSchedules->addFront(new CGotoHideSpotSched(this, pBuster));
		}
	}

	// Engineer: remote repair buildings with Rescue Ranger (item 997)
	if (m_iClass == TF_CLASS_ENGINEER && !m_bIsCarryingObj)
	{
		CBotWeapon *pRescue = m_pWeapons->getWeapon(
		    CWeapons::getWeapon(TF2_WEAPON_SHOTGUN_PRIMARY));
		edict_t *pRescueEnt = pRescue ? pRescue->getWeaponEntity() : nullptr;
		if (pRescueEnt && CClassInterface::TF2_getItemDefinitionIndex(pRescueEnt) == 997
		    && pRescue->getAmmo(this) >= 60)
		{
			edict_t *pTarget = nullptr;
			if (m_pSentryGun.get() && CBotGlobals::entityIsValid(m_pSentryGun)
			    && CClassInterface::getSentryHealth(m_pSentryGun) < 150.0f)
				pTarget = m_pSentryGun;
			else if (m_pDispenser.get() && CBotGlobals::entityIsValid(m_pDispenser)
			    && CClassInterface::getDispenserHealth(m_pDispenser) < 100.0f)
				pTarget = m_pDispenser;
			else if (m_pTeleEntrance.get() && CBotGlobals::entityIsValid(m_pTeleEntrance)
			    && CClassInterface::getTeleporterHealth(m_pTeleEntrance) < 100.0f)
				pTarget = m_pTeleEntrance;
			else if (m_pTeleExit.get() && CBotGlobals::entityIsValid(m_pTeleExit)
			    && CClassInterface::getTeleporterHealth(m_pTeleExit) < 100.0f)
				pTarget = m_pTeleExit;

			if (pTarget && FVisible(pTarget) && distanceFrom(pTarget) > 180.0f)
			{
				lookAtEdict(pTarget);
				setLookAtTask(LOOK_EDICT);
				select_CWeapon(pRescue->getWeaponInfo());
				secondaryAttack();
				doButtons();
			}
		}
	}

	// Also avoid visible sentries we're not actively engaging
	if (m_pNearestEnemySentry.get() != nullptr
	    && !(m_iClass == TF_CLASS_SPY && (isDisguised() || isCloaked()))
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		edict_t *pSentry = m_pNearestEnemySentry.get();
		if (CBotGlobals::entityIsValid(pSentry) && CBotGlobals::entityIsAlive(pSentry)
		    && isVisible(pSentry)
		    && !m_pSchedules->hasSchedule(SCHED_ATTACK_SENTRY_GUN)
		    && !m_pSchedules->isCurrentSchedule(SCHED_ATTACK_SENTRY_GUN))
		{
			float fDist = distanceFrom(pSentry);
			if (fDist < (TF2_MAX_SENTRYGUN_RANGE + 128.0f))
			{
				Vector vSentryPos = CBotGlobals::entityOrigin(pSentry);
				Vector vAway      = getOrigin() - vSentryPos;
				vAway.z           = 0;
				if (vAway.Length() > 0.1f)
				{
					vAway = vAway / vAway.Length();
					setMoveTo(getOrigin() + (vAway * 384.0f));
				}
			}
		}
	}

	// Also avoid known sentry positions (team-memory or personal memory)
	if (!(m_iClass == TF_CLASS_SPY && (isDisguised() || isCloaked()))
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
	    && !m_pSchedules->hasSchedule(SCHED_ATTACK_SENTRY_GUN)
	    && !m_pSchedules->isCurrentSchedule(SCHED_ATTACK_SENTRY_GUN))
	{
		for (auto &h : m_KnownSentries)
		{
			edict_t *pKnown = h.get();
			if (!pKnown || !CBotGlobals::entityIsValid(pKnown)
			    || !CBotGlobals::entityIsAlive(pKnown)) continue;
			float fDist = distanceFrom(pKnown);
			if (fDist < (TF2_MAX_SENTRYGUN_RANGE + 128.0f))
			{
				Vector vPos = CBotGlobals::entityOrigin(pKnown);
				Vector vAway = getOrigin() - vPos;
				vAway.z = 0;
				if (vAway.Length() > 0.1f)
				{
					vAway = vAway / vAway.Length();
					setMoveTo(getOrigin() + (vAway * 384.0f));
					break;
				}
			}
		}
	}

	// when respawned -- check if I should change class
	if (!m_pPlayerInfo->IsDead() && !m_bHijacked)
	{
		const int _forcedClass = rcbot_force_class.GetInt();
		// Change class if not same class as forced one or class was forced but not anymore
		if (m_iClass != _forcedClass
		    && ((_forcedClass > 0 && _forcedClass < 10)
		        || (m_classWasForced && (_forcedClass < 1 || _forcedClass > 9))))
		{
			m_classWasForced = _forcedClass > 0 && _forcedClass < 10;
			chooseClass();
			selectClass();
		}
		else if (m_bCheckClass)
		{
			m_bCheckClass = false;

			if (bot_change_class.GetBool() && (m_fChangeClassTime < engine->Time())
			    && (!CTeamFortress2Mod::isMapType(TF_MAP_MVM) || !CTeamFortress2Mod::hasRoundStarted()))
			{
				// get score for this class
				float scoreValue = CClassInterface::getTF2Score(m_pEdict);

				if (m_iClass == TF_CLASS_ENGINEER)
				{
					if (m_pSentryGun.get())
					{
						scoreValue *= 2.0f * CTeamFortress2Mod::getSentryLevel(m_pSentryGun);
						scoreValue *= ((m_fLastSentryEnemyTime + 15.0f) < engine->Time()) ? 2.0f : 1.0f;
					}
					if (m_pTeleEntrance.get() && m_pTeleExit.get())
						scoreValue *= CTeamFortress2Mod::isAttackDefendMap() ? 2.0f : 1.5f;
					if (m_pDispenser.get())
						scoreValue *= 1.25f;
					// less chance of changing class if bot has these up
					m_fChangeClassTime =
					    engine->Time() + randomFloat(bot_min_cc_time.GetFloat() / 2, bot_max_cc_time.GetFloat() / 2);
				}

				// Change class if either I think I could do better
				if (randomFloat(0.0f, 1.0f) > (scoreValue / CTeamFortress2Mod::getHighestScore()))
				{
					chooseClass(); // edits m_iDesiredClass

					// change class
					selectClass();
				}
			}
		}
	}

	m_fIdealMoveSpeed = CTeamFortress2Mod::TF2_GetPlayerSpeed(m_pEdict, m_iClass) * rcbot_speed_boost.GetFloat();

	/* spy check code */
	if (((m_iClass != TF_CLASS_SPY) || (!isDisguised()))
	    && ((m_pEnemy.get() == nullptr) || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)) && (m_pPrevSpy.get() != nullptr)
	    && (m_fSeeSpyTime > engine->Time()) && !m_bIsCarryingObj && CBotGlobals::isAlivePlayer(m_pPrevSpy)
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(getEdict()))
	{
		if ((m_iClass != TF_CLASS_ENGINEER) || !hasSomeConditions(CONDITION_BUILDING_SAPPED))
		{
			// check for spies within radius of bot / use aim skill as a skill factor
			float fPossibleDistance = (engine->Time() - m_fLastSeeSpyTime) * (m_pProfile->m_fAimSkill * 310.0f)
			                        * (m_fCurrentDanger / MAX_BELIEF);

			// increase distance for pyro, he can use flamethrower !
			if (m_iClass == TF_CLASS_PYRO)
				fPossibleDistance += 200.0f;

			if ((m_vLastSeeSpy - getOrigin()).Length() < fPossibleDistance)
			{
				updateCondition(CONDITION_PARANOID);

				if (m_pNavigator->hasNextPoint() && !m_pSchedules->isCurrentSchedule(SCHED_TF_SPYCHECK))
				{
					CBotSchedule *newSchedule = new CBotSchedule(new CSpyCheckAir());

					newSchedule->setID(SCHED_TF_SPYCHECK);

					m_pSchedules->addFront(newSchedule);
				}
			}
			else
				removeCondition(CONDITION_PARANOID);
		}
	}
	else
		removeCondition(CONDITION_PARANOID);

	if (hasFlag())
		removeCondition(CONDITION_COVERT);

	// Combined single-pass player scan for fire help + medic seeking
	{
		bool bOnFire      = CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict) && m_iClass != TF_CLASS_PYRO;
		bool bSeekHeals   = (bNeedHealth && !m_bIsBeingHealed && m_iClass != TF_CLASS_MEDIC
		                   && (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) || !wantToShoot())
		                   && (!m_pHealthkit || distanceFrom(m_pHealthkit) > 512.0f)
		                   && (!m_pNearestDisp || distanceFrom(m_pNearestDisp) > 512.0f));

		if (bOnFire || bSeekHeals)
		{
			edict_t *pBestSavior   = nullptr;
			float fBestSaviorDist  = bOnFire ? 1536.0f : 2048.0f;
			bool bSaviorIsPyro     = false;

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				edict_t *pT = INDEXENT(i);
				if (!pT || pT == m_pEdict) continue;
				if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
				if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;

				TF_Class iClass = (TF_Class)CClassInterface::getTF2Class(pT);
				bool bIsPyro    = (iClass == TF_CLASS_PYRO);
				bool bIsMedic   = (iClass == TF_CLASS_MEDIC);

				if (bOnFire ? (!bIsPyro && !bIsMedic) : !bIsMedic) continue;

				if (isPlayerAFK(pT)) continue;

				if (bIsPyro)
				{
					CBot *pOther = CBots::getBotPointer(pT);
					if (!pOther) continue;
					CBotTF2 *pOtherTF = (CBotTF2 *)pOther;
					CBotWeapon *pFlame = pOtherTF->getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
					if (!pFlame || !pFlame->hasWeapon()) continue;
					edict_t *pFEnt = pFlame->getWeaponEntity();
					int iItem      = pFEnt ? CClassInterface::TF2_getItemDefinitionIndex(pFEnt) : 0;
					if (iItem == 594) continue;
					int iNeedAmmo  = (iItem == 40) ? 50 : (iItem == 215) ? 25
					               : (iItem == 1178 || iItem == 1099) ? 5 : 20;
					if (pFlame->getAmmo(pOther) < iNeedAmmo) continue;
				}

				float fDist = distanceFrom(pT);
				float fWeightedDist = (bOnFire && bIsPyro) ? fDist / 1.5f : fDist;
				if (fWeightedDist < fBestSaviorDist && FVisible(pT))
				{
					fBestSaviorDist  = fWeightedDist;
					pBestSavior      = pT;
					bSaviorIsPyro    = bIsPyro;
				}
			}

			if (pBestSavior)
			{
				notePlayerEngaged(pBestSavior);
				Vector vOrigin = CBotGlobals::entityOrigin(pBestSavior);
				float fCloseRange = bOnFire ? 200.0f : 450.0f;
				float fRealDist   = bOnFire ? (fBestSaviorDist * (bSaviorIsPyro ? 1.5f : 1.0f)) : fBestSaviorDist;

				if (fRealDist < fCloseRange)
				{
					setMoveLookPriority(MOVELOOK_ATTACK);
					stopMoving();
					setLookVector(vOrigin);
					setLookAtTask(LOOK_VECTOR);
					setMoveLookPriority(MOVELOOK_MODTHINK);

					if (m_fCallMedic < engine->Time() && getHealthPercent() < 0.7f)
					{
						callMedic();
						m_fCallMedic = engine->Time() + randomFloat(3.0f, 5.0f);
					}
				}
				else
				{
					setMoveLookPriority(MOVELOOK_ATTACK);
					setMoveTo(vOrigin);
					setLookVector(vOrigin);
					setLookAtTask(LOOK_VECTOR);
					setMoveLookPriority(MOVELOOK_MODTHINK);
				}
			}
		}
	}

	// MvM: grab nearby cash even while fighting
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && !hasFlag()
	    && m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		edict_t *pNearbyCash = CClassInterface::FindEntityByClassnameNearest(
		    getOrigin(), "item_currencypack_custom", 120.0f);
		if (pNearbyCash && CBotGlobals::entityIsAlive(pNearbyCash))
		{
			setMoveLookPriority(MOVELOOK_ATTACK);
			setMoveTo(CBotGlobals::entityOrigin(pNearbyCash));
			setMoveLookPriority(MOVELOOK_MODTHINK);
		}
	}

	switch (m_iClass)
	{
	case TF_CLASS_SCOUT:
		if (m_pWeapons->hasWeapon(TF2_WEAPON_LUNCHBOX_DRINK))
		{
			int pcond = CClassInterface::getTF2Conditions(m_pEdict);

			if ((pcond & TF2_PLAYER_BONKED) == TF2_PLAYER_BONKED)
			{
				wantToShoot(false);

				if (m_fBonkStartTime == 0.0f)
					m_fBonkStartTime = engine->Time();

				float fBonkElapsed = engine->Time() - m_fBonkStartTime;

				// Run into sentries to waste their ammo and distract them
				edict_t *pTargetSentry = m_pNearestEnemySentry.get();
				if (!pTargetSentry)
					pTargetSentry = m_pLastEnemySentry.get();

				if (pTargetSentry && fBonkElapsed < 6.5f)
				{
					// Run toward the sentry to distract it
					m_pEnemy    = pTargetSentry;
					m_pOldEnemy = pTargetSentry;
					addVoiceCommand(TF_VC_GOGOGO); // signal team to push!
				}
				else
				{
					// Bonks almost over or no sentry -- escape
					m_pEnemy    = nullptr;
					m_pOldEnemy = nullptr;
				}

				if (fBonkElapsed > 6.0f && pTargetSentry)
				{
					// Escape sentry before bonk expires
					Vector vSentryPos = CBotGlobals::entityOrigin(pTargetSentry);
					Vector vAway      = getOrigin() - vSentryPos;
					vAway.z           = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(getOrigin() + (vAway * TF2_MAX_SENTRYGUN_RANGE));
					}
				}
			}
			else
			{
				m_fBonkStartTime = 0.0f;
			}

			if (!hasEnemy() && !hasFlag() && (CClassInterface::TF2_getEnergyDrinkMeter(m_pEdict) > 99.99f))
			{
				if (m_fCurrentDanger > 49.0f)
				{
					if (m_fUseBuffItemTime < engine->Time())
					{
						m_fUseBuffItemTime = engine->Time() + 20.0f;
						m_pSchedules->addFront(new CBotSchedule(new CBotUseLunchBoxDrink()));
					}
				}
			}
		}

	case TF_CLASS_SNIPER:
		if (CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict))
		{
			CBotWeapon *pWp = getCurrentWeapon();
			if (pWp && pWp->isProjectile())
			{
				// Bow: never scoped, always normal FOV
			}
			else
			{
				m_fFov = 20.0f;

				if (moveToIsValid() && !hasEnemy())
					secondaryAttack();
			}
		}
		else
			m_fFov = BOT_DEFAULT_FOV;

		break;
	case TF_CLASS_SOLDIER:
		if (m_pWeapons->hasWeapon(TF2_WEAPON_BUFF_ITEM))
		{
			if (CClassInterface::getRageMeter(m_pEdict) > 99.99f)
			{
				if (m_fCurrentDanger > 99.0f)
				{
					if (m_fUseBuffItemTime < engine->Time())
					{
						m_fUseBuffItemTime = engine->Time() + 30.0f;
						m_pSchedules->addFront(new CBotSchedule(new CBotUseBuffItem()));
					}
				}
			}
		}
		break;
	case TF_CLASS_DEMOMAN:
		if (m_iTrapType != TF_TRAP_TYPE_NONE)
		{
			if (m_pEnemy)
			{
				if ((CBotGlobals::entityOrigin(m_pEnemy) - m_vStickyLocation).Length() < BLAST_RADIUS)
					detonateStickies();
			}
		}
		break;
	case TF_CLASS_MEDIC:
		// Heal teammates even while carrying the flag -- but only until they reach max health
		if (m_pHeal && CBotGlobals::entityIsAlive(m_pHeal))
		{
			bool bHealTargetNeedsHP = false;
			if (hasFlag())
			{
				IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(m_pHeal);
				if (pInfo && pInfo->GetHealth() < pInfo->GetMaxHealth())
					bHealTargetNeedsHP = true;
			}
			else
				bHealTargetNeedsHP = true;

			if (bHealTargetNeedsHP)
			{
				if (!m_pSchedules->hasSchedule(SCHED_HEAL))
				{
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotTF2HealSched(m_pHeal));
				}

				wantToShoot(false);
			}
		}

		tryCrossbowHeal();

		break;
	case TF_CLASS_HWGUY:
	{
		bool bRevMiniGun;

		bRevMiniGun = false;

		// hwguys dont rev minigun if they have the flag
		if (wantToShoot() && !m_bHasFlag)
		{
			CBotWeapon *pWeapon = getCurrentWeapon();

			if (pWeapon && (pWeapon->getID() == TF2_WEAPON_MINIGUN))
			{
				if (!CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict)
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
	    && !CTeamFortress2Mod::TF2_IsPlayerCritBoosted(m_pEdict))
				{
					if (m_fCurrentDanger >= TF2_HWGUY_REV_BELIEF)
					{
						if (pWeapon->getAmmo(this) > 100)
							bRevMiniGun = true;
					}
				}
			}
		}

		// Rev the minigun
		if (bRevMiniGun)
		{
			// record time when bot started revving up
			if (m_fRevMiniGunTime == 0)
			{
				float fMinTime        = (m_fCurrentDanger / 200) * 10;

				m_fRevMiniGunTime     = engine->Time();
				m_fNextRevMiniGunTime = randomFloat(fMinTime, fMinTime + 5.0f);
			}

			// rev for 10 seconds
			if ((m_fRevMiniGunTime + m_fNextRevMiniGunTime) > engine->Time())
			{
				secondaryAttack(true);
				// m_fIdealMoveSpeed = 30.0f; Improve Max Speed here

				if (m_fCurrentDanger < 1)
				{
					m_fRevMiniGunTime     = 0.0f;
					m_fNextRevMiniGunTime = 0.0f;
				}
			}
			else if ((m_fRevMiniGunTime + (2.0f * m_fNextRevMiniGunTime)) < engine->Time())
			{
				m_fRevMiniGunTime = 0.0;
			}
		}

		if (m_pButtons->holdingButton(IN_ATTACK) || m_pButtons->holdingButton(IN_ATTACK2))
		{
			if (m_pButtons->holdingButton(IN_JUMP))
				m_pButtons->letGo(IN_JUMP);
		}
	}

	break;
	case TF_CLASS_ENGINEER:

		checkBuildingsValid(false);

		// MvM: stop defending and go build a sentry if missing one
		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
		    && !m_pSentryGun && !m_bIsCarryingObj && !m_pEnemy)
		{
			CBotWeapon *pWrench = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));
			if (pWrench && pWrench->getAmmo(this) >= 130
			    && m_pSchedules->hasSchedule(SCHED_DEFENDPOINT))
			{
				m_pSchedules->freeMemory();
				updateCondition(CONDITION_CHANGED);
			}
		}

		if (!m_pSchedules->hasSchedule(SCHED_REMOVESAPPER))
		{
			// Own buildings -- highest priority, no range limit if not in combat
			bool bInCombat = (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());
			float fMaxDist = bInCombat ? 512.0f : 2048.0f;

			if ((m_fRemoveSapTime < engine->Time()) && m_pSentryGun
			    && CBotGlobals::entityIsValid(m_pSentryGun)
			    && CTeamFortress2Mod::isSentrySapped(m_pSentryGun)
			    && distanceFrom(m_pSentryGun) < fMaxDist)
			{
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotRemoveSapperSched(m_pSentryGun, ENGI_SENTRY));
				updateCondition(CONDITION_PARANOID);
			}
			else if ((m_fRemoveSapTime < engine->Time()) && m_pDispenser
			         && CBotGlobals::entityIsValid(m_pDispenser)
			         && CTeamFortress2Mod::isDispenserSapped(m_pDispenser)
			         && distanceFrom(m_pDispenser) < fMaxDist)
			{
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotRemoveSapperSched(m_pDispenser, ENGI_DISP));
				updateCondition(CONDITION_PARANOID);
			}
			else if ((m_fRemoveSapTime < engine->Time()) && m_pTeleExit
			         && CBotGlobals::entityIsValid(m_pTeleExit)
			         && CTeamFortress2Mod::isTeleporterSapped(m_pTeleExit)
			         && distanceFrom(m_pTeleExit) < fMaxDist)
			{
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotRemoveSapperSched(m_pTeleExit, ENGI_EXIT));
				updateCondition(CONDITION_PARANOID);
			}
			// Ally buildings -- helpful but lower priority
			else if ((m_fRemoveSapTime < engine->Time()) && m_pNearestAllySentry
			    && CBotGlobals::entityIsValid(m_pNearestAllySentry)
			    && CTeamFortress2Mod::isSentrySapped(m_pNearestAllySentry))
			{
				m_pSchedules->freeMemory();
				m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestAllySentry, ENGI_SENTRY));
				updateCondition(CONDITION_PARANOID);
			}
		}

		// Thank nearby engineers who are helping with our buildings
		if (m_fThanksTime < engine->Time())
		{
			auto thankHelper = [&](edict_t *pBuilding, float &fPrevHp, float fHp, float &fPrevShells,
			                       float fShells, float &fPrevRockets, float fRockets) {
				bool bImproved = (fHp > fPrevHp || fShells > fPrevShells || fRockets > fPrevRockets);
				if (bImproved)
				{
					for (int i = 1; i <= gpGlobals->maxClients; i++)
					{
						edict_t *pT = INDEXENT(i);
						if (!pT || pT == m_pEdict) continue;
						if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
						if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
						if (CClassInterface::getTF2Class(pT) != TF_CLASS_ENGINEER) continue;
						if (distanceFrom(pT) < 256.0f && isVisible(pT))
						{
							setLookAt(CBotGlobals::entityOrigin(pT));
							m_fLookSetTime = engine->Time() + 1.0f;
							addVoiceCommand(TF_VC_THANKS);
							m_fThanksTime = engine->Time() + randomFloat(20.0f, 40.0f);
							break;
						}
					}
				}
				fPrevHp      = fHp;
				fPrevShells  = fShells;
				fPrevRockets = fRockets;
			};

			if (m_pSentryGun.get())
			{
				edict_t *pS = m_pSentryGun.get();
				thankHelper(pS, m_prevSentryHealth, CClassInterface::getSentryHealth(pS),
				            m_prevSentryShells, (float)CClassInterface::getTF2SentryShells(pS),
				            m_prevSentryRockets, (float)CClassInterface::getTF2SentryRockets(pS));
			}
		}

		break;
	case TF_CLASS_SPY:
		if (!hasFlag())
		{
			if (rcbot_tf2_debug_spies_cloakdisguise.GetBool() && (m_fSpyDisguiseTime < engine->Time()))
			{
				// if previously detected or isn't disguised
				if ((m_fDisguiseTime == 0.0f) || !isDisguised())
				{
					int iteam = CTeamFortress2Mod::getEnemyTeam(getTeam());

					spyDisguise(iteam, getSpyDisguiseClass(iteam));
				}

				m_fSpyDisguiseTime = engine->Time() + 5.0f;
			}

			bIsCloaked = CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict);

			// When disguised or cloaked, avoid bumping into enemy bots
			if ((isDisguised() || bIsCloaked) && !hasEnemy())
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					edict_t *pT = INDEXENT(i);
					if (!pT || pT == m_pEdict) continue;
					if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
					if (CTeamFortress2Mod::getTeam(pT) == m_iTeam) continue;

					float fDist = distanceFrom(pT);
					float fAvoidDist = 80.0f;
					if (pT->GetCollideable() && pT->GetCollideable()->OBBMaxs().Length() > 80.0f)
						fAvoidDist = 200.0f; // giant enemies have larger hitboxes
					if (fDist < fAvoidDist)
					{
						// Sidestep away: move perpendicular to the direction to them
						Vector vTo = getOrigin() - CBotGlobals::entityOrigin(pT);
						vTo.z      = 0;
						if (vTo.Length() > 0.1f)
						{
							vTo = vTo / vTo.Length();
							Vector vPerp = vTo.Cross(Vector(0, 0, 1));
							if (vPerp.Length() > 0.1f)
							{
								vPerp = vPerp / vPerp.Length();
								if (randomInt(0, 1))
									vPerp = -vPerp;
								setMoveTo(getOrigin() + vPerp * 160.0f);
								updateCondition(CONDITION_COVERT);
							}
						}
						break;
					}
				}
			}

			// Let sap schedule manage its own cloak/uncloak to avoid oscillation
			if (!m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING))
			{
				if (bIsCloaked && wantToUnCloak())
					spyUnCloak();
				else if (!bIsCloaked && wantToCloak())
					spyCloak();
				else if (bIsCloaked || isDisguised() && !hasEnemy())
					updateCondition(CONDITION_COVERT);
			}

			// Split sap tasks between multiple spies: each spy prefers a different
			// building type based on its entindex to avoid overlapping
			int iEntIdx   = ENTINDEX(m_pEdict);
			int iSapOrder = iEntIdx % 3;
			static const int sapTypes[3][3] = {
				{ 1, 2, 3 }, // SENTRY, TELE, DISP priority
				{ 2, 3, 1 }, // TELE, DISP, SENTRY
				{ 3, 1, 2 	}
			};

			for (int iOrder = 0; iOrder < 3; iOrder++)
			{
				int iType = sapTypes[iSapOrder][iOrder];
				if (iType == 1 && m_pNearestEnemySentry && (m_fSpySapTime < engine->Time())
				    && !CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry)
				    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING))
				{
					m_fSpySapTime = engine->Time() + randomFloat(0.5f, 1.5f);
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemySentry, ENGI_SENTRY));
					break;
				}
				if (iType == 2 && m_pNearestEnemyTeleporter && (m_fSpySapTime < engine->Time())
				    && !CTeamFortress2Mod::isTeleporterSapped(m_pNearestEnemyTeleporter)
				    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING))
				{
					m_fSpySapTime = engine->Time() + randomFloat(0.5f, 1.5f);
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyTeleporter, ENGI_TELE));
					break;
				}
				if (iType == 3 && m_pNearestEnemyDisp && (m_fSpySapTime < engine->Time())
				    && !CTeamFortress2Mod::isDispenserSapped(m_pNearestEnemyDisp)
				    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING))
				{
					m_fSpySapTime = engine->Time() + randomFloat(0.5f, 1.5f);
					m_pSchedules->freeMemory();
					m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyDisp, ENGI_DISP));
					break;
				}
			}
		}

		// Medic-bait: lure enemy medics by calling medic when actually injured
		if (!m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING)
		    && !m_pSchedules->isCurrentSchedule(SCHED_BACKSTAB)
		    && isDisguised() && !bIsCloaked && (m_fBaitCallTime < engine->Time())
		    && bNeedHealth)
		{
			edict_t *pBaitTarget = nullptr;
			float fBaitDist      = 1024.0f;

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				edict_t *pT = INDEXENT(i);
				if (!pT || pT == m_pEdict) continue;
				if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
				if (CTeamFortress2Mod::getTeam(pT) == m_iTeam) continue;
				if (CClassInterface::getTF2Class(pT) != TF_CLASS_MEDIC) continue;
				if (thinkSpyIsEnemy(pT, (TF_Class)CClassInterface::getTF2Class(pT))) continue;

				float fDist = distanceFrom(pT);
				if (fDist < fBaitDist && FVisible(pT))
				{
					fBaitDist    = fDist;
					pBaitTarget  = pT;
				}
			}

			if (pBaitTarget)
			{
				if (fBaitDist < 200.0f)
				{
					CBotWeapon *pKnife = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_KNIFE));
					if (pKnife && pKnife->hasWeapon())
						select_CWeapon(pKnife->getWeaponInfo());
				}
				else
				{
					addVoiceCommand(TF_VC_MEDIC);
					m_fBaitCallTime = engine->Time() + randomFloat(2.0f, 4.0f);

					int iDClass, iDTeam, iDIndex, iDHealth;
					CClassInterface::getTF2SpyDisguised(m_pEdict, &iDClass, &iDTeam, &iDIndex, &iDHealth);
					if (iDClass == TF_CLASS_MEDIC)
						spyDisguise(iDTeam, TF_CLASS_MEDIC);
				}
			}
		}

		// --- Spy context detection and blend-in ---
		{
			int iNewContext = 0;
			if (!CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict) && isDisguised())
			{
				int iNearest = CWaypointLocations::NearestWaypoint(getOrigin(), 256.0f, -1);
				if (iNearest >= 0)
				{
					CWaypoint *pNearest = CWaypoints::getWaypoint(iNearest);
					if (pNearest && pNearest->getArea() == 0)
						iNewContext = 1;
				}
				if (iNewContext == 0 && m_pNearestEnemySentry.get()
				    && distanceFrom(m_pNearestEnemySentry) < 400.0f)
					iNewContext = 2;
				if (iNewContext == 0)
				{
					Vector vObj;
					if (CTeamFortress2Mod::getFlagLocation(m_iTeam, &vObj)
					    || CTeamFortress2Mod::getMVMCapturePoint(&vObj))
					{
						if ((getOrigin() - vObj).Length() < 800.0f)
							iNewContext = 3;
					}
				}
			}

			if (iNewContext != 0 && iNewContext != m_iSpyContext
			    && m_fSpyRedisguiseTime < engine->Time()
			    && !CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict) && isDisguised()
			    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING)
			    && !m_pSchedules->isCurrentSchedule(SCHED_BACKSTAB))
			{
				bool bVisibleToEnemy = false;
				for (int i = 1; i <= CBotGlobals::maxClients(); i++)
				{
					edict_t *pEd = INDEXENT(i);
					if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
					if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
					if (distanceFrom(pEd) < 800.0f && isVisible(pEd))
					{
						bVisibleToEnemy = true;
						break;
					}
				}
				if (!bVisibleToEnemy)
				{
					int iTeam = CTeamFortress2Mod::getEnemyTeam(getTeam());
					int iDisp = getSpyDisguiseClass(iTeam);
					if (iNewContext == 1)
					{
						static int bc[] = { TF_CLASS_ENGINEER, TF_CLASS_SNIPER, TF_CLASS_MEDIC, TF_CLASS_SCOUT };
						iDisp = bc[randomInt(0, 3)];
					}
					else if (iNewContext == 2)
					{
						static int nc[] = { TF_CLASS_ENGINEER, TF_CLASS_ENGINEER, TF_CLASS_PYRO, TF_CLASS_HWGUY };
						iDisp = nc[randomInt(0, 3)];
					}
					else if (iNewContext == 3)
					{
						static int fc[] = { TF_CLASS_SOLDIER, TF_CLASS_PYRO, TF_CLASS_DEMOMAN, TF_CLASS_HWGUY };
						iDisp = fc[randomInt(0, 3)];
					}
					spyDisguise(iTeam, iDisp);
				}
				m_fSpyRedisguiseTime = engine->Time() + randomFloat(15.0f, 25.0f);
			}
			m_iSpyContext = iNewContext;

			// Blend-in: look natural when disguised and close to enemies
			if (isDisguised() && !CTeamFortress2Mod::TF2_IsPlayerCloaked(m_pEdict)
			    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING)
			    && !m_pSchedules->isCurrentSchedule(SCHED_BACKSTAB)
			    && !m_pEnemy)
			{
				int iDClass, iDTeam, iDIndex, iDHealth;
				CClassInterface::getTF2SpyDisguised(m_pEdict, &iDClass, &iDTeam, &iDIndex, &iDHealth);
				for (int i = 1; i <= CBotGlobals::maxClients(); i++)
				{
					edict_t *pEd = INDEXENT(i);
					if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
					if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
					if (distanceFrom(pEd) < 400.0f && isVisible(pEd))
					{
						setLookAtTask(LOOK_AROUND);
						break;
					}
				}
				switch (iDClass)
				{
				case TF_CLASS_ENGINEER:
					if (m_pNearestEnemySentry.get() && distanceFrom(m_pNearestEnemySentry) > 150.0f
					    && distanceFrom(m_pNearestEnemySentry) < 600.0f)
						setMoveTo(CBotGlobals::entityOrigin(m_pNearestEnemySentry));
					break;
				case TF_CLASS_MEDIC:
					for (int i = 1; i <= CBotGlobals::maxClients(); i++)
					{
						edict_t *pEd = INDEXENT(i);
						if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
						if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
						float fD = distanceFrom(pEd);
						if (fD < 300.0f && fD > 50.0f)
							setMoveTo(CBotGlobals::entityOrigin(pEd));
					}
					break;
				case TF_CLASS_SNIPER:
				{
					int iWpt = CWaypointLocations::NearestWaypoint(getOrigin(), 512.0f, -1);
					if (iWpt >= 0)
					{
						CWaypoint *pWpt = CWaypoints::getWaypoint(iWpt);
						if (pWpt)
						{
							for (int j = 0; j < pWpt->numPaths(); j++)
							{
								int n = pWpt->getPath(j);
								CWaypoint *pn = CWaypoints::getWaypoint(n);
								if (pn && pn->getFlags() & CWaypointTypes::W_FL_SNIPER)
								{
									setMoveTo(pn->getOrigin());
									break;
								}
							}
						}
					}
				}
				break;
				default:
					break;
				}
				for (int i = 1; i <= CBotGlobals::maxClients(); i++)
				{
					edict_t *pEd = INDEXENT(i);
					if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
					if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
					if (distanceFrom(pEd) < 200.0f)
					{
						Vector vAway = getOrigin() - CBotGlobals::entityOrigin(pEd);
						vAway.z = 0;
						if (vAway.Length() > 0.1f)
						{
							vAway = vAway / vAway.Length();
							Vector vPerp = vAway.Cross(Vector(0, 0, 1));
							setMoveTo(getOrigin() + vPerp * 160.0f);
						}
						break;
					}
				}
			}
		}

		break;
	case TF_CLASS_PYRO:

		// Extinguish burning teammates when not in active combat
		if (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) || !wantToShoot())
		{
			tryExtinguishTeammates();
			tryIgniteSniperBow();
		}
		break;
	default:
		break;
	}

	// Handle special weapon abilities (charge shots, auto-taunt, etc.)
	handleSpecialAbilities();

	// Prevent weapon switching while Thermal Thruster state machine is pending (travel only)
	bool bInDanger = (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && wantToShoot());
	if (m_bThrusterSwitchPending && !bInDanger)
		wantToChangeWeapon(false);

	// look for tasks / more important tasks here

	if (!hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && !m_bLookedForEnemyLast && m_pLastEnemy
	    && CBotGlobals::entityIsValid(m_pLastEnemy) && CBotGlobals::entityIsAlive(m_pLastEnemy))
	{
		if (wantToFollowEnemy())
		{
			m_pSchedules->freeMemory();
			m_pSchedules->addFront(new CBotFollowLastEnemy(this, m_pLastEnemy, m_vLastSeeEnemy));
			m_bLookedForEnemyLast = true;
		}
	}

	if (m_fTaunting > engine->Time())
	{
		m_pButtons->letGoAllButtons(true);
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		stopMoving();
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}
	else if (m_fDoubleJumpTime && (m_fDoubleJumpTime < engine->Time()))
	{
		tapButton(IN_JUMP);
		m_fDoubleJumpTime = 0;
	}

	if (m_pSchedules->isCurrentSchedule(SCHED_GOTO_ORIGIN) && (m_fPickupTime < engine->Time())
	    && (bNeedHealth || bNeedAmmo) && (!m_pEnemy && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)))
	{
		if ((m_fPickupTime < engine->Time()) && m_pNearestDisp && !m_pSchedules->isCurrentSchedule(SCHED_USE_DISPENSER))
		{
			if (fabs(CBotGlobals::entityOrigin(m_pNearestDisp).z - getOrigin().z) < BOT_JUMP_HEIGHT)
			{
				m_pSchedules->removeSchedule(SCHED_USE_DISPENSER);
				m_pSchedules->addFront(new CBotUseDispSched(this, m_pNearestDisp));

				m_fPickupTime = engine->Time() + randomFloat(6.0f, 20.0f);
				return;
			}
		}
		else if ((m_fPickupTime < engine->Time()) && bNeedHealth && m_pHealthkit
		         && !m_pSchedules->isCurrentSchedule(SCHED_TF2_GET_HEALTH))
		{
			if (fabs(CBotGlobals::entityOrigin(m_pHealthkit).z - getOrigin().z) < BOT_JUMP_HEIGHT)
			{
				m_pSchedules->removeSchedule(SCHED_TF2_GET_HEALTH);
				m_pSchedules->addFront(new CBotTF2GetHealthSched(CBotGlobals::entityOrigin(m_pHealthkit)));

				m_fPickupTime = engine->Time() + randomFloat(5.0f, 10.0f);

				return;
			}
		}
		else if ((m_fPickupTime < engine->Time()) && bNeedAmmo && m_pAmmo
		         && !m_pSchedules->isCurrentSchedule(SCHED_PICKUP))
		{
			if (fabs(CBotGlobals::entityOrigin(m_pAmmo).z - getOrigin().z) < BOT_JUMP_HEIGHT)
			{
				m_pSchedules->removeSchedule(SCHED_TF2_GET_AMMO);
				m_pSchedules->addFront(new CBotTF2GetAmmoSched(CBotGlobals::entityOrigin(m_pAmmo)));

				m_fPickupTime = engine->Time() + randomFloat(5.0f, 10.0f);

				return;
			}
		}
	}

	setMoveLookPriority(MOVELOOK_MODTHINK);
}

void CBotTF2::handleWeapons()
{
	if (m_iClass == TF_CLASS_ENGINEER)
	{
		edict_t *pSentry = m_pSentryGun.get();

		if (m_bIsCarryingObj)
		{
			// don't shoot while carrying object unless after 5 seconds of carrying
			if ((getHealthPercent() > 0.9f) || ((m_fCarryTime + 3.0f) > engine->Time()))
				return;
		}
		else if ((pSentry != nullptr) && !hasSomeConditions(CONDITION_PARANOID))
		{
			if (isVisible(pSentry) && (CClassInterface::getSentryEnemy(pSentry) != nullptr))
			{
				if (distanceFrom(pSentry) < 100)
					return; // don't shoot -- i probably want to upgrade sentry
			}
		}
	}

	// Tank combat: focus on tanks, prevent findEnemy() from cycling to random robots
	bool bTankClass = (m_iClass == TF_CLASS_PYRO || m_iClass == TF_CLASS_SOLDIER
	                   || m_iClass == TF_CLASS_DEMOMAN || m_iClass == TF_CLASS_SCOUT
	                   || m_iClass == TF_CLASS_HWGUY);

	if (m_pEnemy && bTankClass
	    && CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(m_pEnemy.get()))
	{
		bool bInDanger = hasSomeConditions(CONDITION_NEED_HEALTH) || hasSomeConditions(CONDITION_NEED_AMMO)
		                 || recentlyHurt(2.0f) || (m_fCurrentDanger > 50.0f);
		if (!bInDanger)
		{
			wantToShoot(true);
			m_bWantToChangeWeapon = false;
			if ((CBotGlobals::entityOrigin(m_pEnemy.get()) - getOrigin()).Length() < 128.0f)
				updateCondition(CONDITION_SEE_CUR_ENEMY);
		}
		else
		{
			m_pEnemy = nullptr;
			wantToShoot(false);
		}
	}

	//
	// Handle attacking at this point
	//
	bool bTankClose = (m_pEnemy && CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	                   && CTeamFortress2Mod::isTankBoss(m_pEnemy.get())
	                   && (CBotGlobals::entityOrigin(m_pEnemy.get()) - getOrigin()).Length() < 128.0f);

	if (m_pEnemy && !hasSomeConditions(CONDITION_ENEMY_DEAD)
	    && (bTankClose || hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	    && wantToShoot() && (bTankClose || isVisible(m_pEnemy)) && isEnemy(m_pEnemy) && m_pWeapons)
	{
		CBotWeapon *pWeapon;

		pWeapon = m_pWeapons->getBestWeapon(m_pEnemy, !hasFlag(), !hasFlag(), rcbot_melee_only.GetBool(), false,
		                                    CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
		                                        || CTeamFortress2Mod::TF2_IsPlayerCritBoosted(m_pEdict));

		setLookAtTask(LOOK_ENEMY);

		m_pAttackingEnemy = nullptr;

		if (m_bWantToChangeWeapon && (pWeapon != nullptr) && (pWeapon != getCurrentWeapon())
		    && pWeapon->getWeaponIndex())
		{
			select_CWeapon(pWeapon->getWeaponInfo());
			// selectWeapon(pWeapon->getWeaponIndex());
		}
		else if (!handleAttack(pWeapon, m_pEnemy))
		{
			m_pEnemy    = nullptr;
			m_pOldEnemy = nullptr;
			wantToShoot(false);
		}
	}
}

void CBotTF2::enemyFound(edict_t *pEnemy)
{
	CBotFortress::enemyFound(pEnemy);
	m_fRevMiniGunTime = 0.0f;

	if (m_pNearestEnemySentry == pEnemy)
	{
		CBotWeapon *pWeapon = m_pWeapons->getPrimaryWeapon();

		if ((pWeapon != nullptr) && (m_iClass != TF_CLASS_SPY) && !pWeapon->outOfAmmo(this)
		    && pWeapon->primaryGreaterThanRange(TF2_MAX_SENTRYGUN_RANGE + 32.0f))
		{
			updateCondition(CONDITION_CHANGED);
		}
	}
}

bool CBotFortress::canAvoid(edict_t *pEntity)
{
	return CBot::canAvoid(pEntity);
}

bool CBotTF2::canAvoid(edict_t *pEntity)
{
	float distance;
	Vector vAvoidOrigin;
	int index;

	if (!CBotGlobals::entityIsValid(pEntity))
		return false;
	if (m_pLookEdict.get() == pEntity)
		return false;
	if (m_pEdict == pEntity) // can't avoid self!!!!
		return false;
	if (m_pLastEnemy.get() == pEntity)
		return false;
	if (m_pTeleEntrance.get() == pEntity)
		return false;
	if (m_pNearestTeleEntrance.get() == pEntity)
		return false;
	if (m_pNearestDisp.get() == pEntity)
		return false;
	if (pEntity == m_pHealthkit.get())
		return false;
	if (pEntity == m_pAmmo.get())
		return false;
	if ((m_pSentryGun.get() == pEntity) && (CClassInterface::isObjectCarried(pEntity)))
		return false;
	if ((m_pDispenser.get() == pEntity) && (CClassInterface::isObjectCarried(pEntity)))
		return false;
	if ((m_pTeleExit.get() == pEntity) && (CClassInterface::isObjectCarried(pEntity)))
		return false;

	edict_t *groundEntity = CClassInterface::getGroundEntity(m_pEdict);

	// must stand on worldspawn
	if (groundEntity && (ENTINDEX(groundEntity) > 0) && (pEntity == groundEntity))
	{
		if (m_pSentryGun.get() == pEntity)
			return true;
		if (m_pDispenser.get() == pEntity)
			return true;
	}

	index = ENTINDEX(pEntity);

	if (!index)
		return false;

	vAvoidOrigin = CBotGlobals::entityOrigin(pEntity);

	if (vAvoidOrigin == m_vMoveTo)
		return false;

	distance = distanceFrom(vAvoidOrigin);

	if ((distance > 1) && (distance < bot_avoid_radius.GetFloat()) && (vAvoidOrigin.z >= getOrigin().z)
	    && (fabs(getOrigin().z - vAvoidOrigin.z) < 64))
	{
		if ((m_pAttackingEnemy.get() != nullptr) && (m_pAttackingEnemy.get() == pEntity))
			return false; // I need to melee this guy probably
		else if (isEnemy(pEntity, false))
			return true;
		else if ((m_iClass == TF_CLASS_ENGINEER)
		         && ((pEntity == m_pSentryGun.get()) || (pEntity == m_pDispenser.get())))
			return true;
	}

	return false;
}

bool CBotTF2::wantToInvestigateSound()
{
	if (!CBot::wantToInvestigateSound())
		return false;
	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
		return false;

	return (m_fLastSeeEnemy + 8.0f < engine->Time()) && !m_bHasFlag
	    && ((m_iClass != TF_CLASS_ENGINEER)
	        || (!this->m_bIsCarryingObj && (m_pSentryGun.get() != nullptr)
	            && ((CTeamFortress2Mod::getSentryLevel(m_pSentryGun) > 2)
	                && (CClassInterface::getSentryHealth(m_pSentryGun) > 90))));
}

bool CBotTF2::wantToListenToPlayerFootsteps(edict_t *pPlayer)
{
	if (rcbot_notarget.GetBool() && (CClients::isListenServerClient(CClients::get(pPlayer))))
		return false;

	switch (CClassInterface::getTF2Class(pPlayer))
	{
	case TF_CLASS_MEDIC:
	{
		if (m_pHealer.get() == pPlayer)
			return false;
	}
	break;
	case TF_CLASS_SPY:
	{
		// don't listen to cloaked spies
		if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer))
			return false;
	}
	break;

	default:
		break;
	}

	return true;
}

bool CBotTF2::wantToListenToPlayerAttack(edict_t *pPlayer, int iWeaponID)
{
	static edict_t *pWeapon;
	static const char *szWeaponClassname;

	pWeapon = CClassInterface::getCurrentWeapon(pPlayer);

	if (!pWeapon)
		return true;

	szWeaponClassname = pWeapon->GetClassName();

	switch (CClassInterface::getTF2Class(pPlayer))
	{
	case TF_CLASS_MEDIC:
	{
		// don't listen to mediguns
		if (!strcmp("medigun", &szWeaponClassname[10]))
			return false;
	}
	break;
	case TF_CLASS_ENGINEER:
	{
		// don't listen to engis upgrading stuff
		if (!strcmp("wrench", &szWeaponClassname[10]))
			return false;
		else if (!strcmp("builder", &szWeaponClassname[10]))
			return false;
	}
	break;
	case TF_CLASS_SPY:
	{
		// don't listen to cloaked spies
		if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer))
			return false;
		if (!strcmp("knife", &szWeaponClassname[10]))
		{
			// only hear spy knives if they know there are spies around
			// in 15% of cases
			return hasSomeConditions(CONDITION_PARANOID) && (randomFloat(0.0f, 1.0f) <= 0.15f);
		}
	}
	break;

	default:
		break;
	}

	return true;
}

void CBotTF2::checkStuckonSpy(void)
{
	edict_t *pPlayer;
	edict_t *pStuck = nullptr;

	int i           = 0;
	int iTeam       = getTeam();

	float fDistance;
	float fMaxDistance = 80;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		pPlayer = INDEXENT(i);

		if (pPlayer == m_pEdict)
			continue;

		if (CBotGlobals::entityIsValid(pPlayer) && CBotGlobals::entityIsAlive(pPlayer)
		    && (CTeamFortress2Mod::getTeam(pPlayer) != iTeam))
		{
			if ((fDistance = distanceFrom(pPlayer)) < fMaxDistance) // touching distance
			{
				if (isVisible(pPlayer))
				{
					fMaxDistance = fDistance;
					pStuck       = pPlayer;
				}
			}
		}
	}

	if (pStuck)
	{
		// Only recognize the spy after ~1 second of being stuck on them
		// (0.5s initial stuck detection + 0.5s extra)
		if (CClassInterface::getTF2Class(pStuck) == TF_CLASS_SPY)
		{
			if (m_fStuckSpyTime == 0.0f)
				m_fStuckSpyTime = engine->Time();
			else if ((m_fStuckSpyTime + 0.5f) < engine->Time())
			{
				foundSpy(pStuck, CTeamFortress2Mod::getSpyDisguise(pStuck));
				m_pEnemy       = pStuck;
				m_fStuckSpyTime = 0.0f;
			}
		}
		else
			m_fStuckSpyTime = 0.0f;

		if ((m_iClass == TF_CLASS_SPY) && isDisguised())
		{
			// Doh! found me!
			if (randomFloat(0.0f, 100.0f) < getHealthPercent())
				m_fFrenzyTime = engine->Time() + randomFloat(0.0f, getHealthPercent());

			detectedAsSpy(pStuck, false);
			return;
		}
	}
	else
		m_fStuckSpyTime = 0.0f;
}

bool CBotFortress::isClassOnTeam(int iClass, int iTeam)
{
	int i = 0;
	edict_t *pPlayer;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		pPlayer = INDEXENT(i);

		if (CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
		{
			if (CClassInterface::getTF2Class(pPlayer) == iClass)
				return true;
		}
	}

	return false;
}

bool CBotTF2::wantToFollowEnemy()
{
	edict_t *pEnemy = m_pLastEnemy.get();

	if (CTeamFortress2Mod::isLosingTeam(CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
		return true;
	else if ((m_iClass == TF_CLASS_SCOUT)
	         && ((CClassInterface::getTF2Conditions(m_pEdict) & TF2_PLAYER_BONKED) == TF2_PLAYER_BONKED))
		return false;                   // currently can't shoot
	else if (!wantToInvestigateSound()) // maybe capturing point right now
		return false;
	else if ((pEnemy != nullptr) && CBotGlobals::isPlayer(pEnemy) && CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict)
	         && (m_iClass != TF_CLASS_MEDIC))
		return true; // I am ubered  GO!!!
	else if ((pEnemy != nullptr) && CBotGlobals::isPlayer(pEnemy)
	         && CTeamFortress2Mod::TF2_IsPlayerCritBoosted(m_pEdict) && (m_iClass != TF_CLASS_MEDIC))
		return true; // I am crit boosted -- GO!!!
	else if ((pEnemy != nullptr) && CBotGlobals::isPlayer(pEnemy) && CTeamFortress2Mod::TF2_IsPlayerInvuln(pEnemy))
		return false; // Enemy is UBERED  -- don't follow
	else if ((m_iCurrentDefendArea != 0) && (pEnemy != nullptr) && CTeamFortress2Mod::isMapType(TF_MAP_CP)
	         && (CTeamFortress2Mod::m_ObjectiveResource.GetNumControlPoints() > 0))
	{
		Vector vDefend = CTeamFortress2Mod::m_ObjectiveResource.GetCPPosition(
		    CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iCurrentDefendArea]);

		Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
		Vector vOrigin      = getOrigin();

		// He's trying to cap the point? Maybe! Go after him!
		if (((vDefend - vEnemyOrigin).Length() + 80.0f) < (vDefend - vOrigin).Length())
		{
			updateCondition(CONDITION_DEFENSIVE);
			return true;
		}
	}
	else if ((m_fLastKnownTeamFlagTime > 0) && (pEnemy != nullptr)
	         && (CTeamFortress2Mod::isMapType(TF_MAP_CTF) || CTeamFortress2Mod::isMapType(TF_MAP_MVM)))
	{
		Vector vDefend      = m_vLastKnownTeamFlagPoint;

		Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
		Vector vOrigin      = getOrigin();

		// He's trying to get the flag? Maybe! Go after him!
		if (((vDefend - vEnemyOrigin).Length() + 80.0f) < (vDefend - vOrigin).Length())
		{
			updateCondition(CONDITION_DEFENSIVE);
			return true;
		}
	}
	/*else if ( (m_fLastKnownTeamFlagTime == 0) && (m_pLastEnemy.get() != nullptr) &&
	CTeamFortress2Mod::isMapType(TF_MAP_CTF) )
	{
	    Vector vDefend = m_vLastKnownTeamFlagPoint;

	    edict_t *pEnemy = m_pLastEnemy.get();

	    Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
	    Vector vOrigin = getOrigin();

	    // He's trying to get the flag? Maybe! Go after him!
	    if ( ((vDefend-vEnemyOrigin).Length()+80.0f) < (vDefend-vOrigin).Length() )
	    {
	        updateCondition(CONDITION_DEFENSIVE);
	        return true;
	    }
	}*/

	return CBotFortress::wantToFollowEnemy();
}

bool CBotFortress::wantToFollowEnemy()
{
	if (hasSomeConditions(CONDITION_NEED_HEALTH))
		return false;
	if (hasSomeConditions(CONDITION_NEED_AMMO))
		return false;
	if (!CTeamFortress2Mod::hasRoundStarted())
		return false;
	if (!m_pLastEnemy)
		return false;
	if (hasFlag())
		return false;
	if (m_iClass == TF_CLASS_SCOUT)
		return false;
	else if ((m_iClass == TF_CLASS_MEDIC) && m_pHeal)
		return false;
	else if ((m_iClass == TF_CLASS_SPY) && isDisguised()) // sneak around the enemy
		return true;
	else if ((m_iClass == TF_CLASS_SNIPER) && (distanceFrom(m_pLastEnemy) > CWaypointLocations::REACHABLE_RANGE))
		return false; // don't bother!
	else if (CBotGlobals::isPlayer(m_pLastEnemy) && (CClassInterface::getTF2Class(m_pLastEnemy) == TF_CLASS_SPY)
	         && (thinkSpyIsEnemy(m_pLastEnemy, CTeamFortress2Mod::getSpyDisguise(m_pLastEnemy))))
		return true; // always find spies!
	else if (CTeamFortress2Mod::isFlagCarrier(m_pLastEnemy))
		return true; // follow flag carriers to the death
	else if (m_iClass == TF_CLASS_ENGINEER)
		return false; // have work to do

	return CBot::wantToFollowEnemy();
}

void CBotTF2::voiceCommand(int cmd)
{
	char scmd[64];
	u_VOICECMD vcmd;

	vcmd.voicecmd = cmd;

	sprintf(scmd, "voicemenu %d %d", vcmd.b1.v1, vcmd.b1.v2);

	helpers->ClientCommand(m_pEdict, scmd);
}

bool CBotTF2::checkStuck(void)
{
	if (!CTeamFortress2Mod::isAttackDefendMap()
	    || (CTeamFortress2Mod::hasRoundStarted() || (getTeam() == TF2_TEAM_RED)))
	{
		if (CBot::checkStuck())
		{
			checkStuckonSpy();

			return true;
		}
	}

	return false;
}

void CBotTF2::foundSpy(edict_t *pEdict, TF_Class iDisguise)
{
	CBotFortress::foundSpy(pEdict, iDisguise);

	if (m_fLastSaySpy < engine->Time())
	{
		addVoiceCommand(TF_VC_SPY);

		m_fLastSaySpy = engine->Time() + randomFloat(10.0f, 40.0f);
	}
}

int CBotFortress::getSpyDisguiseClass(int iTeam)
{
	int i = 0;
	edict_t *pPlayer;
	std::vector<int> availableClasses;
	int _class;
	float fTotal;
	float fRand;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		pPlayer = INDEXENT(i);

		if (CBotGlobals::entityIsValid(pPlayer) && (CTeamFortress2Mod::getTeam(pPlayer) == iTeam))
		{
			_class = CClassInterface::getTF2Class(pPlayer);

			if (_class)
				availableClasses.push_back(_class);
		}
	}

	// In MvM, robots don't detect spies based on disguise class --
	// just pick a random fast class regardless of what teammates exist
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
	{
		int fastClasses[] = { 1, 2, 5, 7, 8, 9 }; // Scout, Sniper, Medic, Pyro, Spy, Engi
		return fastClasses[randomInt(0, 5)];
	}

	if (availableClasses.empty())
		return randomInt(1, 9);

	fTotal = 0;

	for (int i = 0; i < availableClasses.size(); i++)
		fTotal += m_fClassDisguiseFitness[availableClasses[i]];

	if (fTotal > 0)
	{

		fRand  = randomFloat(0.0, fTotal);

		fTotal = 0;

		for (int i = 0; i < availableClasses.size(); i++)
		{
			fTotal += m_fClassDisguiseFitness[availableClasses[i]];

			if (fRand <= fTotal)
				return availableClasses[i];
		}
	}

	// choose one of the classes proportional to whatever's on the team
	return availableClasses[randomInt(0, availableClasses.size() - 1)];
}

bool CBotFortress::incomingRocket(float fRange)
{
	edict_t *pRocket = m_NearestEnemyRocket;

	if (pRocket != nullptr)
	{
		Vector vel;
		Vector vorg = CBotGlobals::entityOrigin(pRocket);
		Vector vcomp;
		float fDist = distanceFrom(pRocket);

		if (fDist < fRange)
		{
			if (CClassInterface::getVelocity(pRocket, &vel))
			{
				float fSpeed = vel.Length();

				if (fSpeed > 0.1f)
				{
					vel = vel / fSpeed;

					Vector vToProj = vorg - getOrigin();
					float fDot     = vel.Dot(vToProj);
					if (fDot <= 0.0f) // projectile heading away, not a threat
						return false;

					vcomp = vorg + vel * fDist;
				}
				else
					vcomp = vorg;
			}
			else
				vcomp = vorg;

			return (distanceFrom(vcomp) < BLAST_RADIUS);
		}
	}

	pRocket = m_pNearestPipeGren;

	if (pRocket)
	{
		Vector vel;
		Vector vorg = CBotGlobals::entityOrigin(pRocket);
		Vector vcomp;
		float fDist = distanceFrom(pRocket);

		if (fDist < fRange)
		{
			if (CClassInterface::getVelocity(pRocket, &vel))
			{
				float fSpeed = vel.Length();

				if (fSpeed > 0.1f)
				{
					vel = vel / fSpeed;

					Vector vToProj = vorg - getOrigin();
					if (vel.Dot(vToProj) <= 0.0f)
						return false;

					vcomp = vorg + vel * fDist;

					// Gravity compensation for arcing pipe grenades
					float fTime = fDist / fSpeed;
					vcomp.z -= 0.5f * 800.0f * fTime * fTime;
				}
				else
					vcomp = vorg;
			}
			else
				vcomp = vorg;

			return (distanceFrom(vcomp) < BLAST_RADIUS);
		}
	}

	return false;
}

void CBotFortress::enemyLost(edict_t *pEnemy)
{
	if (CBotGlobals::isPlayer(pEnemy) && (CClassInterface::getTF2Class(pEnemy) == TF_CLASS_SPY))
	{
		if (CBotGlobals::isAlivePlayer(pEnemy))
			updateCondition(CONDITION_CHANGED);
	}

	// CBot::enemyLost(pEnemy);
}

bool CBotTF2::setVisible(edict_t *pEntity, bool bVisible)
{
	bool bValid = CBotFortress::setVisible(pEntity, bVisible);

	if (bValid && bVisible)
	{
		if (CTeamFortress2Mod::isTeleporter(pEntity, CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
			addKnownEnemyTeleporter(pEntity);
		else if (CTeamFortress2Mod::isDispenser(pEntity, CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
			addKnownEnemyDispenser(pEntity);
	}

	if (bValid)
	{
		if ((m_pRedPayloadBomb.get() == nullptr) && CTeamFortress2Mod::isPayloadBomb(pEntity, TF2_TEAM_RED))
		{
			m_pRedPayloadBomb = pEntity;
			CTeamFortress2Mod::updateRedPayloadBomb(pEntity);
			// if ( CTeamFortress2Mod::se
		}
		else if ((m_pBluePayloadBomb.get() == nullptr) && CTeamFortress2Mod::isPayloadBomb(pEntity, TF2_TEAM_BLUE))
		{
			m_pBluePayloadBomb = pEntity;
			CTeamFortress2Mod::updateBluePayloadBomb(pEntity);
		}
		else if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(pEntity))
		{
			CTeamFortress2Mod::checkMVMTankBoss(pEntity);
		}
	}

	if (bValid && bVisible)
	{
		edict_t *pTest;

		if (((pTest = m_NearestEnemyRocket.get()) != pEntity)
		    && CTeamFortress2Mod::isHostileProjectile(pEntity, CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
		{
			if ((pTest == nullptr) || (distanceFrom(pEntity) < distanceFrom(pTest)))
			{
				if (pTest)
					m_SecondNearestEnemyRocket = pTest;
				m_NearestEnemyRocket = pEntity;
			}
			else
			{
				edict_t *pSecond = m_SecondNearestEnemyRocket.get();
				if (!pSecond || (distanceFrom(pEntity) < distanceFrom(pSecond)))
				{
					if (m_NearestEnemyRocket.get() != pEntity)
						m_SecondNearestEnemyRocket = pEntity;
				}
			}
		}
	}
	else
	{
		if (pEntity == m_NearestEnemyRocket.get())
		{
			m_NearestEnemyRocket = m_SecondNearestEnemyRocket;
			m_SecondNearestEnemyRocket = nullptr;
		}
		if (pEntity == m_SecondNearestEnemyRocket.get())
			m_SecondNearestEnemyRocket = nullptr;
	}

	if ((ENTINDEX(pEntity) <= gpGlobals->maxClients) && (ENTINDEX(pEntity) > 0))
	{
		if (bVisible)
		{
			TF_Class iPlayerclass = (TF_Class)CClassInterface::getTF2Class(pEntity);

			if (iPlayerclass == TF_CLASS_SPY)
			{
				// check if disguise is not spy on my team
				int iClass, iTeam, iIndex, iHealth;

				CClassInterface::getTF2SpyDisguised(pEntity, &iClass, &iTeam, &iIndex, &iHealth);

				if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pEntity))
				{
					if (iClass != TF_CLASS_SPY) // spies cloaking is normal / non spies cloaking is not!
					{
						if (!m_pCloakedSpy
						    || ((m_pCloakedSpy != pEntity)
						        && CBotGlobals::entityIsValid(m_pCloakedSpy)
						        && (distanceFrom(pEntity) < distanceFrom(m_pCloakedSpy))))
							m_pCloakedSpy = pEntity;
					}
				}
				else if ((iClass == TF_CLASS_MEDIC) && (iTeam == m_iTeam))
				{
					if (!m_pLastSeeMedic.check(pEntity) && CBotGlobals::entityIsAlive(pEntity))
					{
						// i think this spy can cure me!
						if ((m_pLastSeeMedic.check(nullptr)
						     || (distanceFrom(pEntity) < distanceFrom(m_pLastSeeMedic.getLocation())))
						    && !thinkSpyIsEnemy(pEntity, (TF_Class)iClass))
						{
							m_pLastSeeMedic = CBotLastSee(pEntity);
							/*m_pLastSeeMedic = pEntity;
							m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
							m_fLastSeeMedicTime = engine->Time();*/
						}
					}
					else
						m_pLastSeeMedic.update();
				}
			}
			else if (iPlayerclass == TF_CLASS_MEDIC)
			{
				if (m_pLastSeeMedic.check(pEntity))
				{
					m_pLastSeeMedic.update();
					// m_fLastSeeMedicTime = engine->Time();
					// m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
				}
				else if (CBotGlobals::entityIsAlive(pEntity)
				         && ((m_pLastSeeMedic.check(nullptr))
				             || (distanceFrom(pEntity) < distanceFrom(m_pLastSeeMedic.getLocation()))))
				{
					m_pLastSeeMedic = CBotLastSee(pEntity);
					// m_vLastSeeMedic = CBotGlobals::entityOrigin(pEntity);
					// m_fLastSeeMedicTime = engine->Time();
				}
			}
		}
		else
		{
			if (m_pCloakedSpy.get() == pEntity)
				m_pCloakedSpy = nullptr;
		}
	}

	return bValid;
}

void CBotTF2::checkBeingHealed()
{
	static short i;
	static edict_t *p;
	static edict_t *pWeapon;
	static IPlayerInfo *pi;
	static const char *szWeaponName;

	if (m_fCheckHealTime > engine->Time())
		return;

	m_fCheckHealTime = engine->Time() + 1.0f;

	m_bIsBeingHealed = false;
	m_bCanBeUbered   = false;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		p = INDEXENT(i);

		if (p == m_pEdict)
			continue;

		pi = playerinfomanager->GetPlayerInfo(p);

		if (p && pi && (pi->GetTeamIndex() == getTeam()) && CBotGlobals::entityIsValid(p)
		    && p->GetNetworkable()->GetClassName())
		{
			szWeaponName = pi->GetWeaponName();

			if (szWeaponName && *szWeaponName && strcmp(szWeaponName, "tf_weapon_medigun") == 0)
			{
				pWeapon = CTeamFortress2Mod::getMediGun(p);

				if (!pWeapon)
					continue;

				if (CClassInterface::getMedigunHealing(pWeapon)
				    && (CClassInterface::isMedigunTargetting(pWeapon, m_pEdict)))
				{
					if (CClassInterface::getUberChargeLevel(pWeapon) > 99)
						m_bCanBeUbered = true;

					m_bIsBeingHealed = true;
					m_pHealer        = p;
				}
			}
		}
	}
}

// Preconditions :  Current weapon is Medigun
//					pPlayer is not nullptr
//
bool CBotTF2::healPlayer()
{
	static CBotWeapon *pWeap;
	static IPlayerInfo *p;
	static edict_t *pWeapon;
	static Vector vOrigin;
	static Vector vForward;
	static QAngle eyes;
	static float fSpeed;
	static CClient *pClient;

	if (!m_pHeal.get())
		return false;

	if (getHealFactor(m_pHeal) == 0.0f)
		return false;

	// Validate entity BEFORE touching it -- marker may have been freed by another medic's revive
	edict_t *pHealEdict = m_pHeal.get();
	if (!pHealEdict || !CBotGlobals::entityIsValid(pHealEdict))
		return false;

	if (!CBotGlobals::isPlayer(m_pHeal))
	{
		if (!CBotGlobals::entityIsAlive(m_pHeal))
			return false;

		if (strcmp(pHealEdict->GetClassName(), "entity_revive_marker") != 0)
			return false;
	}
	else
	{
		p = playerinfomanager->GetPlayerInfo(m_pHeal);
		if (!p || p->IsDead() || !p->IsConnected() || p->IsObserver())
			return false;
	}

	Vector vPatientOrigin = CBotGlobals::entityOrigin(m_pHeal);
	pWeap   = getCurrentWeapon();

	// Default standoff: stay 150u behind patient relative to their movement direction,
	// or toward medic's current position if patient is stationary
	{
		Vector vDefaultDir;
		Vector vPatVel;
		CClassInterface::getVelocity(m_pHeal, &vPatVel);
		vPatVel.z = 0;
		if (vPatVel.Length2D() > 10.0f)
			vDefaultDir = -vPatVel / vPatVel.Length2D();  // opposite of patient movement
		else
		{
			vDefaultDir = getOrigin() - vPatientOrigin;
			vDefaultDir.z = 0;
			if (vDefaultDir.Length2D() < 10.0f)
				vDefaultDir = Vector(1, 0, 0);  // arbitrary default behind
			else
				vDefaultDir = vDefaultDir / vDefaultDir.Length2D();
		}
		vOrigin = vPatientOrigin + vDefaultDir * 150.0f;
	}

	bool bShouldUpdate = false;

	// Update more aggressively: throttle 0.5-1.0s instead of 1-2s
	float fPatientMoved = (vPatientOrigin - m_vLastMedicPatientOrigin).Length();
	float fThreatAngular = 0.0f;

	if (m_fMedicUpdatePosTime < engine->Time())
	{
		bShouldUpdate = true;
	}
	else if (fPatientMoved > 100.0f)
	{
		// Patient moved significantly -- force update
		bShouldUpdate = true;
	}
	else
	{
		// Check if enemy has flanked -- force update if threat direction changed
		edict_t *pEnemyCheck = m_pEnemy.get();
		if (pEnemyCheck && CBotGlobals::entityIsValid(pEnemyCheck)
		    && CBotGlobals::entityIsAlive(pEnemyCheck))
		{
			Vector vCurDir = CBotGlobals::entityOrigin(pEnemyCheck) - getOrigin();
			vCurDir.z = 0;
			if (vCurDir.Length2D() > 0.1f && m_vLastMedicEnemyDir.Length2D() > 0.1f)
			{
				vCurDir = vCurDir / vCurDir.Length2D();
				Vector vLastDir = m_vLastMedicEnemyDir / m_vLastMedicEnemyDir.Length2D();
				fThreatAngular = acosf(clamp(vCurDir.Dot(vLastDir), -1.0f, 1.0f));
				if (fThreatAngular > 0.5f) // enemy moved >30° laterally
					bShouldUpdate = true;
			}
		}
	}

	if (bShouldUpdate)
	{
		float fRand;
		float fSpeed = 0.0f;

		fRand   = randomFloat(0.5f, 1.0f);

		pClient = CClients::get(m_pHeal);

		if (pClient)
			fSpeed = pClient->getSpeed();

		m_fMedicUpdatePosTime = engine->Time() + (fRand * (1.0f - (fSpeed / 320)));

		// Update last known patient origin
		m_vLastMedicPatientOrigin = vPatientOrigin;

		if (p && (p->GetLastUserCommand().buttons & IN_ATTACK))
		{
			// Smart positioning: stay on the far side of the patient from enemies
			if (!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
			{
				edict_t *pEnemy = m_pEnemy.get();
				if (pEnemy && CBotGlobals::entityIsValid(pEnemy) && CBotGlobals::entityIsAlive(pEnemy)
				    && isVisible(pEnemy))
				{
					Vector vEnemyPos  = CBotGlobals::entityOrigin(pEnemy);
					m_vLastMedicEnemyDir = vEnemyPos - getOrigin();
					Vector vFromEnemy = vPatientOrigin - vEnemyPos;
					vFromEnemy.z      = 0;
					float fLen         = vFromEnemy.Length();
					if (fLen > 0.1f)
					{
						vFromEnemy           = vFromEnemy / fLen;
						Vector vCandidate    = vPatientOrigin + (vFromEnemy * 250.0f);
						CTraceFilterWorldAndPropsOnly filter;
						CBotGlobals::traceLine(vEnemyPos, vCandidate, MASK_SOLID_BRUSHONLY, &filter);
						if (CBotGlobals::getTraceResult()->fraction < 1.0f)
						{
							vOrigin = CBotGlobals::getTraceResult()->endpos;
							Vector vToCover = vOrigin - vEnemyPos;
							vToCover.z      = 0;
							if (vToCover.Length() > 0.1f)
							{
								vToCover = vToCover / vToCover.Length();
								vOrigin  = vOrigin + (vToCover * 32.0f);
							}
							// Crouch if the cover is a low obstacle we can hide behind
							if (fabs(vOrigin.z - getOrigin().z) < 48.0f)
								m_bShouldCrouchCover = true;
						}
						else
						{
							vOrigin = vPatientOrigin + (vFromEnemy * 150.0f);
							m_bShouldCrouchCover = false;
						}
					}
				}
				else
				{
					// No visible enemy: use last enemy or patient velocity, not facing
					Vector vAwayDir;
					edict_t *pLastEnemy = m_pLastEnemy.get();
					if (pLastEnemy && CBotGlobals::entityIsValid(pLastEnemy))
					{
						vAwayDir = vPatientOrigin - CBotGlobals::entityOrigin(pLastEnemy);
						vAwayDir.z = 0;
					}
					if (vAwayDir.Length2D() < 10.0f)
					{
						// Fall back to patient velocity direction
						Vector vPatVel;
						CClassInterface::getVelocity(m_pHeal, &vPatVel);
						vPatVel.z = 0;
						if (vPatVel.Length2D() > 10.0f)
							vAwayDir = -vPatVel;
					}
					if (vAwayDir.Length2D() < 10.0f)
						vAwayDir = getOrigin() - vPatientOrigin;
					vAwayDir.z = 0;
					if (vAwayDir.Length2D() > 10.0f)
					{
						vAwayDir = vAwayDir / vAwayDir.Length2D();
						vOrigin  = vPatientOrigin + vAwayDir * 150.0f;
					}
				}
			}
			m_fHealingMoveTime = engine->Time();
		}
		else if (fSpeed > 100.0f)
			m_fHealingMoveTime = engine->Time();

		if (m_fHealingMoveTime == 0.0f)
			m_fHealingMoveTime = engine->Time();

		m_vMedicPosition = vOrigin;
	}
	else
	{
		m_vMedicPosition = vOrigin;
	}

	// Validate target position is reachable -- trace from medic toward target,
	// fall back to patient origin if blocked by geometry
	{
		Vector vToTarget = m_vMedicPosition - getOrigin();
		vToTarget.z = 0;
		float fTargetDist = vToTarget.Length2D();
		if (fTargetDist > 64.0f)
		{
			CTraceFilterWorldAndPropsOnly targetFilter;
			CBotGlobals::traceLine(getOrigin(), m_vMedicPosition, MASK_SOLID_BRUSHONLY, &targetFilter);
			if (CBotGlobals::getTraceResult()->fraction < 0.5f)
				m_vMedicPosition = vPatientOrigin;
		}
	}

	if (CBotGlobals::isPlayer(m_pHeal))
		{
			if (m_pNearestPipeGren.get() || m_NearestEnemyRocket.get())
				m_iDesiredResistType = RESIST_EXPLO;
			else if (CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pHeal) || CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict))
				m_iDesiredResistType = RESIST_FIRE;
			else if (randomInt(0, 1) == 1)
				m_iDesiredResistType = RESIST_BULLETS;
		}

	/*if ( CTeamFortress2Mod::hasRoundStarted() && (m_fHealingMoveTime + 8.0f < engine->Time()) )
	{
	m_pLastHeal
	return false;
	}*/
	edict_t *pMedigun;

	if ((pWeap->getID() == TF2_WEAPON_MEDIGUN) && ((pMedigun = pWeap->getWeaponEntity()) != nullptr))
	{
		if (CClassInterface::getChargeResistType(pMedigun) != m_iDesiredResistType)
		{
			if (randomInt(0, 1) == 1)
				m_pButtons->tap(IN_RELOAD);
		}
	}

	if (distanceFrom(m_vMedicPosition) < 100)
		stopMoving();
	else
		setMoveTo(m_vMedicPosition);

	// Crouch behind low cover while healing
	if (m_bShouldCrouchCover && distanceFrom(m_vMedicPosition) < 100)
		duck();
	else
		m_bShouldCrouchCover = false;

	// Dodge incoming projectiles while healing
	if (m_fStrafeTime < engine->Time())
	{
		edict_t *pIncoming = m_NearestEnemyRocket.get();
		if (!pIncoming)
			pIncoming = m_pNearestPipeGren.get();

			if (pIncoming && CBotGlobals::entityIsValid(pIncoming)
			    && CBotGlobals::entityIsAlive(pIncoming) && incomingRocket(800.0f))
			{
				Vector vOrigin  = getOrigin();
				Vector vProjOrg = CBotGlobals::entityOrigin(pIncoming);
				Vector vDodge   = vOrigin - vProjOrg;
				vDodge.z        = 0;
				float fLen      = vDodge.Length2D();
				if (fLen > 0.1f)
				{
					vDodge = vDodge / fLen;
					Vector vDest = vOrigin + vDodge * (BLAST_RADIUS + 128.0f);

				// Trace-validate dodge destination; try lateral alternatives if blocked
				CTraceFilterWorldAndPropsOnly filter;
				CBotGlobals::traceLine(vOrigin, vDest, MASK_SOLID_BRUSHONLY, &filter);
				trace_t *tr = CBotGlobals::getTraceResult();
				if (tr->fraction < 1.0f)
				{
					Vector vAlt1(-vDodge.y, vDodge.x, 0);
					Vector vAlt2(vDodge.y, -vDodge.x, 0);
					Vector vDest1 = vOrigin + vAlt1 * (BLAST_RADIUS + 128.0f);
					Vector vDest2 = vOrigin + vAlt2 * (BLAST_RADIUS + 128.0f);
					CBotGlobals::traceLine(vOrigin, vDest1, MASK_SOLID_BRUSHONLY, &filter);
					tr = CBotGlobals::getTraceResult();
					if (tr->fraction < 1.0f)
					{
						CBotGlobals::traceLine(vOrigin, vDest2, MASK_SOLID_BRUSHONLY, &filter);
						tr    = CBotGlobals::getTraceResult();
						vDest = (tr->fraction >= 1.0f) ? vDest2 : vOrigin;
					}
					else
						vDest = vDest1;
				}

				setMoveTo(vDest);
					m_fStrafeTime = engine->Time() + 0.2f;
					m_fSideSpeed  = (vDodge.y > 0 ? 1.0f : -1.0f) * m_fIdealMoveSpeed;
				}
			}
		}

		if (!pWeap || !pWeap->getWeaponInfo())
		return false;

	pWeapon = INDEXENT(pWeap->getWeaponIndex());

	if (pWeapon == nullptr)
		return false;

	m_bIncreaseSensitivity = true;

	edict_t *pWeaponEdict = INDEXENT(pWeap->getWeaponIndex());
	if (!pWeaponEdict || !CBotGlobals::entityIsValid(pWeaponEdict))
		return false;

	edict_t *pCurrentTarget = CClassInterface::getMedigunTarget(pWeaponEdict);

	// Look at healee
	lookAtEdict(m_pHeal);
	setLookAtTask(LOOK_EDICT);

	if (!CBotGlobals::isPlayer(m_pHeal))
		setVisible(m_pHeal, true);

	if (pCurrentTarget == m_pHeal.get())
	{
		// Already healing the right target -- keep beam connected
		primaryAttack(true);
	}
	else if (m_fHealClickTime < engine->Time())
	{
		// Wrong target or no target -- release briefly, aim, and re-fire
		if (pCurrentTarget != nullptr)
		{
			m_pButtons->letGo(IN_ATTACK);
			m_fHealClickTime = engine->Time() + 0.15f;
		}
		else
		{
			primaryAttack(true);
		}
	}

	m_pLastHeal = m_pHeal;

	if (CBotGlobals::isPlayer(m_pHeal))
	{
		// Simple UBER check : healing player not ubered already
		if (!CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pHeal) && !CTeamFortress2Mod::isFlagCarrier(m_pHeal)
		        && (m_pEnemy && isVisible(m_pEnemy))
		    || (((((float)m_pPlayerInfo->GetHealth()) / m_pPlayerInfo->GetMaxHealth()) < 0.33)
		        || (getHealthPercent() < 0.33)))
		{
			if (CTeamFortress2Mod::hasRoundStarted())
			{
				// uber if ready / and round has started
				m_pButtons->tap(IN_ATTACK2);
			}
		}
	}
	else if (CBotGlobals::entityIsValid(m_pHeal))
	{
		int iCharge = CClassInterface::getUberChargeLevel(pWeaponEdict);
		int iItemDef = CClassInterface::TF2_getItemDefinitionIndex(pWeaponEdict);
		// Vaccinator (998) holds up to 4 ubers at 25% each
		int iThreshold = (iItemDef == 998) ? 25 : 100;
		if (iCharge >= iThreshold)
		{
			// Reviving marker -- pop uber to speed up revive
			m_pButtons->tap(IN_ATTACK2);
		}
	}

	return true;
}
// The lower the better
float CBotTF2::getEnemyFactor(edict_t *pEnemy)
{
	float fPreFactor = 0;

	// Player
	if (CBotGlobals::isPlayer(pEnemy))
	{
		IServerEntity *pServerEnt = pEnemy->GetIServerEntity();
		const char *szModel       = pServerEnt ? pServerEnt->GetModelName().ToCStr() : nullptr;
		IPlayerInfo *p      = playerinfomanager->GetPlayerInfo(pEnemy);

		if (CTeamFortress2Mod::isFlagCarrier(pEnemy))
		{
			// this enemy is carrying the flag, attack!
			// shoot flag carrier even if 1000 units away from nearest enemy
			fPreFactor = -1000.0f;
		}
		else if (!CTeamFortress2Mod::isMapType(TF_MAP_CTF)
		         && (CTeamFortress2Mod::isCapping(pEnemy) || CTeamFortress2Mod::isDefending(pEnemy)))
		{
			// this enemy is capping the point, attack!
			fPreFactor = -400.0f;
		}
		else if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pEnemy))
		{
			// dont shoot ubered player unlesss he's the only thing around for 2000 units
			fPreFactor = 2000.0f;
		}
		else if ((m_iClass == TF_CLASS_SPY) && isDisguised() && (p != nullptr)
		         && (CBotGlobals::DotProductFromOrigin(CBotGlobals::entityOrigin(pEnemy), getOrigin(),
		                                               p->GetLastUserCommand().viewangles)
		             > 0.1f))
		{
			// I'm disguised as a spy but this guy can see me , better lay off the attack unless theres someone else
			// around
			fPreFactor = 1000.0f;
		}
		// models/bots/demo/bot_sentry_buster.mdl
		// 0000000000111111111122222222223333333
		// 0123456789012345678901234567890123456
		else if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && szModel && (strlen(szModel) > 28)
		         && (szModel[7] == 'b') && (szModel[12] == 'd')
		         && (szModel[17] == 'b') && (szModel[21] == 's')
		         && (szModel[28] == 'b'))
		{
			// sentry buster: engineers rush to protect sentry, others ignore
			fPreFactor = (m_iClass == TF_CLASS_ENGINEER) ? -500.0f : 2000.0f;
		}
		else
		{
			int iclass = CClassInterface::getTF2Class(pEnemy);

			if (iclass == TF_CLASS_MEDIC)
			{
				// shoot medic even if 250 units further from nearest enemy (approx. healing range)
				fPreFactor = -260.0f;
			}
			else if (iclass == TF_CLASS_SPY)
			{
				// shoot spy even if a little further from nearest enemy
				fPreFactor = -400.0f;
			}
			else if (iclass == TF_CLASS_SNIPER)
			{
				// I'm a spy, and I can see a sniper
				//	if ( m_iClass == TF_CLASS_SPY )
				//		fPreFactor = -600;
				//	else
				if (m_iClass == TF_CLASS_SNIPER)
					fPreFactor = -2048.0f;
				else
				{
					// Behind the sniper ATTACK
					if (CBotGlobals::DotProductFromOrigin(pEnemy, getOrigin()) > 0.98) // 10 deg
						fPreFactor = -1024.0f;
					else // Sniper can see me
						fPreFactor = -300.0f;
				}
			}
			else if (iclass == TF_CLASS_ENGINEER)
			{
				// I'm a spy and I'm attacking an engineer!
				if (m_iClass == TF_CLASS_SPY)
				{
					edict_t *pSentry = nullptr;

					fPreFactor       = -400.0f;

					if ((pSentry = m_pNearestEnemySentry.get()) != nullptr)
					{
						if ((CTeamFortress2Mod::getSentryOwner(pSentry) == pEnemy)
						    && CTeamFortress2Mod::isSentrySapped(pSentry)
						    && (distanceFrom(pSentry) < TF2_MAX_SENTRYGUN_RANGE))
						{
							// this guy is the owner of a disabled sentry gun -- take him out!
							fPreFactor = -1024.0f;
						}
					}
				}
			}
		}
	}
	else
	{
		float fBossFactor = -1024.0f;

		if (CTeamFortress2Mod::isSentry(pEnemy, CTeamFortress2Mod::getEnemyTeam(getTeam())))
		{
			edict_t *pOwner = CTeamFortress2Mod::getBuildingOwner(ENGI_SENTRY, ENTINDEX(pEnemy));

			if (pOwner && isVisible(pOwner))
			{
				// owner probably repairing it -- maybe make use of others around me
				fPreFactor = -512.0f;
			}
			else if (CClassInterface::getSentryEnemy(pEnemy) != m_pEdict)
				fPreFactor = -1124.0f;
			else
				fPreFactor = -768.0f;
		}
		else if (CTeamFortress2Mod::isBoss(pEnemy, &fBossFactor))
		{
			fPreFactor = fBossFactor * bot_bossattackfactor.GetFloat();
		}
		else if (CTeamFortress2Mod::isPipeBomb(pEnemy, CTeamFortress2Mod::getEnemyTeam(m_iTeam)))
		{
			fPreFactor = 320.0f;
		}
	}

	fPreFactor += distanceFrom(pEnemy);

	return fPreFactor;
}

bool CBotFortress::wantToNest()
{
	return (!hasFlag() && ((getClass() != TF_CLASS_ENGINEER) && (m_pSentryGun.get() != nullptr))
	        && ((getClass() != TF_CLASS_MEDIC) || !m_pHeal) && (getHealthPercent() < 0.95)
	        && (nearbyFriendlies(256.0f) < 2));
}

void CBotTF2::teleportedPlayer(void)
{
	m_iTeleportedPlayers++;
}

void CBotTF2::getTasks(unsigned int iIgnore)
{
	static bool bIsUbered;
	static TF_Class iClass;
	static int iMetal;
	static bool bNeedAmmo;
	static bool bNeedHealth;
	static CBotUtilities utils;
	static CBotWeapon *pWeapon;
	static CWaypoint *pWaypointResupply;
	static CWaypoint *pWaypointAmmo;
	static CWaypoint *pWaypointHealth;
	static CBotUtility *next;
	static float fResupplyDist;
	static float fHealthDist;
	static float fAmmoDist;
	static bool bHasFlag;
	static float fGetFlagUtility;
	static float fDefendFlagUtility;
	static int iTeam;
	static float fMetalPercent;
	static Vector vOrigin;
	static unsigned char *failedlist;

	static bool bMoveObjs;

	static bool bSentryHasEnemy;
	static int iSentryLevel;
	static int iDispenserLevel;
	static int iAllySentryLevel;
	static int iAllyDispLevel;

	static float fEntranceDist;
	static float fExitDist;
	static float fUseDispFactor;

	static float fAllyDispenserHealthPercent;
	static float fAllySentryHealthPercent;

	static float fSentryHealthPercent;
	static float fDispenserHealthPercent;
	static float fTeleporterEntranceHealthPercent;
	static float fTeleporterExitHealthPercent;

	static float fSentryPlaceTime;
	static float fDispenserPlaceTime;
	static float fTeleporterEntPlaceTime;
	static float fTeleporterExtPlaceTime;

	static edict_t *pMedigun;
	static float fSentryUtil;
	static int iMetalInDisp;

	static int numplayersonteam;
	static int numplayersonteam_alive;

	static bool bCheckCurrent;

	static CBotWeapon *pBWMediGun = nullptr;
	// static float fResupplyUtil = 0.5;
	// static float fHealthUtil = 0.5;
	// static float fAmmoUtil = 0.5;

	// if in setup time this will tell bot not to shoot yet
	wantToShoot(CTeamFortress2Mod::hasRoundStarted());
	wantToListen(CTeamFortress2Mod::hasRoundStarted());

	// Refill ammo before setup ends if we used any during messing around
	if (!CTeamFortress2Mod::hasRoundStarted() && m_iClass != TF_CLASS_MEDIC)
	{
		float fRemaining = CTeamFortress2Mod::getRoundTime() - engine->Time();
		if (fRemaining > 0.1f && fRemaining < 5.0f)
		{
			CBotWeapon *pPrimary = m_pWeapons->getPrimaryWeapon();
			if (pPrimary && pPrimary->hasWeapon() && pPrimary->outOfAmmo(this)
			    && !m_pSchedules->hasSchedule(SCHED_MESSAROUND))
			{
				updateCondition(CONDITION_NEED_AMMO);
			}
		}
	}

	// Force re-evaluation if bot hasn't moved recently (stuck detection)
	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
	{
		Vector vDelta = getOrigin() - m_vLastReEvalPos;
		if (vDelta.Length2D() > 32.0f || engine->Time() < (m_fReEvalTime + 0.5f))
			return;
	}

	removeCondition(CONDITION_CHANGED);
	m_vLastReEvalPos = getOrigin();
	m_fReEvalTime    = engine->Time();

	bIsUbered = CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict);

	if (bIsUbered && hasEnemy())
	{
		// keep attacking enemy -- no change to task
		return;
	}

	bCheckCurrent          = true; // important for checking the current schedule if not empty
	iMetal                 = 0;
	pBWMediGun             = nullptr;
	vOrigin                = getOrigin();
	bNeedAmmo              = false;
	bNeedHealth            = false;
	fResupplyDist          = 1;
	fHealthDist            = 1;
	fAmmoDist              = 1;
	bHasFlag               = false;
	fGetFlagUtility        = 0.5;
	fDefendFlagUtility     = 0.5;
	iTeam                  = m_iTeam;
	bHasFlag               = hasFlag();
	failedlist             = nullptr;

	numplayersonteam       = CBotGlobals::numPlayersOnTeam(iTeam, false);
	numplayersonteam_alive = CBotGlobals::numPlayersOnTeam(iTeam, true);

	// UNUSED
	// Shadow/Time must be Floating point
	/*if(m_fBlockPushTime < engine->Time())
	{
	    m_bBlockPushing = (randomFloat(0.0,100)>50); // 50 % block pushing
	    m_fBlockPushTime = engine->Time() + randomFloat(10.0f,30.0f); // must be floating point
	}*/

	// No Enemy now
	if ((m_iClass == TF_CLASS_SNIPER) && !hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		CBotWeapon *pWp = getCurrentWeapon();
		if (pWp && pWp->isProjectile())
		{
			// Bow: never scoped, nothing to unzoom
		}
		else if (CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict))
			secondaryAttack();
	}

	iClass      = getClass();

	bNeedAmmo   = hasSomeConditions(CONDITION_NEED_AMMO);

	// don't need health if being healed or ubered!
	bNeedHealth = hasSomeConditions(CONDITION_NEED_HEALTH) && !m_bIsBeingHealed && !bIsUbered;

	if (m_pHealthkit)
	{
		if (!CBotGlobals::entityIsValid(m_pHealthkit))
			m_pHealthkit = nullptr;
	}

	if (m_pAmmo)
	{
		if (!CBotGlobals::entityIsValid(m_pAmmo))
			m_pAmmo = nullptr;
	}

	pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));

	if (pWeapon != nullptr)
	{
		iMetal        = pWeapon->getAmmo(this);
		fMetalPercent = (float)iMetal / 200;
	}
	if (bNeedAmmo || bNeedHealth)
	{
		Vector vOrigin = getOrigin();

		WaypointList *failed;
		m_pNavigator->getFailedGoals(&failed);

		failedlist    = CWaypointLocations::resetFailedWaypoints(failed);

		fResupplyDist = 1;
		fHealthDist   = 1;
		fAmmoDist     = 1;

		// don't go back to resupply if ubered
		if (!bIsUbered)
			pWaypointResupply = CWaypoints::getWaypoint(CWaypoints::getClosestFlagged(
			    CWaypointTypes::W_FL_RESUPPLY, vOrigin, iTeam, &fResupplyDist, failedlist));

		if (bNeedAmmo)
			pWaypointAmmo = CWaypoints::getWaypoint(
			    CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_AMMO, vOrigin, iTeam, &fAmmoDist, failedlist));
		if (bNeedHealth)
			pWaypointHealth = CWaypoints::getWaypoint(
			    CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_HEALTH, vOrigin, iTeam, &fHealthDist, failedlist));
	}

	if (iClass == TF_CLASS_ENGINEER)
	{
		checkBuildingsValid(true);
		updateCarrying();
	}

	ADD_UTILITY(BOT_UTIL_CAPTURE_FLAG,
	            (CTeamFortress2Mod::isMapType(TF_MAP_CTF) || CTeamFortress2Mod::isMapType(TF_MAP_SD)) && bHasFlag,
	            0.95f);

	if (iClass == TF_CLASS_ENGINEER)
	{
		bool bCanBuild                   = m_pWeapons->hasWeapon(TF2_WEAPON_BUILDER);

		bMoveObjs                        = rcbot_move_obj.GetBool();

		iSentryLevel                     = 0;
		iDispenserLevel                  = 0;
		iAllySentryLevel                 = 0;
		iAllyDispLevel                   = 0;

		fEntranceDist                    = 99999.0f;
		fExitDist                        = 99999.0f;
		fUseDispFactor                   = 0.0f;

		fAllyDispenserHealthPercent      = 1.0f;
		fAllySentryHealthPercent         = 1.0f;

		fSentryHealthPercent             = 1.0f;
		fDispenserHealthPercent          = 1.0f;
		fTeleporterEntranceHealthPercent = 1.0f;
		fTeleporterExitHealthPercent     = 1.0f;

		fSentryPlaceTime                 = (engine->Time() - m_fSentryPlaceTime);
		fDispenserPlaceTime              = (engine->Time() - m_fDispenserPlaceTime);
		fTeleporterEntPlaceTime          = (engine->Time() - m_fTeleporterEntPlacedTime);
		fTeleporterExtPlaceTime          = (engine->Time() - m_fTeleporterExtPlacedTime);

		if (m_pTeleExit.get())
		{
			fExitDist                    = distanceFrom(m_pTeleExit);

			fTeleporterExitHealthPercent = CClassInterface::getTeleporterHealth(m_pTeleExit) / 180;

			ADD_UTILITY(
			    BOT_UTIL_ENGI_MOVE_EXIT,
			    (CTeamFortress2Mod::hasRoundStarted() || CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			        && (!m_bIsCarryingObj || m_bIsCarryingTeleExit) && bMoveObjs && m_pTeleEntrance && m_pTeleExit
			        && m_fTeleporterExtPlacedTime && (fTeleporterExtPlaceTime > rcbot_move_tele_time.GetFloat())
			        && (((60.0f * m_iTeleportedPlayers) / fTeleporterExtPlaceTime) < rcbot_move_tele_tpm.GetFloat()),
			    (fTeleporterExitHealthPercent * getHealthPercent() * fMetalPercent) + ((int)m_bIsCarryingTeleExit));
		}

		if (m_pTeleEntrance.get())
		{
			fEntranceDist                    = distanceFrom(m_pTeleEntrance);

			fTeleporterEntranceHealthPercent = CClassInterface::getTeleporterHealth(m_pTeleEntrance) / 180;

			ADD_UTILITY(
			    BOT_UTIL_ENGI_MOVE_ENTRANCE,
			    (!m_bIsCarryingObj || m_bIsCarryingTeleEnt) && bMoveObjs && m_bEntranceVectorValid && m_pTeleEntrance
			        && m_pTeleExit && m_fTeleporterEntPlacedTime
			        && (fTeleporterEntPlaceTime > rcbot_move_tele_time.GetFloat())
			        && (((60.0f * m_iTeleportedPlayers) / fTeleporterEntPlaceTime) < rcbot_move_tele_tpm.GetFloat()),
			    (fTeleporterEntranceHealthPercent * getHealthPercent() * fMetalPercent) + ((int)m_bIsCarryingTeleEnt));
		}

		if (m_pSentryGun.get())
		{
			bSentryHasEnemy = (CClassInterface::getSentryEnemy(m_pSentryGun) != nullptr);
			iSentryLevel =
			    CClassInterface::getTF2UpgradeLevel(m_pSentryGun); // CTeamFortress2Mod::getSentryLevel(m_pSentryGun);
			fSentryHealthPercent = ((float)CClassInterface::getSentryHealth(m_pSentryGun))
			                     / CClassInterface::getTF2GetBuildingMaxHealth(m_pSentryGun);
			// move sentry
			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_SENTRY,
			            (CTeamFortress2Mod::hasRoundStarted() || CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			                && (!m_bIsCarryingObj || m_bIsCarryingSentry) && bMoveObjs && (m_fSentryPlaceTime > 0.0f)
			                && !bHasFlag && m_pSentryGun && (CClassInterface::getSentryEnemy(m_pSentryGun) == nullptr)
			                && ((m_fLastSentryEnemyTime + 15.0f) < engine->Time())
			                && (!CTeamFortress2Mod::isMapType(TF_MAP_CP)
			                    || CTeamFortress2Mod::m_ObjectiveResource.testProbWptArea(m_iSentryArea, m_iTeam))
			                && (fSentryPlaceTime > rcbot_move_sentry_time.GetFloat())
			                && (((60.0f * m_iSentryKills) / fSentryPlaceTime) < rcbot_move_sentry_kpm.GetFloat()),
			            (fMetalPercent * getHealthPercent() * fSentryHealthPercent) + ((int)m_bIsCarryingSentry));
		}

		if (m_pDispenser.get())
		{
			iMetalInDisp    = CClassInterface::getTF2DispMetal(m_pDispenser);
			iDispenserLevel = CClassInterface::getTF2UpgradeLevel(
			    m_pDispenser); // CTeamFortress2Mod::getDispenserLevel(m_pDispenser);
			fDispenserHealthPercent = ((float)CClassInterface::getDispenserHealth(m_pDispenser))
			                        / CClassInterface::getTF2GetBuildingMaxHealth(m_pDispenser);

			fUseDispFactor = (((float)iMetalInDisp) / 400) * (1.0f - fMetalPercent) * ((float)iDispenserLevel / 3)
			               * (1000.0f / distanceFrom(m_pDispenser));

			// move disp
			ADD_UTILITY(BOT_UTIL_ENGI_MOVE_DISP,
			            (CTeamFortress2Mod::hasRoundStarted() || CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			                && (!m_bIsCarryingObj || m_bIsCarryingDisp) && bMoveObjs && (m_fDispenserPlaceTime > 0.0f)
			                && !bHasFlag && m_pDispenser && (fDispenserPlaceTime > rcbot_move_disp_time.GetFloat())
			                && (((60.0f * m_fDispenserHealAmount) / fDispenserPlaceTime)
			                    < rcbot_move_disp_healamount.GetFloat()),
			            ((((float)iMetalInDisp) / 400) * fMetalPercent * getHealthPercent() * fDispenserHealthPercent)
			                + ((int)m_bIsCarryingDisp));
		}

		if (m_pNearestDisp && (m_pNearestDisp.get() != m_pDispenser.get()))
		{
			iMetalInDisp   = CClassInterface::getTF2DispMetal(m_pNearestDisp);
			iAllyDispLevel = CClassInterface::getTF2UpgradeLevel(
			    m_pNearestDisp); // CTeamFortress2Mod::getDispenserLevel(m_pDispenser);
			fAllyDispenserHealthPercent = ((float)CClassInterface::getDispenserHealth(m_pNearestDisp))
			                            / CClassInterface::getTF2GetBuildingMaxHealth(m_pNearestDisp);

			fUseDispFactor = (((float)iMetalInDisp) / 400) * (1.0f - fMetalPercent) * ((float)iAllyDispLevel / 3)
			               * (1000.0f / distanceFrom(m_pNearestDisp));

			ADD_UTILITY(
			    BOT_UTIL_GOTODISP,
			    m_pNearestDisp && !CClassInterface::isObjectBeingBuilt(m_pNearestDisp) && (bNeedAmmo || bNeedHealth),
			    fUseDispFactor
			        + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f
			                                                                                               : 0.0f));
			ADD_UTILITY(BOT_UTIL_REMOVE_TMDISP_SAPPER,
			            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pNearestDisp
			                && CTeamFortress2Mod::isDispenserSapped(m_pNearestDisp),
			            1.1f);
			ADD_UTILITY(BOT_UTIL_UPGTMDISP,
			            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && (m_pNearestDisp != nullptr)
			                && (m_pNearestDisp != m_pDispenser)
			                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pNearestDisp)))
			                && ((iAllyDispLevel < 3) || (fAllyDispenserHealthPercent < 1.0f)),
			            0.88 + ((1.0f - fAllyDispenserHealthPercent) * 0.12));

			// Help build / repair / speed up ally teleporter construction
			int iAllyTeleLevel = 0;
			float fAllyTeleHealthPercent = 1.0f;
			if (m_pNearestEnemyTeleporter.get() == nullptr && m_pNearestTeleEntrance
			    && m_pNearestTeleEntrance != m_pTeleEntrance)
			{
				iAllyTeleLevel = CClassInterface::getTF2UpgradeLevel(m_pNearestTeleEntrance);
				fAllyTeleHealthPercent = CClassInterface::getTeleporterHealth(m_pNearestTeleEntrance)
				    / CClassInterface::getTF2GetBuildingMaxHealth(m_pNearestTeleEntrance);
				ADD_UTILITY(BOT_UTIL_UPGTMTELENT,
				            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pNearestTeleEntrance
				                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pNearestTeleEntrance)))
				                && ((iAllyTeleLevel < 3) || (fAllyTeleHealthPercent < 0.99f)),
				            0.85 + ((1.0f - fAllyTeleHealthPercent) * 0.15));
			}
		}

		if (m_pNearestAllySentry && (m_pNearestAllySentry.get() != m_pSentryGun.get())
		    && !CClassInterface::getTF2BuildingIsMini(m_pNearestAllySentry))
		{
			iAllySentryLevel         = CClassInterface::getTF2UpgradeLevel(m_pNearestAllySentry);
			fAllySentryHealthPercent = CClassInterface::getSentryHealth(m_pNearestAllySentry);
			fAllySentryHealthPercent =
			    fAllySentryHealthPercent / CClassInterface::getTF2GetBuildingMaxHealth(m_pNearestAllySentry);

			ADD_UTILITY(BOT_UTIL_REMOVE_TMSENTRY_SAPPER,
			            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pNearestAllySentry
			                && CTeamFortress2Mod::isSentrySapped(m_pNearestAllySentry),
			            1.1f);
			ADD_UTILITY(BOT_UTIL_UPGTMSENTRY,
			            (fAllySentryHealthPercent > 0.0f) && !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time())
			                && !bHasFlag && m_pNearestAllySentry && (m_pNearestAllySentry != m_pSentryGun)
			                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pNearestAllySentry)))
			                && ((iAllySentryLevel < 3) || (fAllySentryHealthPercent < 0.99f)
			                    || (CClassInterface::getTF2SentryShells(m_pNearestAllySentry) < 50)),
			            0.88 + ((1.0f - fAllySentryHealthPercent) * 0.12));
		}

		fSentryUtil = 0.8 + (((float)((int)bNeedAmmo)) * 0.1) + (((float)(int)bNeedHealth) * 0.1);

		ADD_UTILITY(BOT_UTIL_PLACE_BUILDING, m_bIsCarryingObj,
		            1.0f); // something went wrong moving this- I still have it!!!

		// destroy and build anew
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_SENTRY,
		            !m_bIsCarryingObj && (iMetal >= 130) && (m_pSentryGun.get() != nullptr)
		                && !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iSentryArea),
		            fSentryUtil);
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_DISP,
		            !m_bIsCarryingObj && (iMetal >= 125) && (m_pDispenser.get() != nullptr)
		                && !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iDispenserArea),
		            randomFloat(0.7, 0.9));
		ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_ENTRANCE,
		            !m_bIsCarryingObj && (iMetal >= 125) && (m_pTeleEntrance.get() != nullptr)
		                && !CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iTeleEntranceArea),
		            randomFloat(0.7, 0.9));
		// ADD_UTILITY(BOT_UTIL_ENGI_DESTROY_EXIT, (iMetal>=125) && (m_pTeleExit.get()!=nullptr) &&
		// !CPoints::isValidArea(m_iTeleExitArea),randomFloat(0.7,0.9));

		if (bCanBuild)
		{
			ADD_UTILITY(BOT_UTIL_BUILDSENTRY, !m_bIsCarryingObj && !bHasFlag && !m_pSentryGun && (iMetal >= 130),
			            CTeamFortress2Mod::isMapType(TF_MAP_MVM) ? 0.95f : 0.9f);
		ADD_UTILITY(BOT_UTIL_BUILDDISP,
		            !m_bIsCarryingObj && !bHasFlag && m_pSentryGun
		                && (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
		                    || CClassInterface::getSentryHealth(m_pSentryGun) > 125)
		                && !m_pDispenser && (iMetal >= 100),
		            fSentryUtil);

			if (CTeamFortress2Mod::isAttackDefendMap() && (iTeam == TF2_TEAM_BLUE))
			{
				ADD_UTILITY(BOT_UTIL_BUILDTELEXT,
				            (fSentryHealthPercent > 0.99f) && !m_bIsCarryingObj && !bHasFlag && !m_pTeleExit
				                && (iMetal >= 125),
				            randomFloat(0.7f, 0.9f));
				ADD_UTILITY(BOT_UTIL_BUILDTELENT,
				            !bSentryHasEnemy && (fSentryHealthPercent > 0.99f) && !m_bIsCarryingObj && !bHasFlag
				                && m_bEntranceVectorValid && !m_pTeleEntrance && (iMetal >= 125),
				            0.7f);
			}
			else
			{
			ADD_UTILITY(BOT_UTIL_BUILDTELENT,
			            (CTeamFortress2Mod::isMapType(TF_MAP_MVM) || fSentryHealthPercent > 0.99f)
			                && !m_bIsCarryingObj && !bHasFlag
			                && ((m_pSentryGun.get() && (iSentryLevel > 1)) || (m_pSentryGun.get() == nullptr))
			                && m_bEntranceVectorValid && !m_pTeleEntrance && (iMetal >= 125),
			            0.7f);
			ADD_UTILITY(BOT_UTIL_BUILDTELEXT,
			            (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
			                || (!bSentryHasEnemy && fSentryHealthPercent > 0.99f))
			                && !m_bIsCarryingObj && !bHasFlag
			                && m_pSentryGun && (iSentryLevel > 1) && !m_pTeleExit && (iMetal >= 125),
			            randomFloat(0.7, 0.9));
			}

			if ((m_fSpawnTime + 5.0f) > engine->Time())
			{

				if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
				{
					ADD_UTILITY(BOT_UTIL_BUILDTELENT,
					            !m_bIsCarryingObj && !bHasFlag && m_bEntranceVectorValid && !m_pTeleEntrance
					                && (iMetal >= 125),
					            0.95f);
				}
				else
				{
					Vector vOrigin = getOrigin();

					WaypointList *failed;
					m_pNavigator->getFailedGoals(&failed);

					failedlist        = CWaypointLocations::resetFailedWaypoints(failed);

					pWaypointResupply = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
					    vOrigin, 1024.0f, -1, false, false, true, nullptr, false, getTeam(), true, false,
					    Vector(0, 0, 0),
					    CWaypointTypes::
					        W_FL_RESUPPLY)); // CWaypoints::getWaypoint(CWaypoints::getClosestFlagged(CWaypointTypes::W_FL_RESUPPLY,vOrigin,iTeam,&fResupplyDist,failedlist));

					ADD_UTILITY_DATA(BOT_UTIL_BUILDTELENT_SPAWN,
					                 !m_bIsCarryingObj && (pWaypointResupply != nullptr) && !bHasFlag
					                     && !m_pTeleEntrance && (iMetal >= 125),
					                 0.95f, CWaypoints::getWaypointIndex(pWaypointResupply));
				}
			}
		}
		// to do -- split into two
		ADD_UTILITY(
		    BOT_UTIL_UPGSENTRY,
		    !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && !bHasFlag && (m_pSentryGun.get() != nullptr)
		        && !CClassInterface::getTF2BuildingIsMini(m_pSentryGun)
		        && (((iSentryLevel < 3) && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pSentryGun))))
		            || ((fSentryHealthPercent < 1.0f) && (iMetal > 75))
		            || (CClassInterface::getTF2SentryShells(m_pSentryGun) < 50)
		            || (CClassInterface::getSentryEnemy(m_pSentryGun) != nullptr)),
		    0.8 + ((1.0f - fSentryHealthPercent) * 0.2));

		ADD_UTILITY(BOT_UTIL_GETAMMODISP,
		            !m_bIsCarryingObj && m_pDispenser && !CClassInterface::isObjectBeingBuilt(m_pDispenser)
		                && isVisible(m_pDispenser) && (iMetal < 200),
		            fUseDispFactor);

		ADD_UTILITY(BOT_UTIL_UPGTELENT,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pTeleEntrance != nullptr
		                && !CClassInterface::isObjectBeingBuilt(m_pTeleEntrance)
		                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pTeleEntrance)))
		                && (fTeleporterEntranceHealthPercent < 1.0f),
		            ((fEntranceDist < fExitDist)) * 0.51 + (0.5 - (fTeleporterEntranceHealthPercent * 0.5)));
		ADD_UTILITY(BOT_UTIL_UPGTELEXT,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pTeleExit != nullptr
		                && !CClassInterface::isObjectBeingBuilt(m_pTeleExit)
		                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pTeleExit)))
		                && (fTeleporterExitHealthPercent < 1.0f),
		            ((fExitDist < fEntranceDist) * 0.51) + ((0.5 - fTeleporterExitHealthPercent) * 0.5));
		ADD_UTILITY(BOT_UTIL_UPGDISP,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pDispenser != nullptr
		                && !CClassInterface::isObjectBeingBuilt(m_pDispenser)
		                && (iMetal >= (200 - CClassInterface::getTF2SentryUpgradeMetal(m_pDispenser)))
		                && ((iDispenserLevel < 3) || (fDispenserHealthPercent < 1.0f)),
		    0.7 + ((1.0f - fDispenserHealthPercent) * 0.3));

		// MvM: sequential build priority -- fix what's damaged before expanding
		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		{
			bool bHasSentry = (m_pSentryGun.get() != nullptr);
			bool bHasDisp   = (m_pDispenser.get() != nullptr);
			bool bSentryHealthy = bHasSentry && (fSentryHealthPercent > 0.6f)
			                    && (CClassInterface::getSentryEnemy(m_pSentryGun) == nullptr);
			bool bDispHealthy   = bHasDisp && (fDispenserHealthPercent > 0.4f);

			if (!bHasSentry)
			{
				ADD_UTILITY(BOT_UTIL_BUILDSENTRY, !m_bIsCarryingObj && !bHasFlag && !m_pSentryGun
				    && (iMetal >= 130), 0.98f);
			}
			else if (!bSentryHealthy)
			{
				ADD_UTILITY(BOT_UTIL_UPGSENTRY, !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time())
				    && !bHasFlag && bHasSentry, 0.96f);
			}
			else if (!bHasDisp)
			{
				ADD_UTILITY(BOT_UTIL_BUILDDISP, !m_bIsCarryingObj && !bHasFlag && bHasSentry
				    && !m_pDispenser && (iMetal >= 100), 0.94f);
			}
			else if (!bDispHealthy)
			{
				ADD_UTILITY(BOT_UTIL_UPGDISP, !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time())
				    && bHasDisp, 0.92f);
			}
			else if (!m_pTeleEntrance || !m_pTeleExit)
			{
				ADD_UTILITY(BOT_UTIL_BUILDTELENT, !m_bIsCarryingObj && !bHasFlag
				    && m_bEntranceVectorValid && !m_pTeleEntrance && (iMetal >= 125), 0.90f);
				ADD_UTILITY(BOT_UTIL_BUILDTELEXT, !m_bIsCarryingObj && !bHasFlag
				    && bHasSentry && !m_pTeleExit && (iMetal >= 125), 0.90f);
			}
		}

		// remove sappers
		ADD_UTILITY(BOT_UTIL_REMOVE_SENTRY_SAPPER,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && !bHasFlag && (m_pSentryGun != nullptr)
		                && CTeamFortress2Mod::isMySentrySapped(m_pEdict),
		            1000.0f);
		ADD_UTILITY(BOT_UTIL_REMOVE_DISP_SAPPER,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && !bHasFlag && (m_pDispenser != nullptr)
		                && CTeamFortress2Mod::isMyDispenserSapped(m_pEdict),
		            1000.0f);

		ADD_UTILITY_DATA(BOT_UTIL_GOTORESUPPLY_FOR_AMMO,
		                 !bIsUbered && !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedAmmo && !m_pAmmo,
		                 1000.0f / fResupplyDist, CWaypoints::getWaypointIndex(pWaypointResupply));

		ADD_UTILITY_DATA(
		    BOT_UTIL_FIND_NEAREST_AMMO, !m_bIsCarryingObj && !bHasFlag && bNeedAmmo && !m_pAmmo && pWaypointAmmo,
		    (400.0f / fAmmoDist)
		        + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f : 0.0f),
		    CWaypoints::getWaypointIndex(pWaypointAmmo)); // only if close

		ADD_UTILITY(BOT_UTIL_ENGI_LOOK_AFTER_SENTRY,
		            !m_bIsCarryingObj && (m_pSentryGun.get() != nullptr) && (iSentryLevel > 2)
		                && (m_fLookAfterSentryTime < engine->Time()),
		            fGetFlagUtility + 0.01f);

		// remove sappers

		ADD_UTILITY(BOT_UTIL_REMOVE_TMTELE_SAPPER,
		            !m_bIsCarryingObj && (m_fRemoveSapTime < engine->Time()) && m_pNearestTeleEntrance
		                && CTeamFortress2Mod::isTeleporterSapped(m_pNearestTeleEntrance),
		            1.1f);

		// booooo
	}
	else
	{
		pMedigun = CTeamFortress2Mod::getMediGun(m_pEdict);

		if (pMedigun != nullptr)
			pBWMediGun = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MEDIGUN));

		if (!m_pNearestDisp)
			m_pNearestDisp = CTeamFortress2Mod::nearestDispenser(getOrigin(), iTeam);

		if ((m_iClass != TF_CLASS_SPY) && (m_iClass != TF_CLASS_MEDIC) && (m_iClass != TF_CLASS_SNIPER))
		{
			// squads // follow leader // follow_leader
			ADD_UTILITY(BOT_UTIL_FOLLOW_SQUAD_LEADER,
			            !hasSomeConditions(CONDITION_DEFENSIVE) && hasSomeConditions(CONDITION_SQUAD_LEADER_INRANGE)
			                && inSquad() && (m_pSquad->GetLeader() != m_pEdict)
			                && hasSomeConditions(CONDITION_SEE_SQUAD_LEADER),
			            hasSomeConditions(CONDITION_SQUAD_IDLE) ? 0.1f : 2.0f);
			ADD_UTILITY(BOT_UTIL_FIND_SQUAD_LEADER,
			            !hasSomeConditions(CONDITION_DEFENSIVE) && inSquad() && (m_pSquad->GetLeader() != m_pEdict)
			                && (!hasSomeConditions(CONDITION_SEE_SQUAD_LEADER)
			                    || !hasSomeConditions(CONDITION_SQUAD_LEADER_INRANGE)),
			            hasSomeConditions(CONDITION_SQUAD_IDLE) ? 0.1f : 2.0f);
		}

		ADD_UTILITY_DATA(BOT_UTIL_GOTORESUPPLY_FOR_AMMO,
		                 !bIsUbered && !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedAmmo && !m_pAmmo,
		                 1000.0f / fResupplyDist, CWaypoints::getWaypointIndex(pWaypointResupply));
		ADD_UTILITY_DATA(
		    BOT_UTIL_FIND_NEAREST_AMMO, !bHasFlag && bNeedAmmo && !m_pAmmo && pWaypointAmmo,
		    (400.0f / fAmmoDist)
		        + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f : 0.0f),
		    CWaypoints::getWaypointIndex(pWaypointAmmo));

		if (m_pNearestDisp)
			ADD_UTILITY(BOT_UTIL_GOTODISP,
			            m_pNearestDisp && !CClassInterface::isObjectBeingBuilt(m_pNearestDisp)
			                && !CTeamFortress2Mod::isDispenserSapped(m_pNearestDisp) && (bNeedAmmo || bNeedHealth),
			            (1000.0f / distanceFrom(m_pNearestDisp))
			                + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			                       ? 0.5f
			                       : 0.0f));
	}

	fGetFlagUtility = 0.2 + randomFloat(0.0f, 0.2f);

	if (m_iClass == TF_CLASS_SCOUT)
		fGetFlagUtility = 0.6f;
	else if (m_iClass == TF_CLASS_SPY)
		fGetFlagUtility = 0.6f;
	else if (m_iClass == TF_CLASS_MEDIC)
	{
		if (CTeamFortress2Mod::hasRoundStarted())
			fGetFlagUtility = 0.85f - (((float)numplayersonteam) / (gpGlobals->maxClients / 2));
		else
			fGetFlagUtility = 0.1f; // not my priority
	}
	else if (m_iClass == TF_CLASS_ENGINEER)
	{
		fGetFlagUtility = 0.1f; // not my priority

		if (m_bIsCarryingObj)
			fGetFlagUtility = 0.0f;
	}

	fDefendFlagUtility = bot_defrate.GetFloat() / 4;

	if ((m_iClass == TF_CLASS_HWGUY) || (m_iClass == TF_CLASS_DEMOMAN) || (m_iClass == TF_CLASS_SOLDIER)
	    || (m_iClass == TF_CLASS_PYRO))
		fDefendFlagUtility = bot_defrate.GetFloat() - randomFloat(0.0f, fDefendFlagUtility);
	else if (m_iClass == TF_CLASS_MEDIC)
		fDefendFlagUtility = fGetFlagUtility;
	else if (m_iClass == TF_CLASS_SPY)
		fDefendFlagUtility = 0.0f;
	else if (m_iClass == TF_CLASS_ENGINEER)
		fDefendFlagUtility = m_pSentryGun ? 0.05f : 0.15f; // build instead of camp

	if (hasSomeConditions(CONDITION_PUSH) || CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		fGetFlagUtility *= 2;
		fDefendFlagUtility *= 2;
	}

	// recently saw an enemy go near the point
	if (hasSomeConditions(CONDITION_DEFENSIVE) && (m_pLastEnemy.get() != nullptr)
	    && CBotGlobals::entityIsAlive(m_pLastEnemy.get()) && (m_fLastSeeEnemy > 0))
		fDefendFlagUtility = fGetFlagUtility + 0.1f;

	if (m_fLastKnownTeamFlagTime > engine->Time())
	{
		float fRemaining  = m_fLastKnownTeamFlagTime - engine->Time();
		float fEscalation = 1.0f + (fRemaining / 60.0f);
		if (fEscalation > 4.0f) fEscalation = 4.0f;
		fDefendFlagUtility *= fEscalation;
	}

	// Unguarded objective detection: if no bots near flag, boost defense
	{
		CWaypoint *pFlag = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG, getTeam());
		if (pFlag && pFlag->peekTraversalCount() == 0)
			fDefendFlagUtility *= 2.0f;
	}

	// Guard turn-taking: rotate which bots are on guard duty every 10s
	{
		int iActiveSlot = ((int)(engine->Time() / 10.0f)) % 4;
		if (m_iGuardSlot != iActiveSlot && numplayersonteam_alive >= 4)
			fDefendFlagUtility *= 0.4f;
	}

	// Team dominance: shift posture between defense and aggression
	{
		float fDom = CTeamFortress2Mod::getTeamDominance(m_iTeam);
		if (fDom < 0.0f)
		{
			fDefendFlagUtility *= (1.0f - fDom);
			fGetFlagUtility *= (1.0f + fDom);
			if (fGetFlagUtility < 0.05f) fGetFlagUtility = 0.05f;
		}
		else
		{
			fDefendFlagUtility *= (1.0f - fDom * 0.7f);
			if (fDefendFlagUtility < 0.01f) fDefendFlagUtility = 0.01f;
			fGetFlagUtility *= (1.0f + fDom);
		}
	}

	if (m_iClass == TF_CLASS_ENGINEER)
	{
		if (m_bIsCarryingObj)
			fDefendFlagUtility = 0.0f;
	}
	ADD_UTILITY_DATA(BOT_UTIL_GOTORESUPPLY_FOR_HEALTH,
	                 !bIsUbered && !m_bIsCarryingObj && !bHasFlag && pWaypointResupply && bNeedHealth && !m_pHealthkit,
	                 1000.0f / fResupplyDist, CWaypoints::getWaypointIndex(pWaypointResupply));

	// Only grab ammo if no teammate closer to it needs it more
	bool bYieldsAmmo = false;
	if (bNeedAmmo && m_pAmmo)
	{
		Vector vAmmo = CBotGlobals::entityOrigin(m_pAmmo);
		float fMyDist = (getOrigin() - vAmmo).Length();

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t *pT = INDEXENT(i);
			if (!pT || pT == m_pEdict) continue;
			if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
			if (CTeamFortress2Mod::getTeam(pT) != iTeam) continue;

			float fTheirDist = (CBotGlobals::entityOrigin(pT) - vAmmo).Length();
			if (fTheirDist > 300.0f) continue;

			// Yield to engineers not at max metal who are closer
			if (CClassInterface::getTF2Class(pT) == TF_CLASS_ENGINEER
			    && fTheirDist < fMyDist)
			{
				CBot *pB = CBots::getBotPointer(pT);
				if (pB && pB->isTF2() && ((CBotTF2 *)pB)->getMetal() < 200)
					{ bYieldsAmmo = true; break; }
			}
		}
	}

	ADD_UTILITY(
	    BOT_UTIL_GETAMMOKIT, bNeedAmmo && m_pAmmo && !bYieldsAmmo,
	    1.0 + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f : 0.0f));
	ADD_UTILITY(
	    BOT_UTIL_GETHEALTHKIT, bNeedHealth && m_pHealthkit,
	    1.0 + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f : 0.0f));

	ADD_UTILITY(BOT_UTIL_GETFLAG,
	            (CTeamFortress2Mod::isMapType(TF_MAP_CTF)
	             || (CTeamFortress2Mod::isMapType(TF_MAP_SD) && CTeamFortress2Mod::canTeamPickupFlag_SD(iTeam, false)))
	                && !bHasFlag,
	            fGetFlagUtility);
	ADD_UTILITY(BOT_UTIL_GETFLAG_LASTKNOWN,
	            (CTeamFortress2Mod::isMapType(TF_MAP_CTF) || CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	             || (CTeamFortress2Mod::isMapType(TF_MAP_SD) && CTeamFortress2Mod::canTeamPickupFlag_SD(iTeam, true)))
	                && !bHasFlag && (m_fLastKnownFlagTime && (m_fLastKnownFlagTime > engine->Time())),
	            fGetFlagUtility + 0.1);

	// MvM bomb defense: scale with threat (carrier proximity to hatch)
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && !bHasFlag)
	{
		float fMvmDefendUtil = fDefendFlagUtility;
		Vector vFlagLocation;
		Vector vCapturePoint;

		if (CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE, &vFlagLocation)
		    && CTeamFortress2Mod::getMVMCapturePoint(&vCapturePoint))
		{
			edict_t *pCarrier = CTeamFortress2Mod::getFlagCarrier(TF2_TEAM_BLUE);
			float fDistToHatch = (vFlagLocation - vCapturePoint).Length();

			if (pCarrier && CBotGlobals::entityIsAlive(pCarrier))
			{
				// Bomb is being carried -- urgent! Scale inversely with distance to hatch
				if (fDistToHatch < 2048.0f)
					fMvmDefendUtil += 2.0f * (1.0f - (fDistToHatch / 2048.0f)); // +0 to +2.0
				else
					fMvmDefendUtil += 0.2f;
			}
			else if (fDistToHatch > 512.0f)
			{
				// Bomb not being carried, far from hatch -- guard it lightly
				fMvmDefendUtil += 0.0f;
			}
			else
			{
				// Bomb dropped near hatch -- very dangerous
				fMvmDefendUtil += 1.0f;
			}

			// Count how many friendly bots are already defending the flag area
			int iNearbyDefenders = 0;
			Vector vBombArea = vFlagLocation;
			float fCheckDist = 1024.0f;

			for (int i = 1; i <= CBotGlobals::maxClients(); i++)
			{
				edict_t *pOther = INDEXENT(i);
				if (pOther && pOther != m_pEdict && CBotGlobals::entityIsValid(pOther)
				    && CClassInterface::getTeam(pOther) == iTeam)
				{
					CBot *pOtherBot = CBots::getBotPointer(pOther);
					if (pOtherBot && (CBotGlobals::entityOrigin(pOther) - vBombArea).Length() < fCheckDist)
					{
						if (pOtherBot->getSchedule()->isCurrentSchedule(SCHED_DEFEND)
						    || pOtherBot->getSchedule()->isCurrentSchedule(SCHED_DEFENDPOINT))
							iNearbyDefenders++;
						else if (pOtherBot->getSchedule()->hasSchedule(SCHED_RETURN_TO_INTEL))
							iNearbyDefenders++;
					}
				}
			}

		// Avoid overkill: scale down if enough defenders already there
		if (iNearbyDefenders >= 3)
			fMvmDefendUtil = 0.0f;
		else if (iNearbyDefenders >= 2)
			fMvmDefendUtil *= 0.15f;
		else if (iNearbyDefenders >= 1)
			fMvmDefendUtil *= 0.4f;
		}

		if (m_pEnemy && CBotGlobals::entityIsValid(m_pEnemy) && CBotGlobals::entityIsAlive(m_pEnemy)
		    && hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			m_fLastEnemyNearBomb = engine->Time();

		// Preemptive fighting: reduce defend utility when we can actively engage enemies
		if (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			fMvmDefendUtil *= 0.3f;

		if (engine->Time() - m_fLastEnemyNearBomb > 30.0f)
			fMvmDefendUtil *= 0.05f;

		// Organic rotation: if nearby waypoint has activity, reduce guard weight
		{
			int iWpt = CWaypointLocations::NearestWaypoint(getOrigin(), 256.0f, -1);
			if (iWpt >= 0)
			{
				CWaypoint *pWpt = CWaypoints::getWaypoint(iWpt);
				if (pWpt && pWpt->peekTraversalCount() > 2)
					fMvmDefendUtil *= 0.6f;
			}
		}

		ADD_UTILITY(BOT_UTIL_DEFEND_FLAG, true, fMvmDefendUtil);

		// Tank attack utility — tank-effective classes prioritize tanks
		edict_t *pTank = CTeamFortress2Mod::getNearestTank();
		if (pTank && CBotGlobals::entityIsAlive(pTank))
		{
			// Count how many friendly bots are already focused on a tank
			int iTankFocusBots = 0;
			for (int i = 1; i <= CBotGlobals::maxClients(); i++)
			{
				edict_t *pOther = INDEXENT(i);
				if (!pOther || pOther == m_pEdict || pOther->IsFree() || !pOther->GetUnknown()) continue;
				if (!CBotGlobals::entityIsValid(pOther)) continue;
				CBot *pOtherBot = CBots::getBotPointer(pOther);
				if (!pOtherBot) continue;
				edict_t *pTheirEnemy = pOtherBot->getEnemy();
				if (pTheirEnemy && CTeamFortress2Mod::isTankBoss(pTheirEnemy))
					iTankFocusBots++;
			}

			float fTankUtil = 0.0f;
			bool bTankClass  = (m_iClass == TF_CLASS_PYRO || m_iClass == TF_CLASS_SOLDIER
					 || m_iClass == TF_CLASS_DEMOMAN || m_iClass == TF_CLASS_SCOUT
					 || m_iClass == TF_CLASS_HWGUY);

			if (bTankClass)
			{
				float fTankDist = (CBotGlobals::entityOrigin(pTank) - getOrigin()).Length();
				fTankUtil = 3.0f + (400.0f / (fTankDist + 1.0f));
			}
			else
			{
				// Other classes only help if tank is close to hatch
				Vector vHatch;
				if (CTeamFortress2Mod::getMVMCapturePoint(&vHatch))
				{
					float fDistToHatch = (CBotGlobals::entityOrigin(pTank) - vHatch).Length();
					if (fDistToHatch < 1024.0f)
						fTankUtil = 1.0f + (1.0f - (fDistToHatch / 1024.0f));
				}
			}

			// Limit to ~3 bots per tank unless emergency
			if (fTankUtil > 0.0f && iTankFocusBots >= 3)
			{
				Vector vHatch;
				bool bEmergency = false;
				if (CTeamFortress2Mod::getMVMCapturePoint(&vHatch))
					bEmergency = ((CBotGlobals::entityOrigin(pTank) - vHatch).Length() < 1024.0f);
				if (!bEmergency)
					fTankUtil = 0.0f;
			}

			if (fTankUtil > 0.0f)
				ADD_UTILITY(BOT_UTIL_ATTACK_TANK, true, fTankUtil);
		}
	}
	else
	{
		ADD_UTILITY(BOT_UTIL_DEFEND_FLAG,
		            CTeamFortress2Mod::isMapType(TF_MAP_CTF) && !bHasFlag,
		            fDefendFlagUtility);
	}

	ADD_UTILITY(BOT_UTIL_DEFEND_FLAG_LASTKNOWN,
	            !bHasFlag
	                && (CTeamFortress2Mod::isMapType(TF_MAP_CTF) || CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	                    || (CTeamFortress2Mod::isMapType(TF_MAP_SD)
	                        && (CTeamFortress2Mod::getFlagCarrierTeam() == CTeamFortress2Mod::getEnemyTeam(iTeam))))
	                && (m_fLastKnownTeamFlagTime && (m_fLastKnownTeamFlagTime > engine->Time())),
	            CTeamFortress2Mod::isMapType(TF_MAP_MVM)
	                ? (fDefendFlagUtility + 0.5f + (randomFloat(0.0, 0.3f) - 0.15f))
	                : (fDefendFlagUtility + (randomFloat(0.0, 0.2f) - 0.1f)));
	ADD_UTILITY(BOT_UTIL_SNIPE,
	            (iClass == TF_CLASS_SNIPER) && m_pWeapons->getCurrentWeaponInSlot(0)
	                && !m_pWeapons->getCurrentWeaponInSlot(0)->isProjectile()
	                && !m_pWeapons->getCurrentWeaponInSlot(0)->outOfAmmo(this) && !hasSomeConditions(CONDITION_PARANOID)
	                && !bHasFlag && (getHealthPercent() > 0.2f),
	            0.95);
	// ADD_UTILITY(BOT_UTIL_SNIPE_CROSSBOW, (iClass == TF_CLASS_SNIPER) && m_pWeapons->hasWeapon(TF2_WEAPON_BOW) &&
	// !m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_BOW))->outOfAmmo(this) &&
	// !hasSomeConditions(CONDITION_PARANOID) && !bHasFlag && (getHealthPercent()>0.2f), 0.95);

	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.0001f);
	ADD_UTILITY_DATA(
	    BOT_UTIL_FIND_NEAREST_HEALTH, !bHasFlag && bNeedHealth && !m_pHealthkit && pWaypointHealth,
	    (1000.0f / fHealthDist)
	        + ((!CTeamFortress2Mod::hasRoundStarted() && CTeamFortress2Mod::isMapType(TF_MAP_MVM)) ? 0.5f : 0.0f),
	    CWaypoints::getWaypointIndex(pWaypointHealth));

	ADD_UTILITY(BOT_UTIL_FIND_MEDIC_FOR_HEALTH,
	            (m_iClass != TF_CLASS_MEDIC) && !bHasFlag && bNeedHealth && m_pLastSeeMedic.hasSeen(30.0f), 1.0f);

	// Pick a sentry target: visible first, then known, then team-shared
	edict_t *pSentryTarget = m_pNearestEnemySentry.get();

	if (!pSentryTarget && !m_KnownSentries.empty())
	{
		float fBestDist = 9999.0f;
		for (auto &h : m_KnownSentries)
		{
			edict_t *pKnown = h.get();
			if (pKnown && CBotGlobals::entityIsValid(pKnown)
			    && CBotGlobals::entityIsAlive(pKnown))
			{
				float fD = distanceFrom(pKnown);
				if (fD < fBestDist && fD < 2000.0f)
				{
					fBestDist     = fD;
					pSentryTarget = pKnown;
				}
			}
		}
	}

	if (!pSentryTarget)
	{
		for (int i = 0; i < 8; i++)
		{
			if (CTeamFortress2Mod::m_fTeamKnownSentryTimes[i] > engine->Time())
			{
				float fD = (CTeamFortress2Mod::m_vTeamKnownSentryPositions[i] - getOrigin()).Length();
				if (fD < 2000.0f)
				{
					// Team has intel on a sentry nearby -- use position as target
					// (handleAttack will check visibility when it reaches the area)
				}
			}
		}
	}

	if ((pSentryTarget != nullptr) && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		CBotWeapon *pWeapon = m_pWeapons->getPrimaryWeapon();

		// Count teammates near the sentry for coordination
		int iNearbyTeam = 0;
		edict_t *pCountTarget = pSentryTarget;
		for (int t = 1; t <= gpGlobals->maxClients; t++)
		{
			edict_t *pT = INDEXENT(t);
			if (!pT || pT == m_pEdict) continue;
			if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
			if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
			if ((CBotGlobals::entityOrigin(pT)
			     - CBotGlobals::entityOrigin(pCountTarget)).Length() < 512.0f)
				iNearbyTeam++;
		}

		float fSentryUtil = 0.7f + iNearbyTeam * 0.1f;
		if (iNearbyTeam == 0) fSentryUtil *= 0.5f;

		// Critical override: if sentry is near our objective path, boost utility
		if (fSentryUtil < 0.6f)
		{
			Vector vObj;
			if (CTeamFortress2Mod::getFlagLocation(m_iTeam, &vObj)
			    || CTeamFortress2Mod::getMVMCapturePoint(&vObj))
			{
				float fSentryToObj = (CBotGlobals::entityOrigin(pSentryTarget) - vObj).Length();
				if (fSentryToObj < TF2_MAX_SENTRYGUN_RANGE)
					fSentryUtil = 0.65f;
			}
		}

		// Two-tier weapon range check: long-range (1056+) or short-range (256+, not melee)
		bool bCanLongRange = pWeapon && !pWeapon->outOfAmmo(this)
		    && pWeapon->primaryGreaterThanRange(TF2_MAX_SENTRYGUN_RANGE + 32.0f);
		bool bCanShortRange = pWeapon && !pWeapon->outOfAmmo(this)
		    && !pWeapon->isMelee() && pWeapon->primaryGreaterThanRange(256.0f);
		bool bCanAttack = (m_iClass != TF_CLASS_SPY) && (bCanLongRange || bCanShortRange);
		float fRangeFactor = bCanLongRange ? 1.0f : 0.6f;

		ADD_UTILITY_DATA(BOT_UTIL_ATTACK_SENTRY, bCanAttack, fSentryUtil * fRangeFactor,
		                 ENTINDEX(pSentryTarget));

		// Nest destruction: attack known enemy teleporters near our vital points
		if (!m_KnownEnemyTeleporters.empty())
		{
			Vector vFlag = getOrigin();
			if (!CTeamFortress2Mod::getFlagLocation(m_iTeam, &vFlag))
				CTeamFortress2Mod::getMVMCapturePoint(&vFlag);
			float fBestDist = 9999.0f;
			int iBest       = -1;
			for (size_t i = 0; i < m_KnownEnemyTeleporters.size(); i++)
			{
				edict_t *pTele = m_KnownEnemyTeleporters[i].get();
				if (!pTele || !CBotGlobals::entityIsValid(pTele) || !CBotGlobals::entityIsAlive(pTele))
					continue;
				float fD = (CBotGlobals::entityOrigin(pTele) - vFlag).Length();
				if (fD < fBestDist) { fBestDist = fD; iBest = (int)i; }
			}
			if (iBest >= 0 && fBestDist < 2000.0f)
			{
				float fProximityBoost = 2000.0f / (2000.0f + fBestDist);
				float fClassMult      = 1.0f;
				if (m_iClass == TF_CLASS_HWGUY || m_iClass == TF_CLASS_SOLDIER
				    || m_iClass == TF_CLASS_DEMOMAN)
					fClassMult = 1.5f;
				else if (m_iClass == TF_CLASS_PYRO) fClassMult = 1.2f;
				else if (m_iClass == TF_CLASS_SCOUT || m_iClass == TF_CLASS_SNIPER
				         || m_iClass == TF_CLASS_MEDIC) fClassMult = 0.5f;

				float fUtil = fProximityBoost * fClassMult * 0.6f;
				if (m_iClass != TF_CLASS_SPY && pWeapon && !pWeapon->outOfAmmo(this))
					ADD_UTILITY_DATA(BOT_UTIL_DESTROY_NEST, true, fUtil,
					                 ENTINDEX(m_KnownEnemyTeleporters[iBest].get()));
			}
		}
	}
	// only attack if attack area is > 0
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT,
	            !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) && (m_fAttackPointTime < engine->Time())
	                && ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_iCurrentAttackArea > 0)
	                && (CTeamFortress2Mod::isMapType(TF_MAP_SD) || CTeamFortress2Mod::isMapType(TF_MAP_CART)
	                    || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)
	                    || (CTeamFortress2Mod::isMapType(TF_MAP_ARENA) && CTeamFortress2Mod::isArenaPointOpen())
	                    || (CTeamFortress2Mod::isMapType(TF_MAP_KOTH) && CTeamFortress2Mod::isArenaPointOpen())
	                    || CTeamFortress2Mod::isMapType(TF_MAP_CP) || CTeamFortress2Mod::isMapType(TF_MAP_TC)),
	            fGetFlagUtility);

	// only defend if defend area is > 0
	// (!CTeamFortress2Mod::isAttackDefendMap()||(m_iTeam==TF2_TEAM_RED))
	// Only a handful of bots should defend a point at once
	bool bAlreadyDefended = false;
	if (m_iCurrentDefendArea > 0)
	{
		int iDefenders = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t *pP = INDEXENT(i);
			if (!pP || pP == m_pEdict) continue;
			if (!CBotGlobals::entityIsValid(pP)) continue;
			if (CTeamFortress2Mod::getTeam(pP) != iTeam) continue;
			if (!CBotGlobals::isPlayer(pP)) continue;
			float fD = distanceFrom(pP);
			if (fD < 256.0f) // another defender nearby
			{
				iDefenders++;
				if (iDefenders >= 3) { bAlreadyDefended = true; break; }
			}
		}
	}

	ADD_UTILITY(BOT_UTIL_DEFEND_POINT,
	            (m_iCurrentDefendArea > 0) && !bAlreadyDefended
	                && (CTeamFortress2Mod::isMapType(TF_MAP_MVM) || CTeamFortress2Mod::isMapType(TF_MAP_SD)
	                    || CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)
	                    || CTeamFortress2Mod::isMapType(TF_MAP_ARENA) || CTeamFortress2Mod::isMapType(TF_MAP_KOTH)
	                    || CTeamFortress2Mod::isMapType(TF_MAP_CP) || CTeamFortress2Mod::isMapType(TF_MAP_TC))
	                && m_iClass != TF_CLASS_SCOUT,
	            fDefendFlagUtility);

	ADD_UTILITY(BOT_UTIL_MEDIC_HEAL,
	            (m_iClass == TF_CLASS_MEDIC) && (pMedigun != nullptr) && pBWMediGun && pBWMediGun->hasWeapon()
	                && m_pHeal && CBotGlobals::entityIsAlive(m_pHeal) && (getHealFactor(m_pHeal) > 0),
	            0.98f);

	ADD_UTILITY(BOT_UTIL_MEDIC_HEAL_LAST,
	            (m_iClass == TF_CLASS_MEDIC) && (pMedigun != nullptr) && pBWMediGun && pBWMediGun->hasWeapon()
	                && m_pLastHeal && CBotGlobals::entityIsAlive(m_pLastHeal)
	                && (getHealFactor(m_pLastHeal) > 0),
	            0.99f);

	ADD_UTILITY(BOT_UTIL_HIDE_FROM_ENEMY,
	            !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) && (m_pEnemy.get() != nullptr)
	                && hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && !hasFlag()
	                && !CTeamFortress2Mod::isFlagCarrier(m_pEnemy)
	                && (((m_iClass == TF_CLASS_SPY) && (!isDisguised() && !isCloaked()))
	                    || (((m_iClass == TF_CLASS_MEDIC) && (pMedigun != nullptr) && !m_pHeal)
	                        || CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEnemy))),
	            1.0f);

	ADD_UTILITY(
	    BOT_UTIL_HIDE_FROM_ENEMY,
	    (m_pEnemy.get() != nullptr) && hasSomeConditions(CONDITION_SEE_CUR_ENEMY)
	        && (CTeamFortress2Mod::isLosingTeam(m_iTeam) || ((m_iClass == TF_CLASS_ENGINEER) && m_bIsCarryingObj)),
	    1.0f);

	ADD_UTILITY(
	    BOT_UTIL_MEDIC_FINDPLAYER,
	    (m_iClass == TF_CLASS_MEDIC) && !m_pHeal && m_pLastCalledMedic && (pMedigun != nullptr) && pBWMediGun
	        && pBWMediGun->hasWeapon() && ((m_fLastCalledMedicTime + 30.0f) > engine->Time())
	        && ((numplayersonteam > 1) && (numplayersonteam > CTeamFortress2Mod::numClassOnTeam(iTeam, getClass()))),
	    0.95f);

	ADD_UTILITY(BOT_UTIL_MEDIC_FINDPLAYER_AT_SPAWN,
	            (m_iClass == TF_CLASS_MEDIC) && !m_pHeal && !m_pLastCalledMedic && (pMedigun != nullptr) && pBWMediGun
	                && pBWMediGun->hasWeapon() && m_bEntranceVectorValid && (numplayersonteam > 1)
	                && ((!CTeamFortress2Mod::isAttackDefendMap() && !CTeamFortress2Mod::hasRoundStarted())
	                    || (numplayersonteam_alive < numplayersonteam)),
	            0.94f);

	// MvM: collect dropped cash
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && !hasFlag()
	    && ((m_iClass == TF_CLASS_SCOUT)
	        || (m_pEnemy.get() == nullptr) || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY)))
	{
		float fSearchRange = (m_iClass == TF_CLASS_SCOUT) ? 2048.0f : 1024.0f;
		edict_t *pNearestCash = CClassInterface::FindEntityByClassnameNearest(
		    getOrigin(), "item_currencypack_custom", fSearchRange);

		if (pNearestCash && CBotGlobals::entityIsAlive(pNearestCash))
		{
			float fCashDist = distanceFrom(pNearestCash);
			float fUtil;
			if (m_iClass == TF_CLASS_SCOUT)
				fUtil = (800.0f / fCashDist) + 1.5f; // scouts strongly prioritize cash
			else
				fUtil = (400.0f / fCashDist) + 0.5f;

			ADD_UTILITY_WEAPON_DATA_VECTOR(BOT_UTIL_MVM_COLLECT_CASH, pNearestCash != nullptr,
			                               fUtil, nullptr, 0,
			                               CBotGlobals::entityOrigin(pNearestCash));
		}
	}

	// MvM: medic revive marker prioritization
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && (m_iClass == TF_CLASS_MEDIC) && !hasFlag())
	{
		bool bNoThreat = (!m_pEnemy || !hasSomeConditions(CONDITION_SEE_CUR_ENEMY) || !wantToShoot());

		// Only pursue markers if no threat, or if no alive players are nearby to heal
		bool bShouldRevive = bNoThreat;
		if (!bNoThreat)
		{
			bShouldRevive = true;
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				edict_t *pT = INDEXENT(i);
				if (!pT || pT == m_pEdict) continue;
				if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
				if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
				if (CClassInterface::getTF2Class(pT) == TF_CLASS_MEDIC) continue;
				if (distanceFrom(pT) < 800.0f)
				{
					bShouldRevive = false;
					break;
				}
			}
		}

		if (bShouldRevive)
		{
			edict_t *pReviveMarker = CClassInterface::FindEntityByClassnameNearest(
			    getOrigin(), "entity_revive_marker", 2048.0f);

			if (pReviveMarker && CBotGlobals::entityIsAlive(pReviveMarker))
			{
				float fMyDist = distanceFrom(pReviveMarker);
				bool bIAmClosest = true;

				for (int i = 1; i <= CBotGlobals::maxClients(); i++)
				{
					edict_t *pOther = INDEXENT(i);
					if (pOther && pOther != m_pEdict && CBotGlobals::entityIsValid(pOther)
					    && CClassInterface::getTeam(pOther) == iTeam
					    && CClassInterface::getTF2Class(pOther) == TF_CLASS_MEDIC
					    && CBotGlobals::entityIsAlive(pOther))
					{
						CBot *pOtherBot = CBots::getBotPointer(pOther);
						if (pOtherBot && ((CBotTF2 *)pOtherBot)->getHealingEntity())
							continue;

						if ((CBotGlobals::entityOrigin(pOther) - CBotGlobals::entityOrigin(pReviveMarker)).Length()
						    < fMyDist)
						{
							bIAmClosest = false;
							break;
						}
					}
				}

				if (bIAmClosest)
				{
					m_pHeal = pReviveMarker;
					setVisible(pReviveMarker, true);
					updateCondition(CONDITION_SEE_HEAL);
					float fUtil = bNoThreat ? 0.985f : 0.7f;
					ADD_UTILITY(BOT_UTIL_MVM_MEDIC_REVIVE,
					            (getHealFactor(m_pHeal) > 0) && (pMedigun != nullptr) && pBWMediGun
					                && pBWMediGun->hasWeapon(),
					            fUtil);
				}
			}
		}
	}

	if ((m_iClass == TF_CLASS_DEMOMAN) && !hasEnemy() && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		CBotWeapon *pPipe = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
		CBotWeapon *pGren = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));

		if (pPipe && pPipe->hasWeapon() && !pPipe->outOfAmmo(this) && (m_iTrapType != TF_TRAP_TYPE_ENEMY))
		{
			ADD_UTILITY_WEAPON(BOT_UTIL_PIPE_LAST_ENEMY,
			                   (m_pLastEnemy != nullptr) && (distanceFrom(m_pLastEnemy) > (BLAST_RADIUS)), 0.8f, pPipe);
			ADD_UTILITY_WEAPON(BOT_UTIL_PIPE_NEAREST_SENTRY,
			                   (m_pNearestEnemySentry != nullptr)
			                       && (distanceFrom(m_pNearestEnemySentry) > (BLAST_RADIUS)),
			                   0.81f, pPipe);
			ADD_UTILITY_WEAPON(BOT_UTIL_PIPE_LAST_ENEMY_SENTRY,
			                   (m_pLastEnemySentry != nullptr) && (distanceFrom(m_pLastEnemySentry) > (BLAST_RADIUS)),
			                   0.82f, pPipe);
		}
		else if (pGren && pGren->hasWeapon() && !pGren->outOfAmmo(this))
		{
			ADD_UTILITY_WEAPON(BOT_UTIL_SPAM_LAST_ENEMY,
			                   (m_pLastEnemy != nullptr) && (distanceFrom(m_pLastEnemy) > (BLAST_RADIUS)), 0.78f,
			                   pGren);
			ADD_UTILITY_WEAPON(BOT_UTIL_SPAM_NEAREST_SENTRY,
			                   (m_pNearestEnemySentry != nullptr)
			                       && (distanceFrom(m_pNearestEnemySentry) > (BLAST_RADIUS)),
			                   0.79f, pGren);
			ADD_UTILITY_WEAPON(BOT_UTIL_SPAM_LAST_ENEMY_SENTRY,
			                   (m_pLastEnemySentry != nullptr) && (distanceFrom(m_pLastEnemySentry) > (BLAST_RADIUS)),
			                   0.80f, pGren);
		}
	}

	if (m_iClass == TF_CLASS_SPY)
	{
		ADD_UTILITY(BOT_UTIL_BACKSTAB,
		            !hasFlag() && (!m_pNearestEnemySentry || (CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry))
		                || (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
		                    && CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict)))
		                && (m_fBackstabTime < engine->Time()) && (m_iClass == TF_CLASS_SPY)
		                && ((m_pEnemy && CBotGlobals::isAlivePlayer(m_pEnemy))
		                    || (m_pLastEnemy && CBotGlobals::isAlivePlayer(m_pLastEnemy))),
		            fGetFlagUtility + (getHealthPercent() / 10)
		                + (CTeamFortress2Mod::isMapType(TF_MAP_MVM)
		                    && CTeamFortress2Mod::TF2_IsPlayerOnFire(m_pEdict) ? 2.0f : 0.0f)
		                + (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && m_pEnemy
		                    ? MvmTargetPriority(m_pEnemy) : 0.0f));

		ADD_UTILITY(BOT_UTIL_SAP_ENEMY_SENTRY,
		            m_pEnemy && CTeamFortress2Mod::isSentry(m_pEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isSentrySapped(m_pEnemy),
		            fGetFlagUtility + (getHealthPercent() / 5));

		ADD_UTILITY(BOT_UTIL_SAP_NEAREST_SENTRY,
		            m_pNearestEnemySentry && !CTeamFortress2Mod::isSentrySapped(m_pNearestEnemySentry),
		            fGetFlagUtility + (getHealthPercent() / 5));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_SENTRY,
		            m_pLastEnemy && CTeamFortress2Mod::isSentry(m_pLastEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isSentrySapped(m_pLastEnemy),
		            fGetFlagUtility + (getHealthPercent() / 5));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_SENTRY, m_pLastEnemySentry.get() != nullptr,
		            fGetFlagUtility + (getHealthPercent() / 5));
		////////////////
		// sap tele
		ADD_UTILITY(BOT_UTIL_SAP_ENEMY_TELE,
		            m_pEnemy && CTeamFortress2Mod::isTeleporter(m_pEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isTeleporterSapped(m_pEnemy),
		            fGetFlagUtility + (getHealthPercent() / 6));

		ADD_UTILITY(BOT_UTIL_SAP_NEAREST_TELE,
		            m_pNearestEnemyTeleporter && !CTeamFortress2Mod::isTeleporterSapped(m_pNearestEnemyTeleporter),
		            fGetFlagUtility + (getHealthPercent() / 6));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_TELE,
		            m_pLastEnemy
		                && CTeamFortress2Mod::isTeleporter(m_pLastEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isTeleporterSapped(m_pLastEnemy),
		            fGetFlagUtility + (getHealthPercent() / 6));
		////////////////
		// sap dispenser
		ADD_UTILITY(BOT_UTIL_SAP_ENEMY_DISP,
		            m_pEnemy && CTeamFortress2Mod::isDispenser(m_pEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isDispenserSapped(m_pEnemy),
		            fGetFlagUtility + (getHealthPercent() / 7));

		ADD_UTILITY(BOT_UTIL_SAP_NEAREST_DISP,
		            m_pNearestEnemyDisp && !CTeamFortress2Mod::isDispenserSapped(m_pNearestEnemyDisp),
		            fGetFlagUtility + (getHealthPercent() / 7));

		ADD_UTILITY(BOT_UTIL_SAP_LASTENEMY_DISP,
		            m_pLastEnemy && CTeamFortress2Mod::isDispenser(m_pLastEnemy, CTeamFortress2Mod::getEnemyTeam(iTeam))
		                && !CTeamFortress2Mod::isDispenserSapped(m_pLastEnemy),
		            fGetFlagUtility + (getHealthPercent() / 7));

		// Spy infiltrate: move deep into enemy territory
		if (m_fSpyInfiltrateTime < engine->Time()
		    && isDisguised() && !hasEnemy()
		    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING)
		    && !m_pSchedules->isCurrentSchedule(SCHED_BACKSTAB)
		    && CClassInterface::getTF2SpyCloakMeter(m_pEdict) > 50.0f)
		{
			ADD_UTILITY(BOT_UTIL_SPY_INFILTRATE, true, 0.7f);
		}

		// Spy lurk: wait in enemy backline, watch for opportunities
		if (m_bSpyLurking || (!hasEnemy() && isDisguised() && !isCloaked()
		    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING)
		    && !m_pSchedules->isCurrentSchedule(SCHED_BACKSTAB)))
		{
			ADD_UTILITY(BOT_UTIL_SPY_LURK, true, m_bSpyLurking ? 0.7f : 0.55f);
		}
	}

	// MvM: sap robots (one spy per target via entindex stagger)
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && m_iClass == TF_CLASS_SPY
	    && m_fSpySapTime < engine->Time() && !hasFlag()
	    && !m_pSchedules->hasSchedule(SCHED_SPY_SAP_BUILDING) && m_bHijacked)
	{
		// Prune dead entries from sapped robot list
		for (size_t i = 0; i < m_SappedRobots.size();)
		{
			edict_t *pRob = m_SappedRobots[i].get();
			if (!pRob || !CBotGlobals::entityIsValid(pRob) || !CBotGlobals::entityIsAlive(pRob))
				m_SappedRobots.erase(m_SappedRobots.begin() + i);
			else
				i++;
		}

		// Collect all valid unsapped robots within range
		edict_t *pRobots[64];
		int iRobotCount = 0;
		for (int i = 1; i <= gpGlobals->maxClients && iRobotCount < 64; i++)
		{
			edict_t *pEnt = INDEXENT(i);
			if (!pEnt || pEnt->IsFree()) continue;
			if (!CBotGlobals::entityIsValid(pEnt) || !CBotGlobals::entityIsAlive(pEnt)) continue;
			// MvM robots are fake clients on BLU team
			IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo(pEnt);
			if (!pInfo || !pInfo->IsFakeClient()) continue;
			if (CTeamFortress2Mod::getTeam(pEnt) != TF2_TEAM_BLUE) continue;
			if (distanceFrom(pEnt) > 1024.0f) continue;

			bool bSapped = false;
			for (auto &h : m_SappedRobots)
				if (h.get() == pEnt) { bSapped = true; break; }
			if (bSapped) continue;

			pRobots[iRobotCount++] = pEnt;
		}

		if (iRobotCount > 0)
		{
			// Prioritize medics, giants, and sentry busters via model scan
			int iMyPick = ENTINDEX(m_pEdict) % iRobotCount;
			bool bIsMedic    = false;
			bool bIsGiant    = false;
			bool bIsBuster   = false;
			edict_t *pPicked = pRobots[iMyPick];

			// Check if picked robot is a high-value target
			if (pPicked && pPicked->GetCollideable()
			    && pPicked->GetCollideable()->OBBMaxs().Length() > 80.0f)
				bIsGiant = true;

			// Check for sentry buster by model
			IServerEntity *pServ = pPicked ? pPicked->GetIServerEntity() : nullptr;
			if (pServ)
			{
				const char *szModel = pServ->GetModelName().ToCStr();
				if (szModel && strstr(szModel, "sentry_buster"))
					bIsBuster = true;
			}

			// Check for medic robot
			IPlayerInfo *pPInfo = pPicked ? playerinfomanager->GetPlayerInfo(pPicked) : nullptr;
			if (pPInfo && CClassInterface::getTF2Class(pPicked) == TF_CLASS_MEDIC)
				bIsMedic = true;

			float fUtil = bIsBuster ? 0.95f : (bIsGiant ? 1.5f : (bIsMedic ? 1.2f : 0.7f));
			ADD_UTILITY_DATA(BOT_UTIL_SAP_MVM_ROBOT, true, fUtil, ENTINDEX(pPicked));
		}
	}

	// fGetFlagUtility = 0.2+randomFloat(0.0f,0.2f);

	if (CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE))
	{
		if (iTeam == TF2_TEAM_BLUE)
		{
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pDefendPayloadBomb != nullptr),
			            fDefendFlagUtility + randomFloat(-0.1, 0.2));
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pPushPayloadBomb != nullptr),
			            fGetFlagUtility + randomFloat(-0.1, 0.2));
		}
		else
		{
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pDefendPayloadBomb != nullptr),
			            fDefendFlagUtility + randomFloat(-0.1, 0.2));
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pPushPayloadBomb != nullptr),
			            fGetFlagUtility + randomFloat(-0.1, 0.2));
		}
	}
	else if (CTeamFortress2Mod::isMapType(TF_MAP_CART))
	{
		if (iTeam == TF2_TEAM_BLUE)
		{
			ADD_UTILITY(BOT_UTIL_PUSH_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pPushPayloadBomb != nullptr),
			            fGetFlagUtility + (hasSomeConditions(CONDITION_PUSH) ? 0.25f : randomFloat(-0.1f, 0.2f)));
			// Goto Payload bomb
		}
		else
		{
			// Defend Payload bomb
			ADD_UTILITY(BOT_UTIL_DEFEND_PAYLOAD_BOMB,
			            ((m_iClass != TF_CLASS_SPY) || !isDisguised()) && (m_pDefendPayloadBomb != nullptr),
			            fDefendFlagUtility + (hasSomeConditions(CONDITION_PUSH) ? 0.25f : randomFloat(-0.1f, 0.2f)));
		}
	}

	if ((m_iClass == TF_CLASS_DEMOMAN) && (m_iTrapType == TF_TRAP_TYPE_NONE) && canDeployStickies())
	{
		ADD_UTILITY(
		    BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY, m_pLastEnemy && (m_iTrapType == TF_TRAP_TYPE_NONE),
		    randomFloat(std::min(fDefendFlagUtility, fGetFlagUtility), std::max(fDefendFlagUtility, fGetFlagUtility)));

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_FLAG,
		            CTeamFortress2Mod::isMapType(TF_MAP_CTF) && !bHasFlag
		                && (!m_fLastKnownTeamFlagTime || (m_fLastKnownTeamFlagTime < engine->Time())),
		            fDefendFlagUtility + 0.3f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN,
		            (CTeamFortress2Mod::isMapType(TF_MAP_MVM) || CTeamFortress2Mod::isMapType(TF_MAP_CTF)
		             || (CTeamFortress2Mod::isMapType(TF_MAP_SD)
		                 && (CTeamFortress2Mod::getFlagCarrierTeam() == CTeamFortress2Mod::getEnemyTeam(iTeam))))
		                && !bHasFlag && (m_fLastKnownTeamFlagTime && (m_fLastKnownTeamFlagTime > engine->Time())),
		            fDefendFlagUtility + 0.4f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_POINT,
		            (iTeam == TF2_TEAM_RED) && (m_iCurrentDefendArea > 0)
		                && (CTeamFortress2Mod::isMapType(TF_MAP_MVM) || CTeamFortress2Mod::isMapType(TF_MAP_SD)
		                    || CTeamFortress2Mod::isMapType(TF_MAP_CART)
		                    || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)
		                    || CTeamFortress2Mod::isMapType(TF_MAP_ARENA) || CTeamFortress2Mod::isMapType(TF_MAP_KOTH)
		                    || CTeamFortress2Mod::isMapType(TF_MAP_CP) || CTeamFortress2Mod::isMapType(TF_MAP_TC)),
		            fDefendFlagUtility + 0.4f);

		ADD_UTILITY(BOT_UTIL_DEMO_STICKYTRAP_PL,
		            (CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE))
		                && (m_pDefendPayloadBomb != nullptr),
		            fDefendFlagUtility + 0.4f);
	}
	// if ( !CTeamFortress2Mod::hasRoundStarted() && (iTeam == TF_TEAM_BLUE) )
	//{
	if (bot_messaround.GetBool())
	{
		float fMessUtil = 0.98f;

		// Medics: only mess around once fully ubered
		bool bMedicOk = true;
		if (getClass() == TF_CLASS_MEDIC)
		{
			edict_t *pMg = CTeamFortress2Mod::getMediGun(m_pEdict);
			bMedicOk    = (pMg && CClassInterface::getUberChargeLevel(pMg) > 99);
			fMessUtil   = 0.95f;
		}

		// Mess around during setup or whenever bots can't shoot (setup phase, round end, etc.)
		bool bInSetup = !CTeamFortress2Mod::hasRoundStarted() || !wantToShoot();

		// In the last 8 seconds of setup, only ~70% of bots keep messing around.
		// The remaining ~30% head toward the gate so they're ready when the round starts.
		float fRemaining = CTeamFortress2Mod::getRoundTime() - engine->Time();
		if (fRemaining > 0.1f && fRemaining < 8.0f && randomFloat(0.0f, 1.0f) > 0.7f)
			bInSetup = false;

		// Bored: during active gameplay, bot is near spawn or on guard duty
		// with no enemy danger for a while and team isn't being dominated
		// Social invite: nearby bot is already messing around -- join in
		bool bBored         = false;
		bool bSocialInvite  = false;

		// Reset social inviter each frame; it gets set below if an invite is detected
		m_pMessAroundInviter = MyEHandle(nullptr);

		if (!bInSetup && CTeamFortress2Mod::hasRoundStarted() && wantToShoot())
		{
			if (!m_pEnemy
			    && CTeamFortress2Mod::getTeamDominance(m_iTeam) > -0.3f)
			{
				// Social detection: if a nearby bot teammate is messing around,
				// join in with a shorter idle-time requirement (5s vs 15s)
				if ((engine->Time() - m_fLastHurtTime) > 5.0f)
				{
					for (int i = 1; i <= CBotGlobals::maxClients(); i++)
					{
						edict_t *pEdict = INDEXENT(i);
						if (pEdict == m_pEdict) continue;
						if (!CBotGlobals::entityIsValid(pEdict)
						    || !CBotGlobals::entityIsAlive(pEdict)) continue;
						if (CClassInterface::getTeam(pEdict) != getTeam()) continue;

						CBot *pOther = CBots::getBotPointer(pEdict);
						if (pOther && pOther->getSchedule()
						    && pOther->getSchedule()->isCurrentSchedule(SCHED_MESSAROUND))
						{
							if (distanceFrom(pEdict) < 400.0f)
							{
								bSocialInvite          = true;
								m_pMessAroundInviter   = pEdict;
								break;
							}
						}

						// Also detect human players who appear to be messing around:
						// crouching nearby or within melee range of this bot
						if (!pOther)
						{
							float fDist = distanceFrom(pEdict);
							// Within melee/spam range -- direct interaction with this bot
							if (fDist < 100.0f)
							{
								bSocialInvite          = true;
								m_pMessAroundInviter   = pEdict;
								break;
							}
							// Crouching nearby -- idle/friendly behavior
							if (fDist < 200.0f
							    && (CClassInterface::getPlayerFlags(pEdict) & FL_DUCKING))
							{
								bSocialInvite          = true;
								m_pMessAroundInviter   = pEdict;
								break;
							}
						}
					}
				}

				if ((engine->Time() - m_fLastHurtTime) > 15.0f || bSocialInvite)
				{
					// Near spawn or on guard/defense duty
					int iWpt = CWaypointLocations::NearestWaypoint(getOrigin(), 256.0f, -1);
					if (iWpt >= 0)
					{
						CWaypoint *pWpt = CWaypoints::getWaypoint(iWpt);
						if (pWpt && (pWpt->getArea() == 0
						    || m_pSchedules->isCurrentSchedule(SCHED_DEFEND)
						    || m_pSchedules->isCurrentSchedule(SCHED_DEFENDPOINT)
						    || m_pSchedules->hasSchedule(SCHED_DEFEND)
						    || m_pSchedules->hasSchedule(SCHED_DEFENDPOINT)))
						{
							bBored = true;
							fMessUtil = bSocialInvite ? 0.85f : 0.6f;
						}
					}
				}
			}
		}

		ADD_UTILITY(BOT_UTIL_MESSAROUND,
		            bMedicOk && (bInSetup || bBored)
		                && ((iTeam == TF2_TEAM_BLUE) || (!CTeamFortress2Mod::isAttackDefendMap())),
		            fMessUtil);
	}
	//}

	/////////////////////////////////////////////////////////
	// Work out utilities
	//////////////////////////////////////////////////////////
	utils.execute();

	while ((next = utils.nextBest()) != nullptr)
	{
		if (!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if (m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if (executeAction(next)) //>getId(),pWaypointResupply,pWaypointHealth,pWaypointAmmo) )
		{
			m_CurrentUtil               = next->getId();
			// avoid trying to do same thing again and again if it fails
			m_fUtilTimes[m_CurrentUtil] = engine->Time() + 0.5f;

			if (CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				int i = 0;

				CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "-------- getTasks(%s) --------", m_szBotName);

				do
				{
					CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "%s = %0.3f", g_szUtils.at(next->getId()),
					                         next->getUtility(), this);
				} while ((++i < 10) && ((next = utils.nextBest()) != nullptr));

				CClients::clientDebugMsg(this, BOT_DEBUG_UTIL, "----END---- getTasks(%s) ----END----", m_szBotName);
			}

			utils.freeMemory();
			return;
		}
	}

	utils.freeMemory();
}

bool CBotTF2::canDeployStickies()
{
	if (m_pEnemy.get() != nullptr)
	{
		if (CBotGlobals::isAlivePlayer(m_pEnemy))
		{
			if (isVisible(m_pEnemy))
				return false;
		}
	}

	// enough ammo???
	CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

	if (pWeapon)
		return (pWeapon->hasWeapon() && pWeapon->getAmmo(this) >= 6);

	return false;
}

#define STICKY_INIT 0
#define STICKY_SELECTWEAP 1
#define STICKY_RELOAD 2
#define STICKY_FACEVECTOR 3
#define IN_RANGE(x, low, high) ((x > low) && (x < high))

// returns true when finished
bool CBotTF2::deployStickies(eDemoTrapType type, Vector vStand, Vector vLocation, Vector vSpread, Vector *vPoint,
                             int *iState, int *iStickyNum, bool *bFail, float *fTime, int wptindex)
{
	CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
	int iPipesLeft      = 0;
	wantToListen(false);
	m_bWantToInvestigateSound = false;

	if (pWeapon)
		iPipesLeft = pWeapon->getAmmo(this);

	if (*iState == STICKY_INIT)
	{
		if (iPipesLeft < 6)
			*iStickyNum = iPipesLeft;
		else
			*iStickyNum = 6;

		*iState = 1;
	}

	if (getCurrentWeapon() != pWeapon)
		selectBotWeapon(pWeapon);
	else
	{
		if (*iState == 1)
		{
			*vPoint = vLocation + Vector(randomFloat(-vSpread.x, vSpread.x), randomFloat(-vSpread.y, vSpread.y), 0);
			*iState = 2;
		}

		if (distanceFrom(vStand) > 70)
			setMoveTo(vStand);
		else
			stopMoving();

		if (*iState == 2)
		{
			setLookVector(*vPoint);
			setLookAtTask(LOOK_VECTOR);

			if ((*fTime < engine->Time()) && (CBotGlobals::yawAngleFromEdict(m_pEdict, *vPoint) < 20))
			{
				float fTrapDist2D = (vStand - *vPoint).Length2D();
				if (fTrapDist2D > 64.0f)
				{
					float fCharge = fTrapDist2D / pWeapon->getPrimaryMaxRange();
					fCharge = fCharge * fCharge * 2.0f;
					if (fCharge > 1.5f) fCharge = 1.5f;
					primaryAttack(true, fCharge);
				}
				else
					primaryAttack();
				*fTime      = engine->Time() + randomFloat(1.0f, 1.5f);
				*iState     = 1;
				*iStickyNum = *iStickyNum - 1;
				m_iTrapType = type;
			}
		}

		if ((*iStickyNum == 0) || (iPipesLeft == 0))
		{
			if (IN_RANGE(wptindex, 1, MAX_CONTROL_POINTS + 1))
				m_iTrapCPIndex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[wptindex];
			else
				m_iTrapCPIndex = -1;
			m_vStickyLocation = vLocation;

			// complete
			return true;
		}
	}

	return false;
}

void CBotTF2::detonateStickies(bool isJumping)
{
	// don't try to blow myself up unless i'm jumping
	if (isJumping || (distanceFrom(m_vStickyLocation) > (BLAST_RADIUS / 2)))
	{
		secondaryAttack();
		m_iTrapType    = TF_TRAP_TYPE_NONE;
		m_iTrapCPIndex = -1;
	}
}

bool CBotTF2::lookAfterBuildings(float *fTime)
{
	CBotWeapon *pWeapon = getCurrentWeapon();

	wantToListen(false);

	setLookAtTask(LOOK_AROUND);

	if (!pWeapon)
		return false;
	else if (pWeapon->getID() != TF2_WEAPON_WRENCH)
	{
		if (!select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)))
			return false;
	}

	if (m_pSentryGun)
	{
		if (m_prevSentryHealth > CClassInterface::getSentryHealth(m_pSentryGun))
			return true;

		m_prevSentryHealth = CClassInterface::getSentryHealth(m_pSentryGun);

		if (distanceFrom(m_pSentryGun) > 100)
			setMoveTo(CBotGlobals::entityOrigin(m_pSentryGun));
		else
		{
			stopMoving();

			duck(true); // crouch too
		}

		lookAtEdict(m_pSentryGun);
		setLookAtTask(LOOK_EDICT); // LOOK_EDICT fix engineers not looking at their sentry

		if (*fTime < engine->Time())
		{
			m_pButtons->tap(IN_ATTACK);
			*fTime = engine->Time() + randomFloat(10.0f, 20.0f);
		}
	}
	/*
	if ( m_pDispenser )
	{
	    if ( m_prevDispHealth > CClassInterface::getDispenserHealth(m_pDispenser) )
	        return true;

	    m_prevDispHealth = CClassInterface::getDispenserHealth(m_pDispenser);
	}

	if ( m_pTeleExit )
	{
	    if ( m_prevTeleExtHealth > CClassInterface::getTeleporterHealth(m_pTeleExit) )
	        return true;

	    m_prevTeleExtHealth = CClassInterface::getTeleporterHealth(m_pTeleExit);
	}

	if ( m_pTeleEntrance )
	{
	    if ( m_prevTeleEntHealth > CClassInterface::getTeleporterHealth(m_pTeleEntrance) )
	        return true;

	    m_prevTeleEntHealth = CClassInterface::getTeleporterHealth(m_pTeleEntrance);
	}*/

	return false;
}

bool CBotTF2::select_CWeapon(CWeapon *pWeapon)
{
	CBotWeapon *pBotWeapon;
	pBotWeapon = m_pWeapons->getWeapon(pWeapon);

	if (pBotWeapon && !pBotWeapon->hasWeapon())
		return false;
	if (pBotWeapon && !pBotWeapon->isMelee() && pBotWeapon->canAttack() && pBotWeapon->outOfAmmo(this))
		return false;

	edict_t *pDesiredWeapon = CWeapons::findWeapon(m_pEdict, pWeapon->getWeaponName());
	if (pDesiredWeapon)
		m_iSelectWeapon = ENTINDEX(pDesiredWeapon);

	return true;
}

bool CBotTF2::selectBotWeapon(CBotWeapon *pBotWeapon)
{
	CWeapon *pSelect = pBotWeapon->getWeaponInfo();

	if (pSelect)
	{
		edict_t *pDesiredWeapon = CWeapons::findWeapon(m_pEdict, pSelect->getWeaponName());
		if (pDesiredWeapon)
		{
			m_iSelectWeapon = ENTINDEX(pDesiredWeapon);
			return true;
		}

		return false;
	}
	else
		failWeaponSelect();

	return false;
}

//
// Execute a given Action
//
bool CBotTF2::executeAction(CBotUtility *util) // eBotAction id, CWaypoint *pWaypointResupply, CWaypoint
                                               // *pWaypointHealth, CWaypoint *pWaypointAmmo )
{
	static CWaypoint *pWaypoint;
	int id;

	id        = util->getId();
	pWaypoint = nullptr;

	switch (id)
	{
	case BOT_UTIL_DEFEND_PAYLOAD_BOMB:
	{
		removeCondition(CONDITION_DEFENSIVE);

		if (m_pDefendPayloadBomb)
		{
			pWaypoint =
			    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT, 0, m_iCurrentDefendArea, true, this);

			if (pWaypoint)
			{
				Vector org1 = pWaypoint->getOrigin();
				Vector org2 = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);

				pWaypoint   = CWaypoints::randomWaypointGoalBetweenArea(
                    CWaypointTypes::W_FL_DEFEND, m_iTeam, m_iCurrentDefendArea, true, this, true, &org1, &org2);

				if (pWaypoint)
				{
					m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
					removeCondition(CONDITION_PUSH);
					return true;
				}
			}
		}

		pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND, getTeam(), m_iCurrentDefendArea, true,
		                                           this, true);

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
			removeCondition(CONDITION_PUSH);
			return true;
		}

		if (m_pDefendPayloadBomb.get() != nullptr)
		{
			m_pSchedules->add(new CBotTF2DefendPayloadBombSched(m_pDefendPayloadBomb));
			removeCondition(CONDITION_PUSH);
			return true;
		}
	}
	break;
	case BOT_UTIL_PUSH_PAYLOAD_BOMB:
	{
		if (m_pPushPayloadBomb.get() != nullptr)
		{
			m_pSchedules->add(new CBotTF2PushPayloadBombSched(m_pPushPayloadBomb));
			removeCondition(CONDITION_PUSH);
			return true;
		}
	}
	break;
	case BOT_UTIL_ENGI_LOOK_AFTER_SENTRY:
	{
		if (m_pSentryGun.get() != nullptr)
		{
			m_pSchedules->add(new CBotTFEngiLookAfterSentry(m_pSentryGun));
			return true;
		}
	}
	break;
	case BOT_UTIL_DEFEND_FLAG:
		// use last known flag position
		{
			CWaypoint *pWaypoint = nullptr;
			float fGuardTime = (m_iClass == TF_CLASS_MEDIC || hasSomeConditions(CONDITION_DEFENSIVE))
			                       ? randomFloat(4.0f, 8.0f)
			                       : 0.0f;

			if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			{
				pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_DEFEND);

				// MvM: guard the bomb area longer, especially if carrier is near
				Vector vFlagLocation;
				Vector vCapturePoint;
				if (CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE, &vFlagLocation)
				    && CTeamFortress2Mod::getMVMCapturePoint(&vCapturePoint))
				{
					float fDistToHatch = (vFlagLocation - vCapturePoint).Length();
					edict_t *pCarrier = CTeamFortress2Mod::getFlagCarrier(TF2_TEAM_BLUE);

					if (pCarrier && CBotGlobals::entityIsAlive(pCarrier))
						fGuardTime = randomFloat(8.0f, 15.0f); // carrier active -- guard
					else if (fDistToHatch < 512.0f)
						fGuardTime = randomFloat(5.0f, 10.0f); // bomb near hatch -- guard briefly
					else
						fGuardTime = randomFloat(2.0f, 5.0f); // bomb far from hatch -- barely guard
				}
				else
				{
					fGuardTime = randomFloat(3.0f, 8.0f);
				}
			}

			if (pWaypoint == nullptr)
			{
				// Prefer guard positions near the thief's last known exit
				if (CTeamFortress2Mod::m_iThiefExitWpt >= 0
				    && (engine->Time() - CTeamFortress2Mod::m_fThiefSeenTime) < 60.0f)
				{
					pWaypoint = CWaypoints::getWaypoint(CTeamFortress2Mod::m_iThiefExitWpt);
				}
			}

			if (pWaypoint == nullptr)
				pWaypoint =
				    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND, getTeam(), 0, false, this, true);

			if (pWaypoint)
			{
				// MVM: shift guard position forward toward enemy approach, not at the bomb
				if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
				{
					Vector vDir = CTeamFortress2Mod::getTeamEnemyApproachDir(m_iTeam);
					if (vDir.Length2D() > 0.1f)
					{
						Vector vForward = CBotGlobals::entityOrigin(m_pEdict)
						    + vDir * 384.0f;
						CWaypoint *pFwd = CWaypoints::getWaypoint(
						    CWaypointLocations::NearestWaypoint(vForward, 512.0f, -1));
						if (pFwd)
							pWaypoint = pFwd;
					}
				}

				// Skip if a teammate is already at this exact spot
				Vector vWpt = pWaypoint->getOrigin();
				bool bTooClose = false;
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					edict_t *pT = INDEXENT(i);
					if (!pT || pT == m_pEdict) continue;
					if (!CBotGlobals::entityIsValid(pT) || !CBotGlobals::entityIsAlive(pT)) continue;
					if (CTeamFortress2Mod::getTeam(pT) != m_iTeam) continue;
					if ((CBotGlobals::entityOrigin(pT) - vWpt).Length2D() < 150.0f)
						{ bTooClose = true; break; }
				}
				if (bTooClose) return false;

				setLookAt(pWaypoint->getOrigin());
				m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin(), fGuardTime));
				removeCondition(CONDITION_DEFENSIVE);
				removeCondition(CONDITION_PUSH);
				return true;
			}
		}
		break;
	case BOT_UTIL_DEFEND_FLAG_LASTKNOWN:
		// find our flag waypoint
		{
			setLookAt(m_vLastKnownTeamFlagPoint);

			m_pSchedules->add(new CBotDefendSched(
			    m_vLastKnownTeamFlagPoint, ((m_iClass == TF_CLASS_MEDIC) || hasSomeConditions(CONDITION_DEFENSIVE))
			                                   ? randomFloat(4.0f, 8.0f)
			                                   : 0.0f));

			removeCondition(CONDITION_DEFENSIVE);
			removeCondition(CONDITION_PUSH);
			return true;
		}
		break;
	case BOT_UTIL_ATTACK_POINT:

		/*pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND,getTeam(),m_iCurrentAttackArea,true);

		if ( pWaypoint )
		{
		    m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin()));
		    return true;
		}*/

		pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT, 0, m_iCurrentAttackArea, true, this);

		if (pWaypoint && pWaypoint->checkReachable())
		{
			CWaypoint *pRoute = nullptr;
			Vector vRoute     = Vector(0, 0, 0);
			bool bUseRoute    = false;
			int iRouteWpt     = -1;
			bool bNest        = false;

			if ((m_fUseRouteTime < engine->Time())
		    || (m_iClass == TF_CLASS_SCOUT) || (m_iClass == TF_CLASS_SPY) || (m_iClass == TF_CLASS_SNIPER))
			{
				// find random route
				pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), getTeam(),
				                                         m_iCurrentAttackArea);

				if (pRoute)
				{
					bUseRoute       = true;
					vRoute          = pRoute->getOrigin();
					m_fUseRouteTime = engine->Time() + ((m_iClass == TF_CLASS_SCOUT || m_iClass == TF_CLASS_SPY
					                                    || m_iClass == TF_CLASS_SNIPER)
					                                       ? randomFloat(5.0f, 10.0f)
					                                       : randomFloat(30.0f, 60.0f));
					iRouteWpt       = CWaypoints::getWaypointIndex(pRoute);

					bNest = ((m_pNavigator->getBelief(iRouteWpt) / MAX_BELIEF) + (1.0f - getHealthPercent()) > 0.75f);
				}
			}

			m_pSchedules->add(new CBotAttackPointSched(pWaypoint->getOrigin(), pWaypoint->getRadius(),
			                                           pWaypoint->getArea(), bUseRoute, vRoute, bNest,
			                                           m_pLastEnemySentry.get()));
			removeCondition(CONDITION_PUSH);
			return true;
		}
		break;
	case BOT_UTIL_DEFEND_POINT:
	{
		float fprob;

		if ((CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE) || CTeamFortress2Mod::isMapType(TF_MAP_CART)))
		{
			if (m_pDefendPayloadBomb != nullptr)
			{
				static const float fSearchDist = 1500.0f;
				Vector vPayloadBomb            = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);
				CWaypoint *pCapturePoint       = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
                    vPayloadBomb, fSearchDist, -1, false, false, true, nullptr, false, 0, true, false, Vector(0, 0, 0),
                    CWaypointTypes::W_FL_CAPPOINT));

				if (pCapturePoint)
				{
					float fDistance = pCapturePoint->distanceFrom(vPayloadBomb);

					if (fDistance == 0)
						fprob = 1.0f;
					else
						fprob = 1.0f - (fDistance / fSearchDist);
				}
				else // no where near the capture point
				{
					fprob = bot_defrate.GetFloat();
				}
			}
			else
			{
				fprob = 0.05f;
			}
		}
		else
		{
			int capindex =
			    CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iCurrentDefendArea];
			int enemyteam = CTeamFortress2Mod::getEnemyTeam(m_iTeam);
			if (CTeamFortress2Mod::m_ObjectiveResource.GetCappingTeam(capindex) == enemyteam)
			{
				if (CTeamFortress2Mod::m_ObjectiveResource.GetNumPlayersInArea(capindex, enemyteam) > 0)
					fprob = 1.0f;
				else
					fprob = 0.9f;
			}
			else
			{

				float fTime = rcbot_tf2_protect_cap_time.GetFloat();
				// chance of going to point
				fprob       = (fTime
                         - (engine->Time()
                            - CTeamFortress2Mod::m_ObjectiveResource.getLastCaptureTime(m_iCurrentDefendArea)))
				      / fTime;

				if (fprob < rcbot_tf2_protect_cap_percent.GetFloat())
					fprob = rcbot_tf2_protect_cap_percent.GetFloat();
			}
		}

		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict) && hasEnemy())
		{
			// move towards enemy if invuln
			pWaypoint = CWaypoints::getWaypoint(
			    CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pEnemy), 1000.0f, -1));
			fprob = 1.0f;
		}
		else
			pWaypoint =
			    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT, 0, m_iCurrentDefendArea, true, this);

		if (!pWaypoint->checkReachable() || (randomFloat(0.0, 1.0f) > fprob))
		{
			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_DEFEND, getTeam(), m_iCurrentDefendArea,
			                                           true, this, false);

			if (pWaypoint)
			{
				if ((m_iClass == TF_CLASS_DEMOMAN) && wantToShoot()
				    && randomInt(0, 1)) //(m_fLastSeeEnemy + 30.0f > engine->Time()) )
				{
					CBotWeapon *pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));

					if (pWeapon && !pWeapon->outOfAmmo(this))
					{
						CBotTF2Spam *spam =
						    new CBotTF2Spam(this, pWaypoint->getOrigin(), pWaypoint->getAimYaw(), pWeapon);

						if (spam->getDistance() > 600)
						{
							CFindPathTask *path    = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));

							CBotSchedule *newSched = new CBotSchedule();

							newSched->passVector(spam->getTarget());

							newSched->addTask(path);
							newSched->addTask(spam);
							return true;
						}
						else
							delete spam;
					}
				}

				m_pSchedules->add(new CBotDefendPointSched(pWaypoint->getOrigin(), pWaypoint->getRadius(), pWaypoint->getArea()));
				removeCondition(CONDITION_PUSH);

				removeCondition(CONDITION_DEFENSIVE);

				return true;
			}
		}

		if (pWaypoint)
		{
			m_pSchedules->add(
			    new CBotDefendPointSched(pWaypoint->getOrigin(), pWaypoint->getRadius(), pWaypoint->getArea()));
			removeCondition(CONDITION_PUSH);
			removeCondition(CONDITION_DEFENSIVE);
			return true;
		}
	}
	break;
	case BOT_UTIL_CAPTURE_FLAG:
		pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT, getTeam());

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			removeCondition(CONDITION_PUSH);
			return true;
		}
		break;
	case BOT_UTIL_ENGI_DESTROY_ENTRANCE: // destroy and rebuild sentry elsewhere
		engineerBuild(ENGI_ENTRANCE, ENGI_DESTROY);
	case BOT_UTIL_BUILDTELENT:
		pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
		    m_vTeleportEntrance, 300, -1, true, false, true, nullptr, false, getTeam(), true));

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotTFEngiBuild(this, ENGI_ENTRANCE, pWaypoint));
			m_iTeleEntranceArea = pWaypoint->getArea();
			return true;
		}

		break;
	case BOT_UTIL_BUILDTELENT_SPAWN:
	{
		Vector vOrigin = getOrigin();

		pWaypoint      = CWaypoints::getWaypoint(
            CWaypoints::nearestWaypointGoal(CWaypointTypes::W_FL_TELE_ENTRANCE, vOrigin, 4096.0f, getTeam()));

		if (pWaypoint)
		{
			CBotTFEngiBuildTask *buildtask = new CBotTFEngiBuildTask(ENGI_ENTRANCE, pWaypoint);

			CBotSchedule *newSched         = new CBotSchedule();

			newSched->addTask(new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint))); // first
			newSched->addTask(buildtask);
			newSched->addTask(new CFindPathTask(util->getIntData()));
			buildtask->oneTryOnly();

			m_pSchedules->add(newSched);
			m_iTeleEntranceArea = pWaypoint->getArea();
			return true;
		}
	}

	break;
	case BOT_UTIL_ATTACK_SENTRY:
	{
		CBotWeapon *pWeapon = m_pWeapons->getPrimaryWeapon();

		edict_t *pSentry    = INDEXENT(util->getIntData());

		m_pSchedules->add(new CBotTF2AttackSentryGun(pSentry, pWeapon));
	}
	break;
	case BOT_UTIL_DESTROY_NEST:
	{
		edict_t *pTele = INDEXENT(util->getIntData());
		if (pTele && CBotGlobals::entityIsValid(pTele) && CBotGlobals::entityIsAlive(pTele))
			m_pSchedules->add(new CBotTF2AttackSentryGun(pTele, m_pWeapons->getPrimaryWeapon()));
	}
	break;
	case BOT_UTIL_ENGI_DESTROY_EXIT: // destroy and rebuild sentry elsewhere
		engineerBuild(ENGI_EXIT, ENGI_DESTROY);
	case BOT_UTIL_BUILDTELEXT:

		if (m_bTeleportExitVectorValid)
		{
			pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
			    m_vTeleportExit, 150, -1, true, false, true, nullptr, false, getTeam(), true, false, Vector(0, 0, 0),
			    CWaypointTypes::W_FL_TELE_EXIT));

			if (CWaypoints::getWaypointIndex(pWaypoint) == m_iLastFailTeleExitWpt)
			{
				pWaypoint                  = nullptr;
				m_bTeleportExitVectorValid = false;
			}
			// no use going back to this waypoint
			else if (pWaypoint && (pWaypoint->getArea() > 0) && (pWaypoint->getArea() != m_iCurrentAttackArea)
			         && (pWaypoint->getArea() != m_iCurrentDefendArea))
			{
				pWaypoint                  = nullptr;
				m_bTeleportExitVectorValid = false;
			}
		}

		if (pWaypoint == nullptr)
		{
			if (CTeamFortress2Mod::isAttackDefendMap())
			{
				if (getTeam() == TF2_TEAM_BLUE)
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(),
					                                           m_iCurrentAttackArea, true, this, false);
				else
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(),
					                                           m_iCurrentDefendArea, true, this, false);
			}

			if (!pWaypoint)
			{
				int area = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;

				pWaypoint =
				    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(), area, true, this, false,
				                                   0, m_iLastFailTeleExitWpt); // CTeamFortress2Mod::getArea());

				if (!pWaypoint)
				{
					pWaypoint =
					    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(), 0, false, this, false,
					                                   0, m_iLastFailTeleExitWpt); // CTeamFortress2Mod::getArea());

					if (!pWaypoint)
						m_iLastFailTeleExitWpt = -1;
				}
			}
		}

		if (pWaypoint)
		{
			// Evaluate all teleporter exit spots for obscurity; prefer hidden ones
			float fBestScore = evaluateTeleExitSpot(pWaypoint);
			int iBestIdx     = CWaypoints::getWaypointIndex(pWaypoint);

			// Scan all W_FL_TELE_EXIT waypoints for a sufficiently better obscure spot
			for (int w = 0; w < CWaypoints::numWaypoints(); w++)
			{
				CWaypoint *pW = CWaypoints::getWaypoint(w);
				if (!pW || !pW->isUsed() || !pW->forTeam(getTeam())
				    || !pW->hasFlag(CWaypointTypes::W_FL_TELE_EXIT))
					continue;
				if (pW->getArea() != pWaypoint->getArea())
					continue;
				float fScore = evaluateTeleExitSpot(pW);
				if (fScore > fBestScore * 1.3f)
				{
					fBestScore = fScore;
					iBestIdx   = w;
				}
			}

			if (iBestIdx != CWaypoints::getWaypointIndex(pWaypoint))
				pWaypoint = CWaypoints::getWaypoint(iBestIdx);

			m_bTeleportExitVectorValid = true;
			m_vTeleportExit            = pWaypoint->getOrigin() + pWaypoint->applyRadius();
			updateCondition(CONDITION_COVERT); // sneak around to get there
			m_pSchedules->add(new CBotTFEngiBuild(this, ENGI_EXIT, pWaypoint));
			m_iTeleExitArea        = pWaypoint->getArea();
			m_iLastFailTeleExitWpt = CWaypoints::getWaypointIndex(pWaypoint);
			return true;
		}

		break;
	case BOT_UTIL_ENGI_DESTROY_SENTRY: // destroy and rebuild sentry elsewhere
		engineerBuild(ENGI_SENTRY, ENGI_DESTROY);
	case BOT_UTIL_BUILDSENTRY:

		pWaypoint = nullptr;

		// did someone destroy my sentry at the last sentry point? -- build it again
		if (m_bSentryGunVectorValid)
		{
			pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
			    m_vSentryGun, 150, m_iLastFailSentryWpt, true, false, true, nullptr, false, getTeam(), true, false,
			    Vector(0, 0, 0), CWaypointTypes::W_FL_SENTRY));

			if (pWaypoint && CTeamFortress2Mod::buildingNearby(m_iTeam, pWaypoint->getOrigin()))
				pWaypoint = nullptr;
			// no use going back to this waypoint
			if (pWaypoint && (pWaypoint->getArea() > 0) && (pWaypoint->getArea() != m_iCurrentAttackArea)
			    && (pWaypoint->getArea() != m_iCurrentDefendArea))
				pWaypoint = nullptr;
		}

		if (pWaypoint == nullptr)
		{
			if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			{
				pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SENTRY);
				/*
				Vector vFlagLocation;

				if ( CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlagLocation) )
				{
				    pWaypoint =
				CWaypoints::randomWaypointGoalNearestArea(CWaypointTypes::W_FL_SENTRY,m_iTeam,0,false,this,true,&vFlagLocation);
				}*/
			}
			else if (CTeamFortress2Mod::isAttackDefendMap())
			{
				if (getTeam() == TF2_TEAM_BLUE)
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(),
					                                           m_iCurrentAttackArea, true, this, false,
					                                           WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
				else
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(),
					                                           m_iCurrentDefendArea, true, this, true,
					                                           WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
			}

			if (!pWaypoint)
			{
				int area  = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(), area, true, this,
				                                           area == m_iCurrentDefendArea, WPT_SEARCH_AVOID_SENTRIES,
				                                           m_iLastFailSentryWpt);

				if (!pWaypoint)
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(), 0, false, this,
					                                           true, WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
				}
			}
		}

		if (pWaypoint)
		{
			// Prefer obscure spots when team is not dominated
			{
				float fBestScore = evaluateBuildSpot(pWaypoint, 1);
				int iBestIdx     = CWaypoints::getWaypointIndex(pWaypoint);
				for (int w = 0; w < CWaypoints::numWaypoints(); w++)
				{
					CWaypoint *pW = CWaypoints::getWaypoint(w);
					if (!pW || !pW->isUsed() || !pW->forTeam(getTeam())
					    || !pW->hasFlag(CWaypointTypes::W_FL_SENTRY))
						continue;
					if (pW->getArea() != pWaypoint->getArea())
						continue;
					float fScore = evaluateBuildSpot(pW, 1);
					if (fScore > fBestScore * 1.3f)
						{ fBestScore = fScore; iBestIdx = w; }
				}
				if (iBestIdx != CWaypoints::getWaypointIndex(pWaypoint))
					pWaypoint = CWaypoints::getWaypoint(iBestIdx);
			}

			m_iLastFailSentryWpt    = CWaypoints::getWaypointIndex(pWaypoint);
			m_vSentryGun            = pWaypoint->getOrigin() + pWaypoint->applyRadius();
			m_bSentryGunVectorValid = true;
			updateCondition(CONDITION_COVERT); // sneak around to get there
			m_pSchedules->add(new CBotTFEngiBuild(this, ENGI_SENTRY, pWaypoint));
			m_iSentryArea = pWaypoint->getArea();
			return true;
		}
		else
			m_iLastFailSentryWpt = -1;
		break;
	case BOT_UTIL_BACKSTAB:
	{
		edict_t *pTarget = nullptr;
		if (m_pEnemy && CBotGlobals::isAlivePlayer(m_pEnemy))
			pTarget = m_pEnemy;
		else if (m_pLastEnemy && CBotGlobals::isAlivePlayer(m_pLastEnemy))
			pTarget = m_pLastEnemy;

		if (pTarget)
		{
			// If target is very low HP, just shoot them with revolver instead
			int iHealth = CClassInterface::getPlayerHealth(pTarget);
			if (iHealth > 0 && iHealth <= 40)
			{
				CBotWeapon *pRevolver = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_REVOLVER));
				if (pRevolver && pRevolver->hasWeapon() && !pRevolver->outOfAmmo(this)
				    && distanceFrom(pTarget) < 800.0f)
				{
					select_CWeapon(pRevolver->getWeaponInfo());
					m_pSchedules->add(new CBotAttackSched(pTarget));
					return true;
				}
			}
			m_pSchedules->add(new CBotBackstabSched(pTarget));
			return true;
		}
	}
	case BOT_UTIL_SPY_REDISGUISE:
	{
		int iTeam = CTeamFortress2Mod::getEnemyTeam(getTeam());
		spyDisguise(iTeam, getSpyDisguiseClass(iTeam));
		return true;
	}
	case BOT_UTIL_SPY_INFILTRATE:
	{
		// Cloak before moving through dangerous territory
		if (!isCloaked() && isDisguised()
		    && CClassInterface::getTF2SpyCloakMeter(m_pEdict) > 50.0f)
		{
			spyCloak();
		}

		// Find a waypoint deep in enemy territory
		Vector vDir = getOrigin();
		int iBestWpt = -1;
		float fBestDist = 0.0f;
		for (int i = 0; i < CWaypoints::numWaypoints(); i++)
		{
			CWaypoint *pWpt = CWaypoints::getWaypoint(i);
			if (!pWpt || !pWpt->isUsed()) continue;
			Vector vWpt = pWpt->getOrigin();
			float fDot = (vWpt - getOrigin()).Dot(getOrigin()); // crude "enemy direction"
			float fDist = (vWpt - getOrigin()).Length();
			if (fDist > fBestDist && fDist < 4000.0f)
			{
				// Prefer waypoints with higher traversal counts
				fBestDist = fDist + pWpt->getTraversalCount() * 10.0f;
				iBestWpt = i;
			}
		}

		if (iBestWpt >= 0)
		{
			CWaypoint *pDst = CWaypoints::getWaypoint(iBestWpt);
			m_pSchedules->add(new CBotGotoOriginSched(pDst->getOrigin()));
			m_fSpyInfiltrateTime = engine->Time() + randomFloat(10.0f, 20.0f);
			return true;
		}

		m_fSpyInfiltrateTime = engine->Time() + randomFloat(5.0f, 10.0f);
		return false;
	}
	case BOT_UTIL_SPY_LURK:
	{
		if (!m_bSpyLurking)
		{
			m_bSpyLurking   = true;
			m_fSpyLurkStart = engine->Time();
			memset(m_iSpySeenClassCount, 0, sizeof(m_iSpySeenClassCount));
		}

		// Blend-in: stay disguised, look natural -- never cloak (that gives the game away)
		if (isDisguised())
		{
			int iDClass, iDTeam, iDIndex, iDHealth;
			CClassInterface::getTF2SpyDisguised(m_pEdict, &iDClass, &iDTeam, &iDIndex, &iDHealth);

			setLookAtTask(LOOK_AROUND); // never stare directly at enemies

			switch (iDClass)
			{
			case TF_CLASS_ENGINEER:
				if (m_pNearestEnemySentry.get())
				{
					setLookVector(CBotGlobals::entityOrigin(m_pNearestEnemySentry));
					setLookAtTask(LOOK_VECTOR);
				}
				break;
			case TF_CLASS_SNIPER:
			case TF_CLASS_HWGUY:
				break;
			case TF_CLASS_SCOUT:
				if (randomInt(0, 4) == 0) jump();
				break;
			case TF_CLASS_MEDIC:
				for (int i = 1; i <= CBotGlobals::maxClients(); i++)
				{
					edict_t *pEd = INDEXENT(i);
					if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
					if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
					float fD = distanceFrom(pEd);
					if (fD < 250.0f && fD > 60.0f)
						setMoveTo(CBotGlobals::entityOrigin(pEd));
				}
				break;
			case TF_CLASS_SOLDIER:
			case TF_CLASS_DEMOMAN:
			case TF_CLASS_PYRO:
			{
				Vector vObj;
				if (CTeamFortress2Mod::getFlagLocation(m_iTeam, &vObj)
				    || CTeamFortress2Mod::getMVMCapturePoint(&vObj))
					setLookVector(vObj);
			}
			break;
			default:
				break;
			}
		}

		// Record enemy class frequencies while lurking
		for (int i = 1; i <= CBotGlobals::maxClients(); i++)
		{
			edict_t *pEd = INDEXENT(i);
			if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
			if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;
			if (distanceFrom(pEd) < 800.0f && isVisible(pEd))
			{
				int iClass = CClassInterface::getTF2Class(pEd);
				if (iClass > 0 && iClass < 10)
					m_iSpySeenClassCount[iClass]++;
			}
		}

		// Opportunistic strike: check for isolated/distracted enemies
		if (engine->Time() - m_fSpyLurkStart > 3.0f)
		{
			for (int i = 1; i <= CBotGlobals::maxClients(); i++)
			{
				edict_t *pEd = INDEXENT(i);
				if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
				if (CTeamFortress2Mod::getTeam(pEd) == m_iTeam) continue;

				float fDist = distanceFrom(pEd);
				int iClass  = CClassInterface::getTF2Class(pEd);
				int iHealth = CClassInterface::getPlayerHealth(pEd);

				bool bIsolated  = true;
				bool bDistracted = false;
				for (int j = 1; j <= CBotGlobals::maxClients(); j++)
				{
					edict_t *pE2 = INDEXENT(j);
					if (!pE2 || pE2 == pEd || !CBotGlobals::entityIsValid(pE2)
					    || !CBotGlobals::entityIsAlive(pE2)) continue;
					if (CTeamFortress2Mod::getTeam(pE2) != CTeamFortress2Mod::getTeam(pEd)) continue;
					float fD2 = (CBotGlobals::entityOrigin(pE2) - CBotGlobals::entityOrigin(pEd)).Length();
					if (fD2 < 500.0f)
					{
						bIsolated = false;
						if (fD2 < 300.0f) bDistracted = true;
					}
				}

				// Engineer near building: stab first, then sap
				if (iClass == TF_CLASS_ENGINEER && m_pNearestEnemySentry.get()
				    && distanceFrom(m_pNearestEnemySentry) < 400.0f
				    && fDist < 250.0f)
				{
					m_bSpyLurking = false;
					m_pSchedules->add(new CBotBackstabSched(pEd));
					return true;
				}

				// Isolated enemy or distracted group: strike
				if ((bIsolated && fDist < 400.0f)
				    || (bDistracted && fDist < 300.0f))
				{
					// Prioritize high-value targets
					bool bHighValue = (iClass == TF_CLASS_MEDIC || iClass == TF_CLASS_HWGUY
					                   || (iHealth > 0 && iHealth <= 50));
					if (bHighValue || randomInt(0, 2) == 0)
					{
						m_bSpyLurking = false;
						m_pSchedules->add(new CBotBackstabSched(pEd));
						return true;
					}
				}
			}
		}

		// Lurk timeout: 10-20 seconds
		float fLurkLimit = 10.0f + (ENTINDEX(m_pEdict) % 10) * 2.0f;
		if (engine->Time() - m_fSpyLurkStart > fLurkLimit)
		{
			m_bSpyLurking = false;
		}

		return true; // keep lurking
	}
	case BOT_UTIL_REMOVE_TMTELE_SAPPER:
		updateCondition(CONDITION_PARANOID);
		m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestTeleEntrance, ENGI_TELE));
		return true;

	case BOT_UTIL_REMOVE_SENTRY_SAPPER:
		updateCondition(CONDITION_PARANOID);
		m_pSchedules->add(new CBotRemoveSapperSched(m_pSentryGun, ENGI_SENTRY));
		return true;

	case BOT_UTIL_REMOVE_DISP_SAPPER:
		updateCondition(CONDITION_PARANOID);
		m_pSchedules->add(new CBotRemoveSapperSched(m_pDispenser, ENGI_DISP));
		return true;

	case BOT_UTIL_REMOVE_TMSENTRY_SAPPER:
		updateCondition(CONDITION_PARANOID);
		m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestAllySentry, ENGI_SENTRY));
		return true;

		break;
	case BOT_UTIL_REMOVE_TMDISP_SAPPER:
		updateCondition(CONDITION_PARANOID);
		m_pSchedules->add(new CBotRemoveSapperSched(m_pNearestDisp, ENGI_DISP));
		return true;

		break;
	case BOT_UTIL_ENGI_DESTROY_DISP:
		engineerBuild(ENGI_DISP, ENGI_DESTROY);
	case BOT_UTIL_BUILDDISP:
		pWaypoint = nullptr;
		if (m_bDispenserVectorValid)
		{
			pWaypoint = CWaypoints::getWaypoint(
			    CWaypointLocations::NearestWaypoint(m_vDispenser, 150, -1, true, false, true, nullptr, false, getTeam(),
			                                        true, false, Vector(0, 0, 0), CWaypointTypes::W_FL_SENTRY));

			// no use going back to this waypoint
			if (pWaypoint && (pWaypoint->getArea() > 0) && (pWaypoint->getArea() != m_iCurrentAttackArea)
			    && (pWaypoint->getArea() != m_iCurrentDefendArea))
			{
				pWaypoint               = nullptr;
				m_bDispenserVectorValid = false;
			}
			else if (pWaypoint == nullptr)
				m_bDispenserVectorValid = false;
		}

		if ((pWaypoint == nullptr) && (m_pSentryGun.get() != nullptr))
		{
			if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
				pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SENTRY);
			if (pWaypoint == nullptr)
				pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
				    CBotGlobals::entityOrigin(m_pSentryGun), 150, -1, true, false, true, nullptr, false, getTeam(), true));
		}

		if (pWaypoint)
		{
			// Prefer obscure spots for dispenser too when not dominated
			{
				float fBestScore = evaluateBuildSpot(pWaypoint, 2);
				int iBestIdx     = CWaypoints::getWaypointIndex(pWaypoint);
				for (int w = 0; w < CWaypoints::numWaypoints(); w++)
				{
					CWaypoint *pW = CWaypoints::getWaypoint(w);
					if (!pW || !pW->isUsed() || !pW->forTeam(getTeam())
					    || !pW->hasFlag(CWaypointTypes::W_FL_SENTRY))
						continue;
					if (pW->getArea() != pWaypoint->getArea())
						continue;
					float fScore = evaluateBuildSpot(pW, 2);
					if (fScore > fBestScore * 1.3f)
						{ fBestScore = fScore; iBestIdx = w; }
				}
				if (iBestIdx != CWaypoints::getWaypointIndex(pWaypoint))
					pWaypoint = CWaypoints::getWaypoint(iBestIdx);
			}

			m_vDispenser            = pWaypoint->getOrigin();
			m_bDispenserVectorValid = true;
			updateCondition(CONDITION_COVERT);
			m_pSchedules->add(new CBotTFEngiBuild(this, ENGI_DISP, pWaypoint));
			m_iDispenserArea = pWaypoint->getArea();
			return true;
		}
		break;
	case BOT_UTIL_HIDE_FROM_ENEMY:
	{
		CBotSchedule *pSchedule = new CBotSchedule();

		pSchedule->setID(SCHED_GOOD_HIDE_SPOT);

		// run at flank while shooting
		CFindPathTask *pHideGoalPoint = new CFindPathTask();
		Vector vOrigin                = CBotGlobals::entityOrigin(m_pEnemy);

		pSchedule->addTask(new CFindGoodHideSpot(vOrigin));
		pSchedule->addTask(pHideGoalPoint);
		pSchedule->addTask(new CBotNest());

		// no interrupts, should be a quick waypoint path anyway
		pHideGoalPoint->setNoInterruptions();
		// get vector from good hide spot task
		pHideGoalPoint->getPassedVector();
		// Makes sure bot stopes trying to cover if ubered
		pHideGoalPoint->setInterruptFunction(new CBotTF2CoverInterrupt());
		// pSchedule->setID(SCHED_HIDE_FROM_ENEMY);

		m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
		m_pSchedules->addFront(pSchedule);

		return true;
	}
	case BOT_UTIL_MEDIC_HEAL:
		if (m_pHeal)
		{
			m_pSchedules->add(new CBotTF2HealSched(m_pHeal));
			return true;
		}
	case BOT_UTIL_MEDIC_HEAL_LAST:
		if (m_pLastHeal)
		{
			m_pSchedules->add(new CBotTF2HealSched(m_pLastHeal));
			return true;
		}
	case BOT_UTIL_UPGTMSENTRY:
		if (m_pNearestAllySentry)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pNearestAllySentry));
			return true;
		}
	case BOT_UTIL_UPGTMDISP:
		if (m_pNearestDisp)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pNearestDisp));
			return true;
		}
	case BOT_UTIL_UPGTMTELENT:
		if (m_pNearestTeleEntrance)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pNearestTeleEntrance));
			return true;
		}
	case BOT_UTIL_UPGSENTRY:
		if (m_pSentryGun)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pSentryGun));
			return true;
		}
	case BOT_UTIL_UPGTELENT:
		if (m_pTeleEntrance)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pTeleEntrance));
			return true;
		}
	case BOT_UTIL_UPGTELEXT:
		if (m_pTeleExit)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pTeleExit));
			return true;
		}
	case BOT_UTIL_UPGDISP:
		if (m_pDispenser)
		{
			m_pSchedules->add(new CBotTFEngiUpgrade(this, m_pDispenser));
			return true;
		}
	case BOT_UTIL_GETAMMODISP:
		if (m_pDispenser)
		{
			m_pSchedules->add(new CBotGetMetalSched(CBotGlobals::entityOrigin(m_pDispenser)));
			return true;
		}
	case BOT_UTIL_GOTORESUPPLY_FOR_HEALTH:
	{
		CWaypoint *pWaypointResupply = CWaypoints::getWaypoint(util->getIntData());

		m_pSchedules->add(new CBotTF2GetHealthSched(pWaypointResupply->getOrigin()));
	}
		return true;
	case BOT_UTIL_GOTORESUPPLY_FOR_AMMO:
	{
		CWaypoint *pWaypointResupply = CWaypoints::getWaypoint(util->getIntData());

		m_pSchedules->add(new CBotTF2GetAmmoSched(pWaypointResupply->getOrigin()));
	}
		return true;
	case BOT_UTIL_FIND_NEAREST_HEALTH:
	{
		CWaypoint *pWaypointHealth = CWaypoints::getWaypoint(util->getIntData());

		m_pSchedules->add(new CBotTF2GetHealthSched(pWaypointHealth->getOrigin()));
	}
		return true;
	case BOT_UTIL_FIND_NEAREST_AMMO:
	{
		CWaypoint *pWaypointAmmo = CWaypoints::getWaypoint(util->getIntData());

		m_pSchedules->add(new CBotTF2GetAmmoSched(pWaypointAmmo->getOrigin()));
	}
		return true;
	case BOT_UTIL_GOTODISP:
		m_pSchedules->removeSchedule(SCHED_USE_DISPENSER);
		m_pSchedules->addFront(new CBotUseDispSched(this, m_pNearestDisp));

		m_fPickupTime = engine->Time() + randomFloat(6.0f, 20.0f);
		return true;
	case BOT_UTIL_ENGI_MOVE_SENTRY:
		if (m_pSentryGun.get())
		{
			Vector vSentry       = CBotGlobals::entityOrigin(m_pSentryGun);

			CWaypoint *pWaypoint = nullptr;

			if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
			{
				pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SENTRY);
				/*
				Vector vFlagLocation;

				if ( CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlagLocation) )
				{
				    pWaypoint =
				CWaypoints::randomWaypointGoalNearestArea(CWaypointTypes::W_FL_SENTRY,m_iTeam,0,false,this,true,&vFlagLocation);
				}*/
			}
			else if (CTeamFortress2Mod::isAttackDefendMap())
			{
				if (getTeam() == TF2_TEAM_BLUE)
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(),
					                                           m_iCurrentAttackArea, true, this, false,
					                                           WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
				else
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(),
					                                           m_iCurrentDefendArea, true, this, true,
					                                           WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
			}

			if (!pWaypoint)
			{
				int iCappingTeam  = 0;
				bool bAllowAttack = ((m_iCurrentAttackArea == 0)
				                     || ((iCappingTeam = CTeamFortress2Mod::m_ObjectiveResource.GetCappingTeam(
				                              CTeamFortress2Mod::m_ObjectiveResource
				                                  .m_WaypointAreaToIndexTranslation[m_iCurrentAttackArea]))
				                         != CTeamFortress2Mod::getEnemyTeam(m_iTeam)));

				int area          = 0;

				if (bAllowAttack)
					if (iCappingTeam == m_iTeam) // Move Up Our team is attacking!!!
						area = m_iCurrentAttackArea;
					else
						area = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;
				else
					area = m_iCurrentDefendArea;

				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(), area, true, this,
				                                           area == m_iCurrentDefendArea, WPT_SEARCH_AVOID_SENTRIES,
				                                           m_iLastFailSentryWpt);

				if (!pWaypoint)
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(), 0, false, this,
					                                           true, WPT_SEARCH_AVOID_SENTRIES, m_iLastFailSentryWpt);
				}
			}

			if (pWaypoint && (pWaypoint->distanceFrom(vSentry) > rcbot_move_dist.GetFloat()))
			{
				updateCondition(CONDITION_COVERT);
				m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict, m_pSentryGun.get(), ENGI_SENTRY,
				                                           pWaypoint->getOrigin(), m_bIsCarryingSentry));
				m_iSentryArea = pWaypoint->getArea();
				return true;
			}
			// else
			//	destroySentry();
		}
		return false;
	case BOT_UTIL_SPYCHECK_AIR:
		m_pSchedules->add(new CBotSchedule(new CSpyCheckAir()));
		return true;
	case BOT_UTIL_PLACE_BUILDING:
		if (m_bIsCarryingObj)
		{
			primaryAttack(); // just press attack to place

			/* -- unused
			eEngiBuild iObject = ENGI_DISP;
			QAngle eyes = eyeAngles();
			Vector vForward;

			AngleVectors(eyes,&vForward);
			vForward = vForward/vForward.Length();

			if ( m_bIsCarryingTeleEnt )
			    iObject = ENGI_ENTRANCE;
			else if ( m_bIsCarryingTeleExit )
			    iObject = ENGI_EXIT;
			else if ( m_bIsCarryingDisp )
			    iObject = ENGI_DISP;
			else if ( m_bIsCarryingSentry )
			    iObject = ENGI_SENTRY;


			m_pSchedules->add(new CBotSchedule(new CBotTaskEngiPlaceBuilding(iObject,getOrigin()+vForward*32.0f)));
*/
			return true;
		}
		return false;
	case BOT_UTIL_ENGI_MOVE_DISP:
		if (m_pSentryGun.get() && m_pDispenser.get())
		{
			Vector vDisp         = CBotGlobals::entityOrigin(m_pDispenser);
			CWaypoint *pWaypoint = nullptr;

			if (m_pSentryGun)
				pWaypoint = CWaypoints::getWaypoint(
				    CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pSentryGun), 150, -1, true, false,
				                                        true, nullptr, false, getTeam(), true));
			else
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SENTRY, getTeam(), 0, false, this);

			if (pWaypoint && (pWaypoint->distanceFrom(vDisp) > rcbot_move_dist.GetFloat()))
			{
				updateCondition(CONDITION_COVERT);
				m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict, m_pDispenser.get(), ENGI_DISP,
				                                           pWaypoint->getOrigin(), m_bIsCarryingDisp));
				m_iDispenserArea = pWaypoint->getArea();
				return true;
			}
		}
		return false;
	case BOT_UTIL_ENGI_MOVE_ENTRANCE:

		if (m_pTeleEntrance.get())
		{
			Vector vTele         = CBotGlobals::entityOrigin(m_pTeleEntrance);
			CWaypoint *pWaypoint = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
			    m_vTeleportEntrance, 512, -1, true, false, true, nullptr, false, getTeam(), true));

			if (pWaypoint && (pWaypoint->distanceFrom(vTele) > rcbot_move_dist.GetFloat()))
			{
				updateCondition(CONDITION_COVERT);
				m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict, m_pTeleEntrance.get(), ENGI_ENTRANCE,
				                                           pWaypoint->getOrigin(), m_bIsCarryingTeleEnt));
				m_iTeleEntranceArea = pWaypoint->getArea();
				return true;
			}
		}

		return false;
	case BOT_UTIL_ENGI_MOVE_EXIT:

		if (m_pTeleExit.get())
		{
			Vector vTele = CBotGlobals::entityOrigin(m_pTeleExit);

			if (CTeamFortress2Mod::isAttackDefendMap())
			{
				if (getTeam() == TF2_TEAM_BLUE)
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(),
					                                           m_iCurrentAttackArea, true, this, true);
				else
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(),
					                                           m_iCurrentDefendArea, true, this, true);
			}

			if (!pWaypoint)
			{
				int area  = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;

				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(), area, true,
				                                           this); // CTeamFortress2Mod::getArea());

				if (!pWaypoint)
				{
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_TELE_EXIT, getTeam(), 0, false,
					                                           this); // CTeamFortress2Mod::getArea());
				}
			}

			if (pWaypoint && (pWaypoint->distanceFrom(vTele) > rcbot_move_dist.GetFloat()))
			{
				updateCondition(CONDITION_COVERT);
				m_pSchedules->add(new CBotEngiMoveBuilding(m_pEdict, m_pTeleExit.get(), ENGI_EXIT,
				                                           pWaypoint->getOrigin(), m_bIsCarryingTeleExit));
				m_iTeleExitArea = pWaypoint->getArea();
				return true;
			}
		}
		return false;
	case BOT_UTIL_FIND_MEDIC_FOR_HEALTH:
	{
		Vector vLoc = m_pLastSeeMedic.getLocation();
		CFindPathTask *findpath       = new CFindPathTask(vLoc, LOOK_WAYPOINT);
		findpath->setFailInterrupt(CONDITION_SEE_CUR_ENEMY);

		CTaskVoiceCommand *shoutMedic = new CTaskVoiceCommand(TF_VC_MEDIC);
		CBotTF2WaitHealthTask *wait   = new CBotTF2WaitHealthTask(vLoc);
		CBotSchedule *newSched        = new CBotSchedule();

		findpath->setCompleteInterrupt(0, CONDITION_NEED_HEALTH);
		shoutMedic->setCompleteInterrupt(0, CONDITION_NEED_HEALTH);
		wait->setCompleteInterrupt(0, CONDITION_NEED_HEALTH);

		newSched->addTask(findpath);
		newSched->addTask(shoutMedic);
		newSched->addTask(wait);
		m_pSchedules->addFront(newSched);

		return true;
	}
	case BOT_UTIL_GETHEALTHKIT:
		m_pSchedules->removeSchedule(SCHED_PICKUP);
		m_pSchedules->addFront(new CBotPickupSched(m_pHealthkit));

		m_fPickupTime = engine->Time() + randomFloat(5.0f, 10.0f);

		return true;
	case BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY:
	case BOT_UTIL_DEMO_STICKYTRAP_FLAG:
	case BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN:
	case BOT_UTIL_DEMO_STICKYTRAP_POINT:
	case BOT_UTIL_DEMO_STICKYTRAP_PL:
		// to do
		{

			Vector vStand;
			Vector vPoint;
			Vector vDemoStickyPoint;
			eDemoTrapType iDemoTrapType = TF_TRAP_TYPE_NONE;

			if (id == BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY)
			{
				pWaypoint     = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
                    m_vLastSeeEnemy, 400, -1, true, false, true, 0, false, getTeam(), true));
				iDemoTrapType = TF_TRAP_TYPE_WPT;
			}
			else if (id == BOT_UTIL_DEMO_STICKYTRAP_FLAG)
			{
				pWaypoint     = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG,
				                                               CTeamFortress2Mod::getEnemyTeam(getTeam()));
				iDemoTrapType = TF_TRAP_TYPE_FLAG;
			}
			else if (id == BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN)
			{
				pWaypoint     = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
                    m_vLastKnownTeamFlagPoint, 400, -1, true, false, true, 0, false, getTeam(), true));
				iDemoTrapType = TF_TRAP_TYPE_FLAG;
			}
			else if (id == BOT_UTIL_DEMO_STICKYTRAP_POINT)
			{
				if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
					pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT);
				else
					pWaypoint =
					    CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_CAPPOINT, 0, m_iCurrentDefendArea, true);

				iDemoTrapType = TF_TRAP_TYPE_POINT;
			}
			else if (id == BOT_UTIL_DEMO_STICKYTRAP_PL)
			{
				pWaypoint = CWaypoints::getWaypoint(
				    CWaypointLocations::NearestWaypoint(CBotGlobals::entityOrigin(m_pDefendPayloadBomb), 400, -1, true,
				                                        false, true, 0, false, getTeam(), true));
				iDemoTrapType = TF_TRAP_TYPE_PL;
			}

			if (pWaypoint)
			{
				CWaypoint *pStand = nullptr;
				CWaypoint *pTemp;
				float fDist    = 9999.0f;
				float fClosest = 9999.0f;

				vPoint         = pWaypoint->getOrigin();

				WaypointList m_iVisibles;
				WaypointList m_iInvisibles;

				int iWptFrom =
				    CWaypointLocations::NearestWaypoint(vPoint, 2048.0, -1, true, true, true, nullptr, false, 0, false);

				// int m_iVisiblePoints[CWaypoints::MAX_WAYPOINTS]; // make searching quicker

				CWaypointLocations::GetAllVisible(iWptFrom, iWptFrom, vPoint, vPoint, 2048.0, &m_iVisibles,
				                                  &m_iInvisibles);

				for (int i = 0; i < m_iVisibles.size(); i++)
				{
					if (m_iVisibles[i] == CWaypoints::getWaypointIndex(pWaypoint))
						continue;

					pTemp = CWaypoints::getWaypoint(m_iVisibles[i]);

					if (pTemp->distanceFrom(pWaypoint) < 512)
					{
						fDist = distanceFrom(pTemp->getOrigin());

						if (fDist < fClosest)
						{
							fClosest = fDist;
							pStand   = pTemp;
						}
					}
				}

				if (!pStand)
				{
					pStand = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
					    pWaypoint->getOrigin(), 400, CWaypoints::getWaypointIndex(pWaypoint), true, false, true, 0,
					    false, getTeam(), true));
				}

				if (pStand)
				{
					vStand = pStand->getOrigin();

					if (pWaypoint)
					{
						m_pSchedules->add(new CBotTF2DemoPipeTrapSched(
						    iDemoTrapType, vStand, vPoint, Vector(150, 150, 20), false, pWaypoint->getArea()));
						return true;
					}
				}
			}
		}
		break;
		// dispenser
	case BOT_UTIL_SAP_NEAREST_DISP:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyDisp, ENGI_DISP));
		return true;
	case BOT_UTIL_SAP_ENEMY_DISP:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pEnemy, ENGI_DISP));
		return true;
	case BOT_UTIL_SAP_LASTENEMY_DISP:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pLastEnemy, ENGI_DISP));
		return true;
		// Teleporter
	case BOT_UTIL_SAP_NEAREST_TELE:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemyTeleporter, ENGI_TELE));
		return true;
	case BOT_UTIL_SAP_ENEMY_TELE:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pEnemy, ENGI_TELE));
		return true;
	case BOT_UTIL_SAP_LASTENEMY_TELE:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pLastEnemy, ENGI_TELE));
		return true;
	case BOT_UTIL_SAP_LASTENEMY_SENTRY:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pLastEnemySentry, ENGI_SENTRY));
		return true;
	case BOT_UTIL_SAP_ENEMY_SENTRY:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pEnemy, ENGI_SENTRY));
		return true;
	case BOT_UTIL_SAP_NEAREST_SENTRY:
		m_pSchedules->add(new CBotSpySapBuildingSched(m_pNearestEnemySentry, ENGI_SENTRY));
		return true;
	case BOT_UTIL_SPAM_NEAREST_SENTRY:
	case BOT_UTIL_SPAM_LAST_ENEMY:
	case BOT_UTIL_SPAM_LAST_ENEMY_SENTRY:
	{
		Vector vLoc;
		Vector vEnemy;
		edict_t *pEnemy;

		vLoc = getOrigin();

		if (id == BOT_UTIL_SPAM_NEAREST_SENTRY)
		{
			pEnemy = m_pNearestEnemySentry;
			vEnemy = CBotGlobals::entityOrigin(m_pNearestEnemySentry);
		}
		else if (id == BOT_UTIL_SPAM_LAST_ENEMY_SENTRY)
		{
			pEnemy = m_pLastEnemySentry;
			vEnemy = CBotGlobals::entityOrigin(m_pLastEnemySentry);
			vLoc   = m_vLastDiedOrigin;
		}
		else
		{
			pEnemy = m_pLastEnemy;
			vEnemy = m_vLastSeeEnemy;
		}

		/*

		CWaypoint *pWptBlast =
		CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(vEnemy,vLoc,4096.0,-1,true,true,false,false,getTeam(),false));

		if ( pWptBlast )
		{
		    CWaypoint *pWpt =
		CWaypoints::getWaypoint(CWaypointLocations::NearestBlastWaypoint(vLoc,pWptBlast->getOrigin(),4096.0,CWaypoints::getWaypointIndex(pWptBlast),true,false,true,false,getTeam(),true,1024.0f));
		    */
		int iAiming;
		CWaypoint *pWpt = CWaypoints::nearestPipeWaypoint(vEnemy, getOrigin(), &iAiming);

		if (pWpt)
		{
			CFindPathTask *findpath = new CFindPathTask(pEnemy);
			CBotTask *pipetask      = new CBotTF2Spam(pWpt->getOrigin(), vEnemy, util->getWeaponChoice());
			CBotSchedule *pipesched = new CBotSchedule();

			pipesched->addTask(new CBotTF2FindPipeWaypoint(vLoc, vEnemy));
			pipesched->addTask(findpath);
			pipesched->addTask(pipetask);

			m_pSchedules->add(pipesched);

			findpath->getPassedIntAsWaypointId();
			findpath->completeIfSeeTaskEdict();
			findpath->dontGoToEdict();
			findpath->setDangerPoint(CWaypointLocations::NearestWaypoint(vEnemy, 200.0f, -1));

			return true;
		}
		//}
	}
	break;
	case BOT_UTIL_PIPE_NEAREST_SENTRY:
	case BOT_UTIL_PIPE_LAST_ENEMY:
	case BOT_UTIL_PIPE_LAST_ENEMY_SENTRY:
	{
		Vector vLoc;
		Vector vEnemy;
		edict_t *pEnemy;

		vLoc = getOrigin();

		if (id == BOT_UTIL_PIPE_NEAREST_SENTRY)
		{
			pEnemy = m_pNearestEnemySentry;
			vEnemy = CBotGlobals::entityOrigin(m_pNearestEnemySentry);
		}
		else if (id == BOT_UTIL_PIPE_LAST_ENEMY_SENTRY)
		{
			pEnemy = m_pLastEnemySentry;
			vEnemy = CBotGlobals::entityOrigin(m_pLastEnemySentry);
			vLoc   = m_vLastDiedOrigin;
		}
		else
		{
			pEnemy = m_pLastEnemy;
			vEnemy = m_vLastSeeEnemy;
		}

		int iAiming;
		CWaypoint *pWpt = CWaypoints::nearestPipeWaypoint(vEnemy, getOrigin(), &iAiming);

		if (pWpt)
		{

			CFindPathTask *findpath = new CFindPathTask(pEnemy);
			CBotTask *pipetask      = new CBotTF2DemomanPipeEnemy(
                getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)), vEnemy, pEnemy);
			CBotSchedule *pipesched = new CBotSchedule();

			pipetask->setInterruptFunction(new CBotTF2HurtInterrupt(this));
			pipesched->addTask(new CBotTF2FindPipeWaypoint(vLoc, vEnemy));
			pipesched->addTask(findpath);
			pipesched->addTask(pipetask);

			m_pSchedules->add(pipesched);

			findpath->getPassedIntAsWaypointId();
			findpath->setDangerPoint(CWaypointLocations::NearestWaypoint(vEnemy, 200.0f, -1));
			findpath->completeIfSeeTaskEdict();
			findpath->dontGoToEdict();

			return true;
		}
	}
	break;
	case BOT_UTIL_GETAMMOKIT:
		m_pSchedules->removeSchedule(SCHED_PICKUP);
		m_pSchedules->addFront(new CBotPickupSched(m_pAmmo));

		m_fPickupTime = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	case BOT_UTIL_SNIPE_CROSSBOW:

		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		{
			pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SNIPER);
			/*
			Vector vFlagLocation;

			if ( CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlagLocation) )
			{
			pWaypoint =
			CWaypoints::randomWaypointGoalNearestArea(CWaypointTypes::W_FL_SNIPER,m_iTeam,0,false,this,true,&vFlagLocation);
			}*/
		}
		else if (CTeamFortress2Mod::isAttackDefendMap())
		{
			if (getTeam() == TF2_TEAM_RED)
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), m_iCurrentDefendArea,
				                                           true, this, true, WPT_SEARCH_AVOID_SNIPERS);
			else
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), m_iCurrentAttackArea,
				                                           false, this, false, WPT_SEARCH_AVOID_SNIPERS);
		}

		if (!pWaypoint)
		{
			int area  = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;

			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), area, true, this,
			                                           area == m_iCurrentDefendArea, WPT_SEARCH_AVOID_SNIPERS);

			if (!pWaypoint)
			{
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), 0, false, this,
				                                           false, WPT_SEARCH_AVOID_SNIPERS);
			}
		}

		if (pWaypoint)
		{
			m_pSchedules->add(
			    new CBotTF2SnipeCrossBowSched(pWaypoint->getOrigin(), CWaypoints::getWaypointIndex(pWaypoint)));
			return true;
		}
		break;
	case BOT_UTIL_SNIPE:

		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
		{
			pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SNIPER);
			/*
			Vector vFlagLocation;

			if ( CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE,&vFlagLocation) )
			{
			    pWaypoint =
			CWaypoints::randomWaypointGoalNearestArea(CWaypointTypes::W_FL_SNIPER,m_iTeam,0,false,this,true,&vFlagLocation);
			}*/
		}
		else if (CTeamFortress2Mod::isAttackDefendMap())
		{
			if (getTeam() == TF2_TEAM_RED)
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), m_iCurrentDefendArea,
				                                           true, this, true, WPT_SEARCH_AVOID_SNIPERS);
			else
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), m_iCurrentAttackArea,
				                                           false, this, false, WPT_SEARCH_AVOID_SNIPERS);
		}

		if (!pWaypoint)
		{
			int area  = (randomInt(0, 1) == 1) ? m_iCurrentAttackArea : m_iCurrentDefendArea;

			pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), area, true, this,
			                                           area == m_iCurrentDefendArea, WPT_SEARCH_AVOID_SNIPERS);

			if (!pWaypoint)
			{
				pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), 0, false, this,
				                                           false, WPT_SEARCH_AVOID_SNIPERS);
			}
		}

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotTF2SnipeSched(pWaypoint->getOrigin(), CWaypoints::getWaypointIndex(pWaypoint)));
			return true;
		}
		break;
	case BOT_UTIL_GETFLAG_LASTKNOWN:
		pWaypoint =
		    CWaypoints::getWaypoint(CWaypoints::nearestWaypointGoal(-1, m_vLastKnownFlagPoint, 512.0, getTeam()));

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotTF2FindFlagSched(m_vLastKnownFlagPoint));
			return true;
		}
		break;
	case BOT_UTIL_MEDIC_FINDPLAYER_AT_SPAWN:
	{
		CWaypoint *pWaypoint = nullptr;

		pWaypoint            = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
            m_vTeleportEntrance, 300, -1, true, false, true, nullptr, false, getTeam(), true));

		// if ( pWaypoint && randomInt(0,1) )
		//	pWaypoint = CWaypoints::getPinchPointFromWaypoint(getOrigin(),pWaypoint->getOrigin());

		if (pWaypoint)
		{
			setLookAt(pWaypoint->getOrigin());
			m_pSchedules->add(new CBotDefendSched(pWaypoint->getOrigin(), randomFloat(10.0f, 25.0f)));
			removeCondition(CONDITION_PUSH);
			return true;
		}
	}
	break;
	case BOT_UTIL_MEDIC_FINDPLAYER:
	{
		m_pSchedules->add(new CBotTF2HealSched(m_pLastCalledMedic));
		// roam
		// pWaypoint = CWaypoints::randomWaypointGoal(-1,getTeam(),0,false,this);

		// if ( pWaypoint )
		//{
		//	m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
		//	return true;
		// }
	}
	break;
	case BOT_UTIL_MESSAROUND:
	{
		// Prefer the player/bot who invited us to mess around
		edict_t *pNearby = m_pMessAroundInviter.get();
		if (pNearby && CBotGlobals::entityIsValid(pNearby)
		    && CBotGlobals::entityIsAlive(pNearby) && isVisible(pNearby))
		{
			m_pMessAroundInviter = MyEHandle(nullptr);
		}
		else
		{
			// Fall back to finding a random nearby teammate
			pNearby = nullptr;
		}

		if (!pNearby)
		{
			// find a nearby friendly
		int i = 0;
		edict_t *pEdict;
		edict_t *pFallback = nullptr;
		float fMaxDistance = 800;
		float fFallbackDist = 800;
		float fDistance;

		for (i = 1; i <= CBotGlobals::maxClients(); i++)
		{
			pEdict = INDEXENT(i);

			if (pEdict == m_pEdict)
				continue;

			if (CBotGlobals::entityIsValid(pEdict))
			{
				if (CClassInterface::getTeam(pEdict) == getTeam())
				{
					fDistance = distanceFrom(pEdict);

					if (isVisible(pEdict) && fDistance < fMaxDistance)
					{
						if (!pNearby || randomInt(0, 1))
						{
							pNearby      = pEdict;
							fMaxDistance = fDistance;
						}
					}
					else if (fDistance < fFallbackDist)
					{
						pFallback     = pEdict;
						fFallbackDist = fDistance;
					}
				}
			}
		}

		if (!pNearby)
			pNearby = pFallback;
		}

		if (pNearby)
		{
			m_pSchedules->add(new CBotTF2MessAroundSched(pNearby, TF_VC_INVALID));
			return true;
		}

		return false;
	}
	break;
	case BOT_UTIL_GETFLAG:
		pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_FLAG, getTeam());

		if (pWaypoint)
		{
			CWaypoint *pRoute = nullptr;
			Vector vRoute     = Vector(0, 0, 0);
			bool bUseRoute    = false;

			if ((m_fUseRouteTime < engine->Time())
		    || (m_iClass == TF_CLASS_SCOUT) || (m_iClass == TF_CLASS_SPY) || (m_iClass == TF_CLASS_SNIPER))
			{
				// find random route
				pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), getTeam(),
				                                         m_iCurrentAttackArea);

				if (pRoute)
				{
					bUseRoute       = true;
					vRoute          = pRoute->getOrigin();
					m_fUseRouteTime = engine->Time() + ((m_iClass == TF_CLASS_SCOUT || m_iClass == TF_CLASS_SPY
					                                    || m_iClass == TF_CLASS_SNIPER)
					                                       ? randomFloat(5.0f, 10.0f)
					                                       : randomFloat(30.0f, 60.0f));
				}
			}

			m_pSchedules->add(new CBotTF2GetFlagSched(pWaypoint->getOrigin(), bUseRoute, vRoute));

			return true;
		}

		break;
	case BOT_UTIL_FIND_SQUAD_LEADER:
	{
		Vector pos         = m_pSquad->GetFormationVector(m_pEdict);
		CBotTask *findTask = new CFindPathTask(pos);
		removeCondition(CONDITION_SEE_SQUAD_LEADER);
		removeCondition(CONDITION_SQUAD_LEADER_INRANGE);
		findTask->setCompleteInterrupt(CONDITION_SEE_SQUAD_LEADER | CONDITION_SQUAD_LEADER_INRANGE);

		m_pSchedules->add(new CBotSchedule(findTask));

		return true;
	}
	break;
	case BOT_UTIL_FOLLOW_SQUAD_LEADER:
	{
		Vector pos = m_pSquad->GetFormationVector(m_pEdict);

		m_pSchedules->add(new CBotSchedule(new CBotFollowSquadLeader(m_pSquad)));

		return true;
	}
	break;
	case BOT_UTIL_ROAM:
		// roam
		pWaypoint = CWaypoints::randomWaypointGoal(-1, getTeam(), 0, false, this);

		if (pWaypoint)
		{
			m_pSchedules->add(new CBotGotoOriginSched(pWaypoint->getOrigin()));
			return true;
		}
		break;
	case BOT_UTIL_ATTACK_TANK:
	{
		edict_t *pTank = CTeamFortress2Mod::getNearestTank();
		if (pTank && CBotGlobals::entityIsAlive(pTank))
		{
			m_pEnemy = pTank;
			m_fUtilTimes[BOT_UTIL_ATTACK_TANK] = engine->Time() + 2.0f;
			wantToShoot(true);
			return true;
		}
	}
	break;
	case BOT_UTIL_MVM_COLLECT_CASH:
	{
		// Find the center of all nearby cash packs for group collection
		Vector vGroupCenter(0, 0, 0);
		int iCashCount = 0;
		for (int i = (gpGlobals->maxClients + 1); i < gpGlobals->maxClients + 512; i++)
		{
			edict_t *pEnt = INDEXENT(i);
			if (!pEnt || pEnt->IsFree()) continue;
			if (!CBotGlobals::entityIsValid(pEnt) || !CBotGlobals::entityIsAlive(pEnt)) continue;
			if (strcmp(pEnt->GetClassName(), "item_currencypack_custom") != 0) continue;
			if (distanceFrom(pEnt) > 400.0f) continue;
			vGroupCenter = vGroupCenter + CBotGlobals::entityOrigin(pEnt);
			iCashCount++;
		}

		if (iCashCount > 0)
		{
			vGroupCenter = vGroupCenter / (float)iCashCount;
			CFindPathTask *path = new CFindPathTask(vGroupCenter);
			path->setCompleteInterrupt(0, CONDITION_SEE_CUR_ENEMY);
			CBotSchedule *sched = new CBotSchedule();
			sched->addTask(path);
			sched->addTask(new CMoveToTask(vGroupCenter));
			sched->setID(SCHED_MVM_COLLECT_CASH);
			m_pSchedules->add(sched);
			return true;
		}
	}
	break;
	case BOT_UTIL_MVM_MEDIC_REVIVE:
	{
		if (m_pHeal && CBotGlobals::entityIsAlive(m_pHeal) && (getHealFactor(m_pHeal) > 0))
		{
			m_pSchedules->add(new CBotTF2HealSched(m_pHeal));
			return true;
		}
	}
	break;
	case BOT_UTIL_SAP_MVM_ROBOT:
	{
		int iEntIdx = util->getIntData();
		edict_t *pRobot = INDEXENT(iEntIdx);
		if (pRobot && CBotGlobals::entityIsAlive(pRobot))
		{
			m_pSchedules->add(new CBotSpySapBuildingSched(pRobot, ENGI_ROBOT));
			// Shorter cooldown for high-value targets
			bool bBuster = false;
			IServerEntity *pServ = pRobot->GetIServerEntity();
			if (pServ)
			{
				const char *szModel = pServ->GetModelName().ToCStr();
				if (szModel && strstr(szModel, "sentry_buster"))
					bBuster = true;
			}
			bool bGiant = (pRobot->GetCollideable()
			               && pRobot->GetCollideable()->OBBMaxs().Length() > 80.0f);
			if (bBuster || bGiant)
				m_fSpySapTime = engine->Time() + randomFloat(2.0f, 4.0f);
			else
				m_fSpySapTime = engine->Time() + randomFloat(8.0f, 15.0f);
			return true;
		}
	}
	break;
	default:
		break;
	}

	return false;
}

void CBotTF2::touchedWpt(CWaypoint *pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	static int wptindex;

	CBot::touchedWpt(pWaypoint);

	if (canGotoWaypoint(getOrigin(), pWaypoint))
	{
		if (pWaypoint->hasFlag(CWaypointTypes::W_FL_ROCKET_JUMP))
		{
			if (getNavigator()->hasNextPoint())
			{
				CBotWeapon *pWeapon = nullptr;

				if (getClass() == TF_CLASS_SOLDIER)
				{
					pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));

					if (pWeapon && pWeapon->hasWeapon())
						m_pSchedules->addFront(new CBotSchedule(new CBotTFRocketJump()));
				}
				else if ((getClass() == TF_CLASS_DEMOMAN) && rcbot_demo_jump.GetBool())
				{
					pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

					if (pWeapon && pWeapon->hasWeapon())
					{
						edict_t *pWeaponEdict = pWeapon->getWeaponEntity();

						// if it isnt the scottish resistance pipe launcher
						if (!pWeaponEdict || (CClassInterface::TF2_getItemDefinitionIndex(pWeaponEdict) != 130))
							m_pSchedules->addFront(new CBotSchedule(new CBotTF2DemomanPipeJump(
							    this, pWaypoint->getOrigin(), getNavigator()->getNextPoint(),
							    getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)))));
					}
				}
			}
		}
		else if (pWaypoint->hasFlag(CWaypointTypes::W_FL_DOUBLEJUMP))
		{
			m_pButtons->tap(IN_JUMP);
			m_fDoubleJumpTime = engine->Time() + bot_scoutdj.GetFloat();
			// m_pSchedules->addFront(new CBotSchedule(new CBotTFDoubleJump()));
		}
		else if (pWaypoint->getFlags() == 0)
		{
			if (getNavigator()->hasNextPoint() && (getClass() == TF_CLASS_SCOUT))
			{
				if (randomFloat(0.0f, 100.0f) > (m_pProfile->m_fBraveness * 10))
				{
					float fVel = m_vVelocity.Length();

					if (fVel > 0.0f)
					{
						Vector v_next = getNavigator()->getNextPoint();
						Vector v_org  = getOrigin();
						Vector v_comp = v_next - v_org;
						float fDist   = v_comp.Length();

						Vector v_vel  = (m_vVelocity / fVel) * fDist;

						if ((v_next - (v_org + v_vel)).Length() <= 24.0f)
							m_pButtons->tap(IN_JUMP);
					}
				}
			}
		}
		else if (pWaypoint->hasFlag(CWaypointTypes::W_FL_FALL))
		{
			// jump to avoid being hurt (scouts can jump in the air)
			if (fabs(m_vVelocity.z) > 1)
				jump();
		}
	}

	// only good for spies so they know when to cloak better
	if (getClass() == TF_CLASS_SPY)
	{
		wptindex = CWaypoints::getWaypointIndex(pWaypoint);

		if (m_pEnemy && hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
			m_pNavigator->beliefOne(wptindex, BELIEF_DANGER, distanceFrom(m_pEnemy));
		else
			m_pNavigator->beliefOne(wptindex, BELIEF_SAFETY, 0);
	}
}

void EnemyMoveHistory::record(const Vector &vPos, const Vector &vVel, float fTime)
{
	// Drop samples that are too close together (< 40ms)
	if (iCount > 0 && fTime - fTimes[(iCount - 1) % MOVEMENT_HISTORY_MAX] < 0.04f)
		return;

	int i = iCount % MOVEMENT_HISTORY_MAX;
	vSamples[i]   = vPos;
	vVelSamples[i] = vVel;
	fTimes[i]     = fTime;
	fLastSample   = fTime;
	iCount++;

	// Detect direction changes (2D)
	Vector vDir2D = vVel;
	vDir2D.z      = 0;
	float fLen     = vDir2D.Length();
	if (fLen > 10.0f)
	{
		vDir2D = vDir2D / fLen;
		if (vLastDir2D.Length() > 0.01f && vDir2D.Dot(vLastDir2D) < 0.3f)
		{
			float fInterval = fTime - fLastDirChange;
			if (fLastDirChange > 0 && fInterval > 0.15f)
			{
				iDirChanges++;
				float fAvg = (iDirChanges <= 1) ? fInterval
				            : (fLastDirChange > 0 ? (fInterval + fLastDirChange) * 0.5f : fInterval);
				fLastDirChange = fTime; // track last change for next interval calc
				(void)fAvg; // stored implicitly via sampling
			}
			else if (fLastDirChange == 0)
				fLastDirChange = fTime;
		}
		vLastDir2D = vDir2D;
	}

	// Prune old entries (> 6 seconds)
	int iPrune = 0;
	while (iPrune < iCount && fTime - fTimes[iPrune % MOVEMENT_HISTORY_MAX] > 6.0f)
		iPrune++;
	if (iPrune > 0 && iPrune < iCount)
	{
		// Shift remaining entries to front (approximate: just adjust count)
		int iKeep = iCount - iPrune;
		for (int j = 0; j < iKeep; j++)
		{
			int iSrc = (iPrune + j) % MOVEMENT_HISTORY_MAX;
			vSamples[j] = vSamples[iSrc];
			vVelSamples[j] = vVelSamples[iSrc];
			fTimes[j] = fTimes[iSrc];
		}
		iCount = iKeep;
		iDirChanges = 0;
		vLastDir2D = Vector(0,0,0);
		fLastDirChange = 0;
		// Recompute direction changes
		for (int j = 1; j < iCount; j++)
		{
			Vector vD = vVelSamples[j];
			vD.z = 0;
			float l = vD.Length();
			if (l > 10.0f)
			{
				vD = vD / l;
				Vector vPrev = vVelSamples[j-1];
				vPrev.z = 0;
				float lp = vPrev.Length();
				if (lp > 10.0f)
				{
					vPrev = vPrev / lp;
					if (vD.Dot(vPrev) < 0.3f)
					{
						iDirChanges++;
						fLastDirChange = fTimes[j];
					}
				}
			}
		}
		if (iCount > 0)
			vLastDir2D = vVelSamples[iCount-1];
		vLastDir2D.z = 0;
		float fl = vLastDir2D.Length();
		if (fl > 10.0f)
			vLastDir2D = vLastDir2D / fl;
	}

	// Cap at MOVEMENT_HISTORY_MAX
	if (iCount > MOVEMENT_HISTORY_MAX)
		iCount = MOVEMENT_HISTORY_MAX;
}

Vector EnemyMoveHistory::predict(float fTime, float fConfidence) const
{
	if (iCount < 2)
		return Vector(0, 0, 0);

	int iLast       = (iCount - 1) % MOVEMENT_HISTORY_MAX;
	Vector vCurrVel  = vVelSamples[iLast];
	float fSpeed     = vCurrVel.Length2D();

	if (fSpeed < 5.0f)
		return Vector(0, 0, 0);

	// Check for zigzag pattern: direction changes every 0.2-0.9s
	float fLastChange = fLastDirChange;
	float fTimeSinceChange = (fLastSample > 0 && fLastChange > 0) ? (fLastSample - fLastChange) : 0.0f;

	bool bZigZag = false;
	float fSwitchTime = 0.5f;

	if (iDirChanges >= 2)
	{
		float fInterval = (iCount > 1) ? (fTimes[iLast] - fTimes[0]) / (float)iCount : 0.05f;
		fSwitchTime = fInterval * 4.0f;
		if (fSwitchTime > 0.2f && fSwitchTime < 1.2f)
			bZigZag = true;
	}

	if (bZigZag && fTimeSinceChange > 0)
	{
		float fTimeToSwitch = fSwitchTime - fTimeSinceChange;
		if (fTimeToSwitch > 0 && fTimeToSwitch < fTime)
		{
			Vector vDir2D = vCurrVel;
			vDir2D.z = 0;
			float fLen = vDir2D.Length();
			if (fLen > 0.1f)
				vDir2D = vDir2D / fLen;
			Vector vNewDir = vDir2D * -1.0f;
			Vector vPred = (vDir2D * fSpeed * fTimeToSwitch) + (vNewDir * fSpeed * (fTime - fTimeToSwitch));
			return vPred;
		}
	}

	// Apply overshoot/undershoot learning. Default 1.0 = no adjustment,
	// >1.0 = we've been underpredicting, lead more. <1.0 = overpredicting.
	return vCurrVel * fTime * fAdjSmooth;
}

void EnemyMoveHistory::adjustFromError(const Vector &vPredOffset, float fPredDuration, float fTime)
{
	if (iCount < 2 || fPredDuration < 0.02f)
		return;

	int iLast   = (iCount - 1) % MOVEMENT_HISTORY_MAX;
	Vector vActualVel = vVelSamples[iLast];

	float fActualDist = vActualVel.Length2D() * fPredDuration;
	float fPredDist   = vPredOffset.Length2D();

	if (fActualDist < 1.0f || fPredDist < 1.0f)
		return;

	// If predicted distance > actual distance: we overpredicted (adjust < 1.0)
	// If predicted distance < actual distance: we underpredicted (adjust > 1.0)
	float fErrorRatio = fActualDist / fPredDist;
	if (fErrorRatio < 0.1f) fErrorRatio = 0.1f;
	if (fErrorRatio > 3.0f) fErrorRatio = 3.0f;

	// Faster convergence for consistent error patterns
	float fAlpha = (fErrorRatio < 0.5f || fErrorRatio > 1.5f) ? 0.25f : 0.15f;
	fAdjSmooth   = fAdjSmooth * (1.0f - fAlpha) + fErrorRatio * fAlpha;
	iAdjustCount++;

	if (fAdjSmooth < 0.3f) fAdjSmooth = 0.3f;
	if (fAdjSmooth > 3.0f) fAdjSmooth = 3.0f;
}

void CBotTF2::recordEnemyMovement(edict_t *pEnemy)
{
	if (!pEnemy || !CBotGlobals::entityIsValid(pEnemy))
		return;

	int i = ENTINDEX(pEnemy) - 1;
	if (i < 0 || i >= 64)
		return;

	EnemyMoveHistory *hist = &m_EnemyMovement[i];
	if (!hist->bActive)
	{
		// Initialize
		*hist = EnemyMoveHistory();
		hist->iEntIndex = i + 1;
		hist->bActive   = true;
	}

	Vector vPos = CBotGlobals::entityOrigin(pEnemy);
	Vector vVel;
	if (!CClassInterface::getVelocity(pEnemy, &vVel))
	{
		CClient *pClient = CClients::get(pEnemy);
		if (pClient)
			vVel = pClient->getVelocity();
		else
			vVel = Vector(0, 0, 0);
	}

	hist->record(vPos, vVel, engine->Time());
}

Vector CBotTF2::predictEnemyOffset(edict_t *pEnemy, float fTime, float fConfidence)
{
	if (!pEnemy)
		return Vector(0, 0, 0);

	int i = ENTINDEX(pEnemy) - 1;
	if (i < 0 || i >= 64)
		return Vector(0, 0, 0);

	EnemyMoveHistory *hist = &m_EnemyMovement[i];
	if (!hist->bActive || hist->iEntIndex != ENTINDEX(pEnemy))
		return Vector(0, 0, 0);

	// Check if we have a previous prediction to compare against
	float fNow = engine->Time();
	if (hist->fLastPredTime > 0 && hist->fLastPredDuration > 0.02f)
	{
		float fElapsed = fNow - hist->fLastPredTime;
		if (fElapsed > 0 && fElapsed < 3.0f)
			hist->adjustFromError(hist->vLastPred, hist->fLastPredDuration, fNow);
	}

	Vector vPred = hist->predict(fTime, fConfidence);

	// Store this prediction for error comparison next time
	hist->vLastPred         = vPred;
	hist->fLastPredTime     = fNow;
	hist->fLastPredDuration = fTime;

	return vPred;
}

void CBotTF2::modAim(edict_t *pEntity, Vector &v_origin, Vector *v_desired_offset, Vector &v_size, float fDist,
                     float fDist2D)
{
	static CBotWeapon *pWp;
	static float fTime;

	pWp = getCurrentWeapon();

	CBot::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D);

	if (pWp)
	{
		/*if (m_iClass == TF_CLASS_SNIPER)
		{
		    if (pWp->getID() == TF2_WEAPON_BOW)
		    {
		        if (pWp->getProjectileSpeed() > 0)
		        {
		            CClient *pClient = CClients::get(pEntity);
		            Vector vVelocity;

		            if (CClassInterface::getVelocity(pEntity, &vVelocity))
		            {
		                if (pClient && (vVelocity == Vector(0, 0, 0)))
		                    vVelocity = pClient->getVelocity();
		            }
		            else if (pClient)
		                vVelocity = pClient->getVelocity();

		            // the arrow will arc
		            fTime = fDist2D / (pWp->getProjectileSpeed() * 0.707);

		            if (rcbot_supermode.GetBool())
		                *v_desired_offset = *v_desired_offset + ((vVelocity * fTime));
		            else
		                *v_desired_offset = *v_desired_offset + ((vVelocity * fTime) * m_pProfile->m_fAimSkill);

		            if (sv_gravity.IsValid())
		                v_desired_offset->z += ((pow(2, fTime) - 1.0f)
		                                        * (sv_gravity.GetFloat() * 0.1f)); // - (getOrigin().z - v_origin.z);

		            v_desired_offset->z *= 0.6f;
		        }
		    }
		    else if ((v_desired_offset->z < 64.0f) && !hasSomeConditions(CONDITION_SEE_ENEMY_GROUND))
		    {
		        if (rcbot_supermode.GetBool())
		            v_desired_offset->z += 15.0f;
		        else
		            v_desired_offset->z += randomFloat(0.0f, 16.0f);
		    }
		}*/

		CWeapon *pWepInfo = pWp->getWeaponInfo();
		if (pWepInfo->isProjectile())
		{
			//			int iSpeed = 0;

			/*switch (pWp->getID())
			{
			case TF2_WEAPON_ROCKETLAUNCHER:
			// fall through
			case TF2_WEAPON_COWMANGLER:
			case TF2_WEAPON_FLAREGUN:
			case TF2_WEAPON_GRENADELAUNCHER:
			{*/
			CClient *pClient = CClients::get(pEntity);
			Vector vVelocity;

			// if ( iSpeed == 0 )
			//	iSpeed = TF2_GRENADESPEED;

			// if ( pClient )
			//{
			if (CClassInterface::getVelocity(pEntity, &vVelocity))
			{
				if (pClient && (vVelocity == Vector(0, 0, 0)))
					vVelocity = pClient->getVelocity();
			}
			else if (pClient)
				vVelocity = pClient->getVelocity();

			// speed = distance/time
			// .'.
			// time = distance/speed

			bool bIsGrenade        = pWepInfo->isGrenade();
			float fProjectileSpeed = pWepInfo->getProjectileSpeed();

			if (fProjectileSpeed > 0.f)
			{
				if (bIsGrenade)
					fTime = fDist2D / (fProjectileSpeed * 0.707);
				else
					fTime = fDist / fProjectileSpeed;

				// Record enemy movement for statistical prediction
				recordEnemyMovement(pEntity);

				// Statistical prediction: analyze movement patterns instead of naive velocity projection
				Vector vStatPred = predictEnemyOffset(pEntity, fTime, m_pProfile->m_fAimSkill);
				if (vStatPred.Length2D() > 0.1f)
					*v_desired_offset = *v_desired_offset + vStatPred;
				else
					*v_desired_offset = *v_desired_offset + ((vVelocity * fTime));

				// Don't overpredict into a wall if the target will be stopped by it
				Vector vPredPos = v_origin + (vStatPred.Length2D() > 0.1f ? vStatPred : (vVelocity * fTime));
				CTraceFilterWorldAndPropsOnly filter;
				CBotGlobals::traceLine(v_origin, vPredPos, MASK_SOLID_BRUSHONLY, &filter);
				if (CBotGlobals::getTraceResult()->fraction < 1.0f)
				{
					Vector vHit  = CBotGlobals::getTraceResult()->endpos;
					Vector vUsed = (vStatPred.Length2D() > 0.1f) ? vStatPred : (vVelocity * fTime);
					*v_desired_offset = *v_desired_offset - vUsed + (vHit - v_origin);
				}

				if ((sv_gravity.IsValid()) && bIsGrenade)
					v_desired_offset->z +=
					    ((pow(2, fTime) - 1.0f) * (sv_gravity.GetFloat() * 0.1f)); // - (getOrigin().z - v_origin.z);

				/*if ((pWp->getID() == TF2_WEAPON_GRENADELAUNCHER) && hasSomeConditions(CONDITION_SEE_ENEMY_GROUND))
				    v_desired_offset->z -= randomFloat(8.0f, 32.0f); // aim for ground - with grenade launcher
				else if ((pWp->getID() == TF2_WEAPON_ROCKETLAUNCHER) || (pWp->getID() == TF2_WEAPON_COWMANGLER)
				         || (v_origin.z > (getOrigin().z + 16.0f)))*/
				v_desired_offset->z += randomFloat(8.0f, 32.0f); // aim for body, not ground
				                                                 /*}
				                                                 break;
				                                                 }*/
			}
		}

		if (m_iClass == TF_CLASS_HWGUY)
		{
			if (pWp->getID() == TF2_WEAPON_MINIGUN)
			{
				Vector vForward;
				Vector vRight;
				Vector vUp;

				QAngle eyes = m_pController ? m_pController->GetLocalAngles() : CBotGlobals::playerAngles(m_pEdict);

				// in fov? Check angle to edict
				AngleVectors(eyes, &vForward, &vRight, &vUp);

				*v_desired_offset =
				    *v_desired_offset + (((vRight * 24) - Vector(0, 0, 24)) * bot_heavyaimoffset.GetFloat());
			}
		}

		if (m_iClass == TF_CLASS_MEDIC)
		{
			if (pWp->getID() != TF2_WEAPON_CROSSBOW)
				v_desired_offset->z += sqrt(fDist) * 2;
		}

		if (m_iClass == TF_CLASS_SNIPER)
		{
			if (pWepInfo->getSlot() == 0)
			{
				Vector eye;
				gameclients->ClientEarPosition(pEntity, &eye);
				v_desired_offset->z = (eye - v_origin).z;
			}
		}
	}
}
/*
Vector CBotTF2::getAimVector ( edict_t *pEntity )
{
    static CBotWeapon *pWp;
    static float fDist;
    static float fTime;

    pWp = getCurrentWeapon();
    fDist = distanceFrom(pEntity);

    if ( m_fNextUpdateAimVector > engine->Time() )
    {
        //if ( m_bPrevAimVectorValid && bot_aimsmoothing.GetBool() )
        //	return
BOTUTIL_SmoothAim(m_vPrevAimVector,m_vAimVector,m_fStartUpdateAimVector,engine->Time(),m_fNextUpdateAimVector);

        return m_vAimVector;
    }

    Vector vAim = CBot::getAimVector(pEntity);

    if ( pWp )
    {

        if ( m_iClass == TF_CLASS_MEDIC )
        {
            if ( pWp->getID() == TF2_WEAPON_SYRINGEGUN )
                vAim = vAim + Vector(0,0,sqrt(fDist)*2);
        }
        else if ( m_iClass == TF_CLASS_HWGUY )
        {
            if ( pWp->getID() == TF2_WEAPON_MINIGUN )
            {

                Vector vForward;
                Vector vRight;
                QAngle eyes;

                eyes = eyeAngles();

                // in fov? Check angle to edict
                AngleVectors(eyes,&vForward);
                vForward = vForward.NormalizeInPlace();

                vRight = vForward.Cross(Vector(0,0,1));

                vAim = vAim + (((vRight * 24) - Vector(0,0,24))* bot_heavyaimoffset.GetFloat());
            }
        }
        else if ( (m_iClass == TF_CLASS_SOLDIER) || (m_iClass == TF_CLASS_DEMOMAN) )
        {
            int iSpeed = 0;

            switch ( pWp->getID() )
            {
                case TF2_WEAPON_ROCKETLAUNCHER:
                {
                    iSpeed = TF2_ROCKETSPEED;

                    if ( vAim.z <= getOrigin().z )
                        vAim = vAim - Vector(0,0,randomFloat(8.0f,24.0f));
                }
                // fall through
                case TF2_WEAPON_GRENADELAUNCHER:
                {
                    CClient *pClient = CClients::get(pEntity);
                    Vector vVelocity;

                    if ( iSpeed == 0 )
                        iSpeed = TF2_GRENADESPEED;

                    if ( pClient )
                    {
                        if ( CClassInterface::getVelocity(pEntity,&vVelocity) )
                        {
                            if ( pClient && (vVelocity == Vector(0,0,0)) )
                                vVelocity = pClient->getVelocity();
                        }
                        else if ( pClient )
                            vVelocity = pClient->getVelocity();

                        // speed = distance/time
                        // .'.
                        // time = distance/speed
                        fTime = fDist/iSpeed;

                        vAim = vAim + ((vVelocity*fTime)*m_pProfile->m_fAimSkill );
                    }

                    if ( pWp->getID() == TF2_WEAPON_GRENADELAUNCHER )
                        vAim = vAim + Vector(0,0,sqrt(fDist));
		}

		// Wrangler: only hold it when the sentry can actually hit the target
		if (m_pSentryGun.get() != nullptr)
		{
			edict_t *pSentry = m_pSentryGun.get();
			if (CBotGlobals::entityIsValid(pSentry) && CBotGlobals::entityIsAlive(pSentry)
			    && distanceFrom(pSentry) < 256.0f)
			{
				edict_t *pSentryEnemy = CClassInterface::getSentryEnemy(pSentry);
				if (pSentryEnemy
				    && CBotGlobals::isAlivePlayer(pSentryEnemy)
				    && (CBotGlobals::entityOrigin(pSentry) - CBotGlobals::entityOrigin(pSentryEnemy)).Length()
				        < (float)TF2_MAX_SENTRYGUN_RANGE
				    && CBotGlobals::isVisible(pSentry, CBotGlobals::entityOrigin(pSentry), pSentryEnemy)
				    && (CClassInterface::getTF2SentryShells(pSentry) > 0
				        || CClassInterface::getTF2SentryRockets(pSentry) > 0))
				{
					CBotWeapon *pWrangler = m_pWeapons->getWeapon(
					    CWeapons::getWeapon(TF2_WEAPON_WRANGLER));
					if (pWrangler && pWrangler->hasWeapon()
					    && getCurrentWeapon() != pWrangler)
						select_CWeapon(pWrangler->getWeaponInfo());
				}
			}
		}

		break;
            }
        }
    }

    m_fStartUpdateAimVector = engine->Time();
    m_vAimVector = vAim;

    return m_vAimVector;
}
*/
void CBotTF2::checkDependantEntities()
{
	CBotFortress::checkDependantEntities();
}

eBotFuncState CBotTF2::rocketJump(int *iState, float *fTime)
{
	setLookAtTask(LOOK_GROUND);
	m_bIncreaseSensitivity = true;

	switch (*iState)
	{
	case 0:
	{
		if ((getSpeed() > 100) && (CBotGlobals::playerAngles(m_pEdict).x > 86.0f))
		{
			m_pButtons->tap(IN_JUMP);
			*iState = *iState + 1;
			*fTime  = engine->Time() + bot_rj.GetFloat(); // randomFloat(0.08,0.5);

			return BOT_FUNC_CONTINUE;
		}
	}
	break;
	case 1:
	{
		if (*fTime < engine->Time())
		{
			m_pButtons->tap(IN_ATTACK);

			return BOT_FUNC_COMPLETE;
		}
	}
	break;
	}

	return BOT_FUNC_CONTINUE;
}

// return true if the enemy is ok to shoot, return false if there is a problem (e.g. weapon problem)
bool CBotTF2::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy)
{
	static float fDistance;

	if (!pEnemy)
		return false;

	fDistance = distanceFrom(pEnemy);

	if ((fDistance > 128) && (m_vAimVector.Length() > 0.1f)
	    && (DotProductFromOrigin(m_vAimVector) < rcbot_enemyshootfov.GetFloat()))
		return true; // keep enemy / don't shoot : until angle between enemy is less than 45 degrees

	// Only avoid visible sentries that can actually shoot us.
	// If behind a wall, no need to avoid. If we have no ranged weapon, rush or path around.
	if (!(m_iClass == TF_CLASS_SPY && (isDisguised() || isCloaked()))
	    && !((CClassInterface::getTF2Conditions(m_pEdict) & TF2_PLAYER_BONKED) == TF2_PLAYER_BONKED)
	    && !CTeamFortress2Mod::TF2_IsPlayerInvuln(m_pEdict))
	{
		edict_t *pDangerSentry = m_pNearestEnemySentry.get();
		if (pDangerSentry && CBotGlobals::entityIsValid(pDangerSentry)
		    && CBotGlobals::entityIsAlive(pDangerSentry) && isVisible(pDangerSentry))
		{
			float fSentryDist = distanceFrom(pDangerSentry);
			if (fSentryDist < TF2_MAX_SENTRYGUN_RANGE && !m_pSchedules->hasSchedule(SCHED_ATTACK_SENTRY_GUN)
			    && !m_pSchedules->isCurrentSchedule(SCHED_ATTACK_SENTRY_GUN))
			{
				CBotWeapon *pCheckRanged = m_pWeapons->getBestWeapon(pDangerSentry, false, false);
				if (pCheckRanged && !pCheckRanged->isMelee() && !pCheckRanged->outOfAmmo(this))
				{
					Vector vSentryPos = CBotGlobals::entityOrigin(pDangerSentry);
					Vector vAway      = getOrigin() - vSentryPos;
					vAway.z           = 0;
					if (vAway.Length() > 0.1f)
				{
					vAway = vAway / vAway.Length();
					setMoveTo(getOrigin() + (vAway * 384.0f));
				}
			}
		}
	}
	}

	// Zigzag against scoped snipers to avoid headshots -- check all players, not just current enemy
	{
		bool bSniperThreat = false;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t *pPlayer = INDEXENT(i);
			if (!pPlayer || !CBotGlobals::entityIsValid(pPlayer) || !CBotGlobals::entityIsAlive(pPlayer))
				continue;
			if (CTeamFortress2Mod::getTeam(pPlayer) == m_iTeam)
				continue;
			if (CClassInterface::getTF2Class(pPlayer) != TF_CLASS_SNIPER)
				continue;
			if (!CTeamFortress2Mod::TF2_IsPlayerZoomed(pPlayer))
				continue;

			float fDist = distanceFrom(pPlayer);
			if (fDist <= 128.0f || fDist >= 4000.0f)
				continue;

			Vector vToSniper = CBotGlobals::entityOrigin(pPlayer) - getOrigin();
			vToSniper.z      = 0;
			float fLen       = vToSniper.Length();
			if (fLen <= 0.1f)
				continue;
			Vector vToSniperNorm = vToSniper / fLen;

			Vector vSniperAim;
			AngleVectors(CBotGlobals::entityEyeAngles(pPlayer), &vSniperAim);
			vSniperAim.z = 0;
			float fAimLen = vSniperAim.Length();
			if (fAimLen <= 0.1f)
				continue;
			vSniperAim = vSniperAim / fAimLen;

			if (vSniperAim.Dot(vToSniperNorm) > 0.7f)
			{
				bSniperThreat = true;
				break;
			}
		}

		if (bSniperThreat)
		{
			m_fStrafeTime = engine->Time() + 0.3f;

			if (m_fAvoidSideSwitch < engine->Time())
			{
				m_fAvoidSideSwitch = engine->Time() + randomFloat(0.25f, 0.35f);
				m_bAvoidRight      = !m_bAvoidRight;
			}

			m_fSideSpeed = m_bAvoidRight ? m_fIdealMoveSpeed : -m_fIdealMoveSpeed;

			Vector vForward;
			AngleVectors(m_vViewAngles, &vForward);
			vForward.z = 0;
			setMoveTo(getOrigin() + vForward * 128.0f);
		}
	}

	/* Handle Spy Attacking Choice here */
	if (m_iClass == TF_CLASS_SPY)
	{
		/*if ( isCloaked() )
		    return false;
		else*/
		if (isDisguised())
		{
			if (((fDistance < rcbot_tf2_spy_kill_on_cap_dist.GetFloat()) && CTeamFortress2Mod::isCapping(pEnemy))
			    || ((fDistance < 130) && CBotGlobals::isAlivePlayer(pEnemy)
			        && (fabs(CBotGlobals::yawAngleFromEdict(pEnemy, getOrigin())) > bot_spyknifefov.GetFloat())))
			{
				; // ok attack
			}
			else if (m_fFrenzyTime < engine->Time())
				return true; // return but don't attack
			else if (CBotGlobals::isPlayer(pEnemy) && (CClassInterface::getTF2Class(pEnemy) == TF_CLASS_ENGINEER)
			         && (CTeamFortress2Mod::isMySentrySapped(pEnemy) || CTeamFortress2Mod::isMyTeleporterSapped(pEnemy)
			             || CTeamFortress2Mod::isMyDispenserSapped(pEnemy)))
			{
				return true; // return but don't attack
			}
		}
	}

	if (pWeapon)
	{
		Vector vEnemyOrigin;
		bool bSecAttack = false;
		bool bIsPlayer  = false;

		clearFailedWeaponSelect();

		if (pWeapon->isMelee() && !(CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(pEnemy)))
		{
			Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);

			// Strafe around enemy with close-range weapons instead of walking straight at them
			if (m_fAvoidSideSwitch < engine->Time())
			{
				m_fAvoidSideSwitch = engine->Time() + randomFloat(1.5f, 2.5f);
				m_bAvoidRight      = !m_bAvoidRight;
			}

			Vector vToEnemy     = vEnemyOrigin - getOrigin();
			vToEnemy.z          = 0;
			float flDistToEnemy = vToEnemy.Length();

			if (flDistToEnemy > 64.0f)
			{
				// Approach from alternating sides to make it harder for enemy to hit us
				if (flDistToEnemy > 0.1f)
				{
					Vector vToEnemyNorm = vToEnemy / flDistToEnemy;
					Vector vLeft        = vToEnemyNorm.Cross(Vector(0, 0, 1));
					if (vLeft.Length() > 0.1f)
					{
						vLeft           = vLeft / vLeft.Length();
						float fStrafeOff = bot_avoid_strength.GetFloat();
						if (m_bAvoidRight)
							setMoveTo(vEnemyOrigin + (vLeft * fStrafeOff));
						else
							setMoveTo(vEnemyOrigin - (vLeft * fStrafeOff));
					}
				}
			}
			else
			{
				setMoveTo(vEnemyOrigin);
			}

			// setLookAt(m_vAimVector);
			setLookAtTask(LOOK_ENEMY);
			// dontAvoid my enemy
			m_fAvoidTime = engine->Time() + 1.0f;
		}

		// MvM: tank boss -- bots should focus the tank with optimal weapon
		if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(pEnemy))
		{
			Vector vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);

			// Force class-optimal anti-tank weapon before any distance checks
			if (m_iClass == TF_CLASS_PYRO)
			{
				CBotWeapon *pFlame = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
				if (pFlame && pFlame->hasWeapon() && !pFlame->outOfAmmo(this) && getCurrentWeapon() != pFlame)
					select_CWeapon(pFlame->getWeaponInfo());
			}
			else if (m_iClass == TF_CLASS_SOLDIER)
			{
				CBotWeapon *pRocket = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER));
				if (pRocket && pRocket->hasWeapon() && !pRocket->outOfAmmo(this) && getCurrentWeapon() != pRocket)
					select_CWeapon(pRocket->getWeaponInfo());
			}
			else if (m_iClass == TF_CLASS_DEMOMAN)
			{
				CBotWeapon *pGren = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));
				if (pGren && pGren->hasWeapon() && !pGren->outOfAmmo(this) && getCurrentWeapon() != pGren)
					select_CWeapon(pGren->getWeaponInfo());
				else
				{
					CBotWeapon *pSticky = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
					if (pSticky && pSticky->hasWeapon() && !pSticky->outOfAmmo(this)
					    && pSticky->getClip1(this) > 0 && getCurrentWeapon() != pSticky)
						select_CWeapon(pSticky->getWeaponInfo());
				}
			}
			else if (m_iClass == TF_CLASS_HWGUY)
			{
				CBotWeapon *pMini = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_MINIGUN));
				if (pMini && pMini->hasWeapon() && !pMini->outOfAmmo(this) && getCurrentWeapon() != pMini)
					select_CWeapon(pMini->getWeaponInfo());
			}
			else if (pWeapon->isMelee() && !pWeapon->isSpecial())
			{
				CBotWeapon *pPrimary = m_pWeapons->getPrimaryWeapon();
				if (pPrimary && pPrimary->hasWeapon() && !pPrimary->outOfAmmo(this) && pPrimary != pWeapon)
					select_CWeapon(pPrimary->getWeaponInfo());
			}

			// Find closest non-tank enemy to use the tank as cover
		edict_t *pThreat = nullptr;
		float fThreatDist = 800.0f;
		{
		for (int i = 1; i <= CBotGlobals::maxClients(); i++)
		{
			edict_t *p = INDEXENT(i);
			if (!p || p == m_pEdict || p->IsFree() || !p->GetUnknown()) continue;
			if (!CBotGlobals::entityIsValid(p) || !CBotGlobals::entityIsAlive(p)) continue;
			if (!isEnemy(p) || CTeamFortress2Mod::isTankBoss(p)) continue;
			float d = (CBotGlobals::entityOrigin(p) - getOrigin()).Length();
			if (d < fThreatDist) { pThreat = p; fThreatDist = d; }
		}
		}

			if (m_iClass == TF_CLASS_SOLDIER || m_iClass == TF_CLASS_DEMOMAN)
			{
				const float fCoverDist = 350.0f;
				if (pThreat)
				{
					Vector vAway = vEnemyOrigin - CBotGlobals::entityOrigin(pThreat);
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(vEnemyOrigin + vAway * fCoverDist);
					}
					else
						setMoveTo(vEnemyOrigin);
				}
				else
				{
				const float fMinDist = 300.0f;
				if (fDistance < fMinDist)
				{
					Vector vAway = getOrigin() - vEnemyOrigin;
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(getOrigin() + (vAway * fMinDist));
					}
					if (fDistance < 128.0f && pWeapon->isExplosive())
						return true;
				}
				else if (fDistance > fMinDist + 200.0f)
				{
					setMoveTo(vEnemyOrigin);
				}
				}

				if (m_iClass == TF_CLASS_DEMOMAN)
				{
					CBotWeapon *pGrenadeLauncher = m_pWeapons->getWeapon(
					    CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));
					CBotWeapon *pStickyLauncher = m_pWeapons->getWeapon(
					    CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

					bool bNoPrimary = !pGrenadeLauncher || !pGrenadeLauncher->hasWeapon()
					                  || pGrenadeLauncher->outOfAmmo(this);

					if (bNoPrimary
					    && pStickyLauncher && pStickyLauncher->hasWeapon()
					    && !pStickyLauncher->outOfAmmo(this) && pStickyLauncher->getClip1(this) > 0
					    && fDistance > 128.0f
					    && fDistance < pStickyLauncher->getPrimaryMaxRange())
					{
						if (getCurrentWeapon() != pStickyLauncher)
							select_CWeapon(pStickyLauncher->getWeaponInfo());
						else if (!m_bStickyCharging && m_fStickyDetTime == 0.0f)
						{
							primaryAttack(true);
							m_fStickyChargeStart = engine->Time();
							m_bStickyCharging    = true;
						}
						else if (m_bStickyCharging)
						{
							float fCharge = fDistance / pStickyLauncher->getPrimaryMaxRange();
							fCharge       = fCharge * fCharge * 2.0f;
							if (fCharge < 0.03f) fCharge = 0.03f;
							if (fCharge > 2.0f) fCharge  = 2.0f;

							float fElapsed = engine->Time() - m_fStickyChargeStart;
							if (fDistance <= 200.0f || fElapsed >= fCharge)
							{
								m_pButtons->letGo(IN_ATTACK);
								m_bStickyCharging = false;

								edict_t *pStickyEnt = pStickyLauncher->getWeaponEntity();
								int iItem = pStickyEnt
								    ? CClassInterface::TF2_getItemDefinitionIndex(pStickyEnt) : 0;
								float fArmTime = 0.8f;
								if (iItem == 1150)       fArmTime = 0.6f;
								else if (iItem == 130)   fArmTime = 1.6f;

								float fFlyTime = fDistance / 700.0f;
								m_fStickyDetTime = engine->Time() + fFlyTime + fArmTime;
							}
						}
						else if (m_fStickyDetTime < engine->Time())
						{
							if (fDistance < BLAST_RADIUS
							    && distanceFrom(CBotGlobals::entityOrigin(m_pEnemy.get())) > (BLAST_RADIUS / 2))
							{
								tapButton(IN_ATTACK2);
								m_fStickyDetTime = 0.0f;
							}
							else
								m_fStickyDetTime = engine->Time() + 0.1f;
						}
					}
				}
			}
			else if (m_iClass == TF_CLASS_PYRO)
			{
				if (fDistance > 200.0f)
				{
					setMoveTo(vEnemyOrigin);
				}
				else
				{
					const float fCoverDist = 200.0f;
					if (pThreat)
					{
						Vector vAway = vEnemyOrigin - CBotGlobals::entityOrigin(pThreat);
						vAway.z = 0;
						if (vAway.Length() > 0.1f)
						{
							vAway = vAway / vAway.Length();
							setMoveTo(vEnemyOrigin + vAway * fCoverDist);
						}
						else
							setMoveTo(vEnemyOrigin);
					}
					else
					{
						if (m_fAvoidSideSwitch < engine->Time())
						{
							m_fAvoidSideSwitch = engine->Time() + randomFloat(1.5f, 2.5f);
							m_bAvoidRight      = !m_bAvoidRight;
						}
						Vector vToTank       = vEnemyOrigin - getOrigin();
						vToTank.z            = 0;
						float flDistToTank   = vToTank.Length();
						if (flDistToTank > 0.1f)
						{
							Vector vToTankNorm = vToTank / flDistToTank;
							Vector vLeft       = vToTankNorm.Cross(Vector(0, 0, 1));
							if (vLeft.Length() > 0.1f)
							{
								vLeft           = vLeft / vLeft.Length();
								float fStrafeOff = bot_avoid_strength.GetFloat() * 1.2f;
								if (m_bAvoidRight)
									setMoveTo(vEnemyOrigin + (vLeft * fStrafeOff));
								else
									setMoveTo(vEnemyOrigin - (vLeft * fStrafeOff));
							}
						}
					}
				}
			}
			else if (m_iClass == TF_CLASS_HWGUY)
			{
				const float fCoverDist = 250.0f;
				if (pThreat)
				{
					Vector vAway = vEnemyOrigin - CBotGlobals::entityOrigin(pThreat);
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(vEnemyOrigin + vAway * fCoverDist);
					}
					else
						setMoveTo(vEnemyOrigin);
				}
				else
				{
				const float fMinDist = 200.0f;
				if (fDistance < fMinDist)
				{
					Vector vAway = getOrigin() - vEnemyOrigin;
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(getOrigin() + (vAway * fMinDist));
					}
				}
				else if (fDistance > fMinDist + 250.0f)
				{
					setMoveTo(vEnemyOrigin);
				}
				}
			}
			else if (m_iClass == TF_CLASS_SCOUT)
			{
				const float fCoverDist = 150.0f;
				if (pThreat)
				{
					Vector vAway = vEnemyOrigin - CBotGlobals::entityOrigin(pThreat);
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(vEnemyOrigin + vAway * fCoverDist);
					}
					else
						setMoveTo(vEnemyOrigin);
				}
				else if (fDistance > 200.0f)
					setMoveTo(vEnemyOrigin);
			}
			else
			{
				const float fCoverDist = 250.0f;
				if (pThreat)
				{
					Vector vAway = vEnemyOrigin - CBotGlobals::entityOrigin(pThreat);
					vAway.z = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway = vAway / vAway.Length();
						setMoveTo(vEnemyOrigin + vAway * fCoverDist);
					}
					else
						setMoveTo(vEnemyOrigin);
				}
				else
				{
				const float fCloseDist = 250.0f;
				if (fDistance > fCloseDist)
					setMoveTo(vEnemyOrigin);
				}
			}
		}

		// Dodge incoming projectiles if one is heading at us
		if (m_fStrafeTime < engine->Time())
		{
			edict_t *pIncoming = m_NearestEnemyRocket.get();
			if (!pIncoming)
				pIncoming = m_pNearestPipeGren.get();

			if (pIncoming && CBotGlobals::entityIsValid(pIncoming)
		    && CBotGlobals::entityIsAlive(pIncoming) && incomingRocket(800.0f))
			{
				Vector vOrigin  = getOrigin();
				Vector vProjOrg = CBotGlobals::entityOrigin(pIncoming);
				Vector vDodge   = vOrigin - vProjOrg;
				vDodge.z        = 0;
				float fLen      = vDodge.Length2D();
				if (fLen > 0.1f)
				{
					vDodge = vDodge / fLen;
					Vector vDest = vOrigin + vDodge * (BLAST_RADIUS + 128.0f);

				// Trace-validate dodge destination; try lateral alternatives if blocked
				CTraceFilterWorldAndPropsOnly filter;
				CBotGlobals::traceLine(vOrigin, vDest, MASK_SOLID_BRUSHONLY, &filter);
				trace_t *tr = CBotGlobals::getTraceResult();
				if (tr->fraction < 1.0f)
				{
					Vector vAlt1(-vDodge.y, vDodge.x, 0);
					Vector vAlt2(vDodge.y, -vDodge.x, 0);
					Vector vDest1 = vOrigin + vAlt1 * (BLAST_RADIUS + 128.0f);
					Vector vDest2 = vOrigin + vAlt2 * (BLAST_RADIUS + 128.0f);
					CBotGlobals::traceLine(vOrigin, vDest1, MASK_SOLID_BRUSHONLY, &filter);
					tr = CBotGlobals::getTraceResult();
					if (tr->fraction < 1.0f)
					{
						CBotGlobals::traceLine(vOrigin, vDest2, MASK_SOLID_BRUSHONLY, &filter);
						tr    = CBotGlobals::getTraceResult();
						vDest = (tr->fraction >= 1.0f) ? vDest2 : vOrigin;
					}
					else
						vDest = vDest1;
				}

				setMoveTo(vDest);
					m_fStrafeTime = engine->Time() + 0.2f;
					m_fSideSpeed  = (vDodge.y > 0 ? 1.0f : -1.0f) * m_fIdealMoveSpeed;
				}
			}
		}

		// Demoman: use sticky launcher as combat weapon when grenade launcher is dry
		// Only outside melee range and inside sticky launcher range
		if (m_iClass == TF_CLASS_DEMOMAN)
		{
			CBotWeapon *pGrenadeLauncher = m_pWeapons->getWeapon(
			    CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER));
			CBotWeapon *pStickyLauncher = m_pWeapons->getWeapon(
			    CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));

			bool bNoPrimary = !pGrenadeLauncher || !pGrenadeLauncher->hasWeapon()
			                  || pGrenadeLauncher->outOfAmmo(this);

			// If enemy is too close for sticky, force melee instead of staying stuck
			if (getCurrentWeapon() == pStickyLauncher && fDistance <= 128.0f)
			{
				CBotWeapon *pMelee = getBestWeapon(m_pEnemy, true, true, true);
				if (pMelee && pMelee->isMelee())
					select_CWeapon(pMelee->getWeaponInfo());
			}
			// Don't override if getBestWeapon already chose melee
			else if (bNoPrimary && !pWeapon->isMelee()
			    && pStickyLauncher && pStickyLauncher->hasWeapon()
			    && !pStickyLauncher->outOfAmmo(this) && pStickyLauncher->getClip1(this) > 0
			    && fDistance > 128.0f
			    && fDistance < pStickyLauncher->getPrimaryMaxRange())
			{
				if (getCurrentWeapon() != pStickyLauncher)
					select_CWeapon(pStickyLauncher->getWeaponInfo());
				else if (!m_bStickyCharging && m_fStickyDetTime == 0.0f)
				{
					// Start continuous charge — release later based on live distance
					primaryAttack(true);
					m_fStickyChargeStart = engine->Time();
					m_bStickyCharging    = true;
				}
				else if (m_bStickyCharging)
				{
					// Recompute ideal charge based on current distance each frame
					float fCharge = fDistance / pStickyLauncher->getPrimaryMaxRange();
					fCharge       = fCharge * fCharge * 2.0f;
					if (fCharge < 0.03f) fCharge = 0.03f;
					if (fCharge > 2.0f) fCharge  = 2.0f;

					float fElapsed = engine->Time() - m_fStickyChargeStart;
					// Release if enemy moved close enough or charge reached
					if (fDistance <= 200.0f || fElapsed >= fCharge)
					{
						m_pButtons->letGo(IN_ATTACK);
						m_bStickyCharging = false;

						// Arm time by item
						edict_t *pStickyEnt = pStickyLauncher->getWeaponEntity();
						int iItem = pStickyEnt
						    ? CClassInterface::TF2_getItemDefinitionIndex(pStickyEnt) : 0;
						float fArmTime = 0.8f;
						if (iItem == 1150)       fArmTime = 0.6f;
						else if (iItem == 130)   fArmTime = 1.6f;

						float fFlyTime = fDistance / 700.0f;
						m_fStickyDetTime = engine->Time() + fFlyTime + fArmTime;
					}
				}
				else if (m_fStickyDetTime < engine->Time())
				{
					// Detonate only if enemy is within blast radius and we're safe
					if (fDistance < BLAST_RADIUS
					    && distanceFrom(CBotGlobals::entityOrigin(m_pEnemy.get())) > (BLAST_RADIUS / 2))
					{
						tapButton(IN_ATTACK2);
						m_fStickyDetTime = 0.0f;
					}
					else
						m_fStickyDetTime = engine->Time() + 0.1f; // recheck soon
				}
			}
		}

		// Degreaser quick-switch: if holding secondary and projectile incoming,
		// quickly swap to Degreaser to reflect it, then swap back
		if (m_iClass == TF_CLASS_PYRO && !pWeapon->canDeflectRockets())
		{
			CBotWeapon *pDegreaser = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));
			edict_t *pFlameEnt     = pDegreaser ? pDegreaser->getWeaponEntity() : nullptr;
			int iDegItem   = pFlameEnt ? CClassInterface::TF2_getItemDefinitionIndex(pFlameEnt) : 0;

			if (iDegItem == 215 && pDegreaser->hasWeapon() && pDegreaser->getAmmo(this) >= 25)
			{
				edict_t *pProj = m_NearestEnemyRocket.get();
				float fProjDist = pProj ? distanceFrom(pProj) : 9999.0f;
				if (!pProj || fProjDist > 400.0f)
				{
					pProj = m_pNearestPipeGren.get();
					if (pProj) fProjDist = distanceFrom(pProj);
				}

				if (pProj && fProjDist > 80.0f && fProjDist < 400.0f)
				{
					// Save current weapon and switch to Degreaser
					if (m_iDegreaserPrevSlot == 0)
					{
						CBotWeapon *pCur = getCurrentWeapon();
						if (pCur && pCur->getWeaponInfo())
							m_iDegreaserPrevSlot = pCur->getWeaponInfo()->getSlot();
						else
							m_iDegreaserPrevSlot = 1;
						m_fDegreaserSwapBack = engine->Time() + 1.2f;
					}
					select_CWeapon(pDegreaser->getWeaponInfo());
				}
			}
		}

		// Swap back from Degreaser to previous weapon
		if (m_fDegreaserSwapBack > 0 && m_fDegreaserSwapBack < engine->Time())
		{
			m_fDegreaserSwapBack = 0;
			if (m_iDegreaserPrevSlot > 0)
			{
				CBotWeapon *pPrev = m_pWeapons->getCurrentWeaponInSlot(m_iDegreaserPrevSlot);
				if (pPrev && pPrev->hasWeapon() && pPrev != getCurrentWeapon())
					select_CWeapon(pPrev->getWeaponInfo());
				m_iDegreaserPrevSlot = 0;
			}
		}

		// Extinguish burning teammates -- reuse shared logic
		if (!bSecAttack && m_iClass == TF_CLASS_PYRO)
		{
			if (tryExtinguishTeammates())
				bSecAttack = true;
		}

		// Airblast: reflect incoming projectiles even when targeting someone else
		if (pWeapon->canDeflectRockets() && m_iClass == TF_CLASS_PYRO)
		{
			edict_t *pFlameEnt = CClassInterface::getCurrentWeapon(m_pEdict);
			int iItemDef        = pFlameEnt ? CClassInterface::TF2_getItemDefinitionIndex(pFlameEnt) : 0;
			int iMinAmmo        = 20;

			if (iItemDef == 594)       // Phlogistinator -- no airblast
				iMinAmmo = 9999;
			else if (iItemDef == 40)   // Backburner -- costs 50
				iMinAmmo = 50;
			else if (iItemDef == 215)  // Degreaser -- costs 20, user wants 25 buffer
				iMinAmmo = 25;
			else if (iItemDef == 1178 || iItemDef == 1099) // Dragon's Fury -- costs 5
				iMinAmmo = 5;

			if (pWeapon->getAmmo(this) >= iMinAmmo)
			{
				edict_t *pProj = m_NearestEnemyRocket.get();
				float fProjDist = pProj ? distanceFrom(pProj) : 9999.0f;

				if (!pProj || fProjDist > 400.0f)
				{
					pProj = m_pNearestPipeGren.get();
					if (pProj)
						fProjDist = distanceFrom(pProj);
				}

				if (pProj && fProjDist > 80.0f && fProjDist < 400.0f)
					bSecAttack = true;
			}
		}

		// Airblast an enemy rocket, ubered player, or capping/defending player if on fire
		if (!bSecAttack)
		{
			bool bPhlog = false;
			{
				edict_t *pFlameEnt = CClassInterface::getCurrentWeapon(m_pEdict);
				bPhlog = (pFlameEnt && CClassInterface::TF2_getItemDefinitionIndex(pFlameEnt) == 594);
			}

			if ((pEnemy == m_NearestEnemyRocket.get()) || (pEnemy == m_pNearestPipeGren.get())
			    || (((bIsPlayer = CBotGlobals::isPlayer(pEnemy)) == true)
			        && (CTeamFortress2Mod::TF2_IsPlayerInvuln(pEnemy)
			            || (CTeamFortress2Mod::TF2_IsPlayerOnFire(pEnemy)
			                && ((CTeamFortress2Mod::isFlagCarrier(pEnemy))
			                    || (m_iCurrentDefendArea && CTeamFortress2Mod::isCapping(pEnemy))
			                    || (m_iCurrentAttackArea && CTeamFortress2Mod::isDefending(pEnemy)))))))
			{
				if ((bIsPlayer || (fDistance > 80)) && (fDistance < 400) && pWeapon->canDeflectRockets()
				    && !bPhlog
				    && (pWeapon->getAmmo(this) >= rcbot_tf2_pyro_airblast.GetInt()))
					bSecAttack = true;
				else if ((pEnemy == m_NearestEnemyRocket.get()) || (pEnemy == m_pNearestPipeGren.get()))
					return false; // don't attack the rocket anymore
			}
		}

		if (m_iClass == TF_CLASS_SNIPER && pWeapon->isProjectile())
		{
			// Bow: keep strafing while charging, never stand still
			if (m_fSnipeAttackTime > engine->Time())
			{
				if (m_fAvoidSideSwitch < engine->Time())
				{
					m_fAvoidSideSwitch = engine->Time() + randomFloat(1.5f, 2.5f);
					m_bAvoidRight      = !m_bAvoidRight;
				}
				Vector vToEnemy = CBotGlobals::entityOrigin(pEnemy) - getOrigin();
				vToEnemy.z       = 0;
				if (vToEnemy.Length2D() > 32.0f)
				{
					Vector vPerp = vToEnemy.Cross(Vector(0, 0, 1));
					if (vPerp.Length() > 0.1f)
					{
						vPerp = vPerp / vPerp.Length();
						float fOff = bot_avoid_strength.GetFloat();
						if (m_bAvoidRight)
							setMoveTo(getOrigin() + (vPerp * fOff));
						else
							setMoveTo(getOrigin() - (vPerp * fOff));
					}
				}
				primaryAttack(true);
			}
			else
			{
				float fDistFactor  = distanceFrom(pEnemy) / pWeapon->getPrimaryMaxRange();
				float fRandom      = randomFloat(m_pProfile->m_fAimSkill, 1.0f);
				float fSkill       = 1.0f - fRandom;

				m_fSnipeAttackTime = engine->Time() + ((4.0f * fDistFactor));
				m_pButtons->letGo(IN_ATTACK);
			}
		}
		else if (m_iClass == TF_CLASS_SNIPER && pWeapon->getWeaponInfo() && pWeapon->getWeaponInfo()->getSlot() == 0
		         && !pWeapon->isProjectile())
		{
			// stopMoving();

			if (m_fSnipeAttackTime < engine->Time())
			{
				if (CTeamFortress2Mod::TF2_IsPlayerZoomed(m_pEdict))
					primaryAttack(); // shoot
				else
					secondaryAttack(); // zoom

				m_fSnipeAttackTime = engine->Time() + randomFloat(0.1f, 0.8f);
			}
		}
		else if (!bSecAttack)
		{
			bool bHandled = false;

			// --- Pyro reflection adaptation: randomize firing delay at close range ---
			if (pEnemy && pWeapon->isExplosive()
			    && CClassInterface::getTF2Class(pEnemy) == TF_CLASS_PYRO
			    && distanceFrom(pEnemy) < 600.0f)
			{
				int iIdx = ENTINDEX(pEnemy) - 1;
				if (iIdx >= 0 && iIdx < MAX_PLAYERS)
				{
					// Reset on class change
					TF_Class iCur = (TF_Class)CClassInterface::getTF2Class(pEnemy);
					if (iCur != m_iLastKnownEnemyClass[iIdx])
					{
						m_iReflectCount[iIdx] = 0;
						m_fNextPyroShotTime[iIdx] = 0;
						m_iLastKnownEnemyClass[iIdx] = iCur;
					}

					// Only delay if Pyro has reflected before AND has ammo to airblast
					if (m_iReflectCount[iIdx] > 0)
					{
						edict_t *pPyroWep = CClassInterface::getCurrentWeapon(pEnemy);
						bool bHasFlame = false;
						bool bHasAmmo  = false;
						if (pPyroWep)
						{
							const char *szClass = pPyroWep->GetClassName();
							bHasFlame = (strstr(szClass, "flame") != nullptr);
							int *pAmmo = CClassInterface::getWeaponClip1Pointer(pPyroWep);
							bHasAmmo  = (pAmmo && *pAmmo >= 20);
						}

						if (bHasFlame && bHasAmmo && m_fNextPyroShotTime[iIdx] > engine->Time())
							bHandled = true;
					}
				}
			}

			// --- Beggar's Bazooka (730): hold to load, release at 3 rockets ---
			if (m_iClass == TF_CLASS_SOLDIER)
			{
				edict_t *pWepEnt = pWeapon->getWeaponEntity();
				if (pWepEnt && CClassInterface::TF2_getItemDefinitionIndex(pWepEnt) == 730)
				{
					if (pWeapon->getClip1(this) < 3)
						primaryAttack(true);
					else
						m_pButtons->letGo(IN_ATTACK);
					bHandled = true;
				}
			}

			if (!bHandled)
			{
				if (pWeapon->mustHoldAttack())
					primaryAttack(true);
				else
					primaryAttack();
			}

			// After firing at Pyro: schedule next firing delay
			if (!bHandled && pEnemy && pWeapon->isExplosive()
			    && CClassInterface::getTF2Class(pEnemy) == TF_CLASS_PYRO
			    && distanceFrom(pEnemy) < 600.0f)
			{
				int iIdx = ENTINDEX(pEnemy) - 1;
				if (iIdx >= 0 && iIdx < MAX_PLAYERS && m_iReflectCount[iIdx] > 0)
				{
					float fDelay = 0.3f + (m_iReflectCount[iIdx] - 1) * 0.2f;
					if (fDelay > 1.5f) fDelay = 1.5f;
					m_fNextPyroShotTime[iIdx] = engine->Time() + randomFloat(0.0f, fDelay);
				}
			}
		}
		else
		{
			tapButton(IN_ATTACK2);
		}

		vEnemyOrigin = CBotGlobals::entityOrigin(pEnemy);
		// enemy below me!
		if (pWeapon->isMelee() && (distanceFrom2D(pEnemy) < 64.0f) && (vEnemyOrigin.z < getOrigin().z)
		    && (vEnemyOrigin.z > (getOrigin().z - 128)))
		{
			duck();
		}

		if ((!pWeapon->isMelee() || pWeapon->isSpecial()) && pWeapon->outOfAmmo(this))
		{
			// Demoman: allow sticky launcher fallback even if grenade launcher is dry
			if (m_iClass == TF_CLASS_DEMOMAN && pWeapon->getWeaponInfo()
			    && pWeapon->getWeaponInfo()->getID() == TF2_WEAPON_GRENADELAUNCHER)
			{
				CBotWeapon *pSticky = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS));
				if (pSticky && pSticky->hasWeapon() && !pSticky->outOfAmmo(this) && pSticky->getClip1(this) > 0
				    && getCurrentWeapon() == pSticky)
				{
					// Already using sticky launcher, don't force weapon change
				}
				else
					return false; // change weapon/enemy
			}
			else
				return false; // change weapon/enemy
		}
	}
	else
		primaryAttack();

	m_pAttackingEnemy = pEnemy;

	return true;
}

int CBotFortress::getMetal()
{
	if (m_iClass == TF_CLASS_ENGINEER)
	{
		CBotWeapon *pWrench = m_pWeapons->getWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH));

		if (pWrench)
			return pWrench->getAmmo(this);
	}

	return 0;
}

bool CBotTF2::upgradeBuilding(edict_t *pBuilding, bool removesapper)
{
	Vector vOrigin      = CBotGlobals::entityOrigin(pBuilding);

	CBotWeapon *pWeapon = getCurrentWeapon();
	int iMetal          = 0;

	wantToListen(false);

	if (!pWeapon)
		return false;

	iMetal = pWeapon->getAmmo(this);

	if (pWeapon->getID() != TF2_WEAPON_WRENCH)
	{
		if (!select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)))
			return false;
	}
	else if (!removesapper && (iMetal == 0)) // finished / out of metal // dont need metal to remove sapper
		return true;
	else
	{
		clearFailedWeaponSelect();

		if (distanceFrom(vOrigin) > 85)
		{
			setMoveTo(vOrigin);
		}
		else
		{
			duck(true);
			primaryAttack();
		}
	}

	lookAtEdict(pBuilding);
	m_fLookSetTime = 0;
	setLookAtTask(LOOK_EDICT);
	m_fLookSetTime = engine->Time() + randomFloat(3.0, 8.0);

	return true;
}

void CBotFortress::teamFlagPickup()
{
	if (CTeamFortress2Mod::isMapType(TF_MAP_SD) && m_pSchedules->hasSchedule(SCHED_TF2_GET_FLAG))
		m_pSchedules->removeSchedule(SCHED_TF2_GET_FLAG);
}

void CBotTF2::roundWon(int iTeam, bool bFullRound)
{
	m_pSchedules->freeMemory();

	if (bFullRound)
		m_fChangeClassTime = engine->Time();

	updateCondition(CONDITION_PUSH);
	removeCondition(CONDITION_PARANOID);
	removeCondition(CONDITION_BUILDING_SAPPED);
	removeCondition(CONDITION_COVERT);
}

void CBotTF2::waitRemoveSap()
{
	// brief cooldown after removing a sapper before reacting to a re-sap
	m_fRemoveSapTime = engine->Time() + randomFloat(1.0f, 2.0f);
}

void CBotTF2::roundReset(bool bFullReset)
{
	m_pRedPayloadBomb          = nullptr;
	m_pBluePayloadBomb         = nullptr;
	m_fLastKnownTeamFlagTime   = 0.0f;
	m_iLastDeathArea           = -1;
	m_fLastDeathTime           = 0.0f;
	m_bEntranceVectorValid     = false;
	m_bSentryGunVectorValid    = false;
	m_bDispenserVectorValid    = false;
	m_bTeleportExitVectorValid = false;
	m_pPrevSpy                 = nullptr;
	m_pHeal                    = nullptr;
	m_pSentryGun               = nullptr;
	m_pDispenser               = nullptr;
	m_pTeleEntrance            = nullptr;
	m_pTeleExit                = nullptr;
	m_pAmmo                    = nullptr;
	m_pHealthkit               = nullptr;
	m_pNearestDisp             = nullptr;
	m_pNearestEnemySentry      = nullptr;
	m_pNearestAllySentry       = nullptr;
	m_pNearestEnemyTeleporter  = nullptr;
	m_pNearestEnemyDisp        = nullptr;
	m_pNearestPipeGren         = nullptr;
	m_pFlag                    = nullptr;
	m_pPrevSpy                 = nullptr;
	m_KnownSentries.clear();
	m_KnownEnemyTeleporters.clear();
	m_KnownEnemyDispensers.clear();

	// Clear movement prediction history
	for (int i = 0; i < 64; i++)
		m_EnemyMovement[i] = EnemyMoveHistory();

	m_iSentryKills             = 0;
	m_fSentryPlaceTime         = 0.0;
	m_fDispenserPlaceTime      = 0.0f;
	m_fDispenserHealAmount     = 0.0f;
	m_fTeleporterEntPlacedTime = 0.0f;
	m_fTeleporterExtPlacedTime = 0.0f;
	m_iTeleportedPlayers       = 0;

	flagReset();
	teamFlagReset();

	m_pNavigator->clear();

	m_iTeam = getTeam();
	// fix : reset current areas
	updateAttackDefendPoints();
	// CPoints::getAreas(getTeam(),&m_iCurrentDefendArea,&m_iCurrentAttackArea);

	// m_pPayloadBomb = nullptr;

	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
	{
		if (!CTeamFortress2Mod::wonLastRound(m_iTeam))
		{
			// lost - reset spawn time
			m_fSpawnTime          = engine->Time();
			m_fSentryPlaceTime    = 0.0f;
			m_fDispenserPlaceTime = 0.0f;
			spawnInit(); // bots will respawn
		}
	}
}

void CBotTF2::updateAttackDefendPoints()
{
	m_iTeam              = getTeam();
	m_iCurrentAttackArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_ATTACK);
	m_iCurrentDefendArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_DEFEND);
}

void CBotTF2::pointsUpdated()
{
	if (m_iClass == TF_CLASS_ENGINEER)
	{
		// m_pPayloadBomb = nullptr;
		bool bMoveSentry = (m_iSentryArea != m_iCurrentAttackArea) && (m_iSentryArea != m_iCurrentDefendArea)
		                && ((m_iTeam == TF2_TEAM_BLUE) || !CTeamFortress2Mod::isAttackDefendMap());
		bool bMoveTeleEntrance = (m_iTeam == TF2_TEAM_BLUE) || !CTeamFortress2Mod::isAttackDefendMap();
		bool bMoveTeleExit     = (m_iTeleExitArea != m_iCurrentAttackArea) && (m_iTeleExitArea != m_iCurrentDefendArea)
		                  && ((m_iTeam == TF2_TEAM_BLUE) || !CTeamFortress2Mod::isAttackDefendMap());
		bool bMoveDisp = bMoveSentry;

		// think about moving stuff now
		if (bMoveSentry && m_pSentryGun.get()
		    && ((m_fSentryPlaceTime + rcbot_move_sentry_time.GetFloat()) > engine->Time()))
			m_fSentryPlaceTime = engine->Time() - rcbot_move_sentry_time.GetFloat();
		if (bMoveDisp && m_pDispenser.get()
		    && ((m_fDispenserPlaceTime + rcbot_move_disp_time.GetFloat()) > engine->Time()))
			m_fDispenserPlaceTime = engine->Time() - rcbot_move_disp_time.GetFloat();
		if (bMoveTeleEntrance && m_pTeleEntrance.get()
		    && ((m_fTeleporterEntPlacedTime + rcbot_move_tele_time.GetFloat()) > engine->Time()))
			m_fTeleporterEntPlacedTime = engine->Time() - rcbot_move_tele_time.GetFloat();
		if (bMoveTeleExit && m_pTeleExit.get()
		    && ((m_fTeleporterExtPlacedTime + rcbot_move_tele_time.GetFloat()) > engine->Time()))
			m_fTeleporterExtPlacedTime = engine->Time() - rcbot_move_tele_time.GetFloat();

		// rethink everything
		updateCondition(CONDITION_CHANGED);
	}
}

void CBotTF2::updateAttackPoints()
{
	int iPrev            = m_iCurrentAttackArea;

	m_iTeam              = getTeam();

	m_iCurrentAttackArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_ATTACK);

	if (iPrev != m_iCurrentAttackArea)
		updateCondition(CONDITION_CHANGED);
}

void CBotTF2::updateDefendPoints()
{
	int iPrev            = m_iCurrentDefendArea;

	m_iTeam              = getTeam();

	m_iCurrentDefendArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_DEFEND);

	if (iPrev != m_iCurrentDefendArea)
		updateCondition(CONDITION_CHANGED);
}

/// TO DO : list of areas
// TODO: determine if the intent was to use WaypointList for these
void CBotTF2::getDefendArea(std::vector<int> *m_iAreas)
{
	m_iCurrentDefendArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_DEFEND);
}

void CBotTF2::getAttackArea(std::vector<int> *m_iAreas)
{
	m_iCurrentAttackArea = CTeamFortress2Mod::m_ObjectiveResource.getRandomValidPointForTeam(m_iTeam, TF2_POINT_ATTACK);
}

void CBotTF2::pointCaptured(int iPoint, int iTeam, const char *szPointName)
{
	m_pRedPayloadBomb  = nullptr;
	m_pBluePayloadBomb = nullptr;
}

// Is Enemy Function
// take a pEdict entity to check if its an enemy
// return TRUE to "OPEN FIRE" (Attack)
// return FALSE to ignore
#define RCBOT_ISENEMY_UNDEF -1
#define RCBOT_ISENEMY_TRUE 1
#define RCBOT_ISENEMY_FALSE 0

bool CBotTF2::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
	bool bIsPipeBomb = false, bIsRocket = false, bValid = false, bIsInvisible = false, bIsBoss = false,
	     bIsGrenade = false;

	if (!pEdict || !pEdict->GetUnknown())
		return false;

	if (!CBotGlobals::entityIsAlive(pEdict))
		return false;

	if (rcbot_notarget.GetBool() && (ENTINDEX(pEdict) == 1))
		return false;

	if (CBotGlobals::isPlayer(pEdict))
	{
		if (CBotGlobals::getTeam(pEdict) != getTeam())
		{
			if (m_iClass == TF_CLASS_SPY)
			{
				edict_t *pSentry = nullptr;

				if (!bCheckWeapons)
					return true;

				if (isDisguised())
				{
					if ((pSentry = m_pNearestEnemySentry.get()) != nullptr)
					{
						// If I'm disguised don't attack any player until nearby sentry is disabled
						if (!CTeamFortress2Mod::isSentrySapped(pSentry) && isVisible(pSentry)
						    && (distanceFrom(pSentry) < TF2_MAX_SENTRYGUN_RANGE))
							return false;
					}

					if ((pSentry = m_pLastEnemySentry.get()) != nullptr)
					{
						// If I'm disguised don't attack any player until nearby sentry is disabled
						if (!CTeamFortress2Mod::isSentrySapped(pSentry) && isVisible(pSentry)
						    && (distanceFrom(pSentry) < TF2_MAX_SENTRYGUN_RANGE))
							return false;
					}
				}
			}

			if (CClassInterface::getTF2Class(pEdict) == (int)TF_CLASS_SPY)
			{
				static float fMinReaction;
				static float fMaxReaction;
				static int dTeam, dClass, dHealth, dIndex;
				static bool bFoundSpy;
				static edict_t *pDisguisedAs;

				bFoundSpy = true; // shout found spy if true

				if (CClassInterface::getTF2SpyDisguised(pEdict, &dClass, &dTeam, &dIndex, &dHealth))
				{
					pDisguisedAs         = (dIndex > 0) ? (INDEXENT(dIndex)) : (nullptr);

					int entIndex         = ENTINDEX(pEdict) - 1;
					float fSpyAttackTime = engine->Time() - m_fSpyAttackedList[entIndex];

					if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pEdict)) // if he is cloaked -- can't see him
					{
						int iConds           = CClassInterface::getTF2Conditions(pEdict);

						bool bExposedCloaked = CClassInterface::getTF2SpyCloakMeter(pEdict) == 0.0f
						                    || CTeamFortress2Mod::TF2_IsPlayerOnFire(
						                           pEdict)        // if he is on fire and cloaked I can see him
						                    || iConds & (1 << 9)  /* Flicker */
						                    || iConds & (1 << 24) /* Jarated */
						                    || iConds & (1 << 25) /* Bleeding */
						                    || iConds & (1 << 27) /* Milked */
						                    || iConds & (1 << 123) /* Gassed */;

						const float fSpyAttackAfterCloakTime = .5f;
						float fSpyLastUncloakedTime          = engine->Time() - m_fSpyLastUncloakedList[entIndex];

						// Don't update last uncloaked time if there isn't anything exposing the spy in his cloaked
						// state
						if (!bExposedCloaked)
							bIsInvisible = true;

						// Destroy the cloaked spy if he either:
						// - Ran out of cloak, got damaged or touched an enemy (revealing him temporarily)
						// - Is on fire, coating in something or bleeding
						// - Just cloaked within fSpyAttackAfterCloakTime seconds & is not using dead ringer
						bValid = bExposedCloaked
						      || (fSpyLastUncloakedTime < fSpyAttackAfterCloakTime && !(iConds & (1 << 13)));
					}
					else if (dTeam == 0) // not disguised
					{
						bValid = true;
					}
					else if (dTeam != getTeam())
					{
						bValid    = true;
						bFoundSpy = false; // disguised as enemy!
					}
					else if (dIndex == ENTINDEX(m_pEdict)) // if he is disguised as me -- he must be a spy!
					{
						bValid = true;
					}
					else if (!isClassOnTeam(dClass, getTeam()))
					{ // be smart - check if player disguised as a class that exists on my team
						bValid = true;
					}
					else if (dHealth <= 0)
					{
						// be smart - check if player's health is below 0
						bValid = true;
					}
					// if he is on fire and I saw my team mate shoot him within the last 5 seconds, he's a spy!
					else if (CTeamFortress2Mod::TF2_IsPlayerOnFire(pEdict) && (fSpyAttackTime < 5.0f))
					{
						bValid = true;
					}
					// a. I can see the player he is disguised as
					// b. and the spy was shot recently by a teammate
					// = possibly by the player he was disguised as!
					else if (pDisguisedAs && isVisible(pDisguisedAs) && (fSpyAttackTime < 5.0f))
					{
						bValid = true;
					}
					else
						bValid = thinkSpyIsEnemy(pEdict, (TF_Class)dClass);

					if (bValid && !bIsInvisible)
						m_fSpyLastUncloakedList[entIndex] = engine->Time();

					if (bValid && bCheckWeapons && bFoundSpy)
						foundSpy(pEdict, (TF_Class)dClass);
				}

				// if ( CTeamFortress2Mod::TF2_IsPlayerDisguised(pEdict) ||
				// CTeamFortress2Mod::TF2_IsPlayerCloaked(pEdict) ) 	bValid = false;
			}
			else if ((m_iClass == TF_CLASS_ENGINEER) && (m_pSentryGun.get() != nullptr))
			{
				edict_t *pSentryEnemy = CClassInterface::getSentryEnemy(m_pSentryGun);

				if (pSentryEnemy == nullptr)
					bValid = true; // attack
				else if (pSentryEnemy == pEdict)
				{
					// let my sentry gun do the work
					bValid = false; // don't attack
				}
				else if ((CBotGlobals::entityOrigin(pSentryEnemy) - CBotGlobals::entityOrigin(pEdict)).Length() < 200)
					bValid = false; // sentry gun already shooting near this guy -- don't attack - let sentry gun do it
				else
					bValid = true;
			}
			else
				bValid = true;
		}
	}
	else if (CTeamFortress2Mod::isMapType(TF_MAP_RD) && !strcmp(pEdict->GetClassName(), "tf_robot_destruction_robot")
	         && (CClassInterface::getTeam(pEdict) != m_iTeam))
	{
		bValid = true;
	}
	else if (CTeamFortress2Mod::isBoss(pEdict))
	{
		bIsBoss = bValid = true;
	}
	// "FrenzyTime" is the time it takes for the bot to check out where he got hurt
	else if ((m_iClass != TF_CLASS_SPY) || (m_fFrenzyTime > engine->Time()))
	{
		int iEnemyTeam = CTeamFortress2Mod::getEnemyTeam(getTeam());

		// don't attack sentries if spy, just sap them
		if (((m_iClass != TF_CLASS_SPY) && CTeamFortress2Mod::isSentry(pEdict, iEnemyTeam))
		    || CTeamFortress2Mod::isDispenser(pEdict, iEnemyTeam) || CTeamFortress2Mod::isTeleporter(pEdict, iEnemyTeam)
		    /*CTeamFortress2Mod::isTeleporterExit(pEdict,iEnemyTeam)*/)
		{
			bValid = true;
		}
		else if (CTeamFortress2Mod::isPipeBomb(pEdict, iEnemyTeam))
			bIsPipeBomb = bValid = true;
		else if (CTeamFortress2Mod::isRocket(pEdict, iEnemyTeam))
			bIsRocket = bValid = true;
		else if (CTeamFortress2Mod::isHurtfulPipeGrenade(pEdict, m_pEdict, false))
			bIsGrenade = bValid = true;
	}

	if (bValid)
	{
		if (bCheckWeapons)
		{
			CBotWeapon *pWeapon = m_pWeapons->getBestWeapon(pEdict);

			if (pWeapon == nullptr)
				return false;
			else if (bIsPipeBomb && !pWeapon->canDestroyPipeBombs())
				return false;
			else if (bIsRocket && !pWeapon->canDeflectRockets())
				return false;
			else if (bIsRocket && m_iClass == TF_CLASS_PYRO)
			{
				edict_t *pFEnt = CClassInterface::getCurrentWeapon(m_pEdict);
				if (pFEnt && CClassInterface::TF2_getItemDefinitionIndex(pFEnt) == 594)
					return false;
			}
			else if (bIsGrenade && !pWeapon->canDeflectRockets())
				return false;
			else if (bIsGrenade && m_iClass == TF_CLASS_PYRO)
			{
				edict_t *pFEnt = CClassInterface::getCurrentWeapon(m_pEdict);
				if (pFEnt && CClassInterface::TF2_getItemDefinitionIndex(pFEnt) == 594)
					return false;
			}
			else if (bIsBoss && pWeapon->isMelee() && !pWeapon->isSpecial())
				return false;
		}

		return true;
	}

	return false;
}

////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// FORTRESS FOREVER

void CBotFF::modThink()
{
	// mod specific think code here
	CBotFortress::modThink();
}

bool CBotFF::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
	if (pEdict == m_pEdict)
		return false;

	if (!ENTINDEX(pEdict) || (ENTINDEX(pEdict) > CBotGlobals::maxClients()))
		return false;

	if (CBotGlobals::getTeam(pEdict) == getTeam())
		return false;

	return true;
}

void CBotTF2::MannVsMachineWaveComplete()
{
	if ((m_iClass != TF_CLASS_ENGINEER) || !isCarrying())
		m_pSchedules->freeMemory();

	m_fLastKnownTeamFlagTime = 0.0f;
	m_pPrevSpy               = nullptr;

	flagReset();
	teamFlagReset();

	m_pNavigator->clear();

	setLastEnemy(nullptr);

	reload();

	if (m_pSentryGun.get() != nullptr)
	{
		CWaypoint *pBest = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SENTRY);

		if (!pBest || (pBest->distanceFrom(CBotGlobals::entityOrigin(m_pSentryGun)) > 768))
		{
			// move
			m_fSentryPlaceTime     = 1.0f;
			m_fDispenserPlaceTime  = 1.0f;
			m_fLastSentryEnemyTime = 0.0f;
			m_iSentryKills         = 0;
			m_fDispenserHealAmount = 0;
		}
	}
}

void CBotTF2::MannVsMachineAlarmTriggered(Vector vLoc)
{
	if (m_iClass == TF_CLASS_ENGINEER)
	{
		edict_t *pSentry;

		if ((pSentry = m_pSentryGun.get()) != nullptr)
		{
			if (CTeamFortress2Mod::getSentryLevel(pSentry) < 3)
			{
				return;
			}
			else if ((m_fLastSentryEnemyTime + 5.0f) > engine->Time())
			{
				CWaypoint *pWaypoint = CTeamFortress2Mod::getBestWaypointMVM(this, CWaypointTypes::W_FL_SENTRY);

				Vector vSentry       = CBotGlobals::entityOrigin(pSentry);

				float fDist          = pWaypoint->distanceFrom(vSentry);

				if (fDist > 1024.0f)
				{
					// move sentry
					m_iSentryKills     = 0;
					m_fSentryPlaceTime = 1.0f;
				}
			}

			// don't defend work on sentry!!!
		}

		if (isCarrying())
			return;
	}

	float fDefTime         = randomFloat(10.0f, 20.0f);

	m_fDefendTime          = engine->Time() + fDefTime + 1.0f;

	CBotSchedule *newSched = new CBotDefendSched(vLoc, m_fDefendTime / 2);

	m_pSchedules->freeMemory();
	m_pSchedules->add(newSched);
	newSched->setID(SCHED_RETURN_TO_INTEL);
}

// Go back to Cap/Flag to
void CBotTF2::enemyAtIntel(Vector vPos, int type, int iArea)
{

	if (m_pSchedules->getCurrentSchedule())
	{
		if (m_pSchedules->getCurrentSchedule()->isID(SCHED_RETURN_TO_INTEL))
		{
			// already going back to intel
			return;
		}
	}

	if (CBotGlobals::entityIsValid(m_pDefendPayloadBomb)
	    && (CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)))
	{
		vPos = CBotGlobals::entityOrigin(m_pDefendPayloadBomb);
	}

	if ((m_iClass == TF_CLASS_DEMOMAN) && (m_iTrapType != TF_TRAP_TYPE_NONE) && (m_iTrapType != TF_TRAP_TYPE_WPT))
	{
		if ((m_iTrapType != TF_TRAP_TYPE_POINT) || (iArea == m_iTrapCPIndex))
		{
			// Stickies at PL Capture or bomb point
			if (((m_iTrapType == TF_TRAP_TYPE_POINT) || (m_iTrapType == TF_TRAP_TYPE_PL))
			    && (CTeamFortress2Mod::isMapType(TF_MAP_CART) || CTeamFortress2Mod::isMapType(TF_MAP_CARTRACE)))
			{
				edict_t *pBomb;

				// get enemy pl bomb
				if ((pBomb = m_pDefendPayloadBomb.get()) != nullptr)
				{
					if ((m_vStickyLocation - CBotGlobals::entityOrigin(pBomb)).Length() < (BLAST_RADIUS * 2))
						detonateStickies();
				}
			}
			else
				detonateStickies();
		}
	}

	m_fRevMiniGunTime = engine->Time() - m_fNextRevMiniGunTime;

	if (!m_pPlayerInfo)
		return;

	if (!isAlive())
		return;

	if (hasFlag())
		return;

	if (m_fDefendTime > engine->Time())
		return;

	if (m_iClass == TF_CLASS_ENGINEER)
		return; // got work to do...

	if ((distanceFrom(vPos) < 768.0f)
	    && (CTeamFortress2Mod::isMapType(TF_MAP_CP) || CTeamFortress2Mod::isMapType(TF_MAP_KOTH)))
	{
		m_vListenPosition      = vPos;
		m_bListenPositionValid = true;
		m_fListenTime          = engine->Time() + randomFloat(4.0f, 6.0f);
		m_fLookSetTime         = m_fListenTime;
		setLookAtTask(LOOK_NOISE);
	}

	// bot is already capturing a point
	if (m_pSchedules && m_pSchedules->isCurrentSchedule(SCHED_ATTACKPOINT))
	{
		// already attacking a point
		int capindex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iCurrentAttackArea];

		if (capindex >= 0)
		{
			// caxanga334: SDK 2013 doesn't like to create a Vector from an int
			// TODO: Proper fix
			const Vector vCapAttacking =
			    Vector(CTeamFortress2Mod::m_ObjectiveResource.getControlPointWaypoint(capindex));

			if (distanceFrom(vPos) > distanceFrom(vCapAttacking))
				return;
		}
		else if ((distanceFrom(vPos) > CWaypointLocations::REACHABLE_RANGE))
			return; // too far away, i should keep attacking
	}

	if ((vPos == Vector(0, 0, 0)) && (type == EVENT_CAPPOINT))
	{
		if (!m_iCurrentDefendArea)
			return;

		CWaypoint *pWpt = CWaypoints::randomWaypointGoal(
		    CWaypointTypes::W_FL_CAPPOINT, CTeamFortress2Mod::getEnemyTeam(getTeam()), m_iCurrentDefendArea, true);

		if (!pWpt)
			return;

		vPos = pWpt->getOrigin();
	}

	// everyone go back to cap point unless doing something important
	if ((type == EVENT_CAPPOINT)
	    || (!m_pNavigator->hasNextPoint()
	        || ((m_pNavigator->getGoalOrigin() - getOrigin()).Length() > ((vPos - getOrigin()).Length()))))
	{
		WaypointList *failed;
		m_pNavigator->getFailedGoals(&failed);
		CWaypoint *pWpt = nullptr;
		int iIgnore     = -1;

		if ((iArea >= 0) && (iArea < MAX_CONTROL_POINTS))
		{
			// get control point waypoint
			int iWpt = CTeamFortress2Mod::m_ObjectiveResource.getControlPointWaypoint(iArea);
			pWpt     = CWaypoints::getWaypoint(iWpt);

			if (pWpt && !pWpt->checkReachable())
			{
				iIgnore = iWpt;
				// go to nearest defend waypoint
				pWpt    = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(
                    vPos, 1024.0f, iIgnore, false, false, true, failed, false, m_iTeam, true, false, Vector(0, 0, 0),
                    CWaypointTypes::W_FL_DEFEND, m_pEdict));
			}
		}

		if (pWpt == nullptr)
			pWpt = CWaypoints::getWaypoint(CWaypointLocations::NearestWaypoint(vPos, 400, iIgnore, false, false, true,
			                                                                   failed, false, getTeam(), true));

		if (pWpt)
		{
			CBotSchedule *newSched = new CBotDefendSched(CWaypoints::getWaypointIndex(pWpt), randomFloat(1.0f, 6.0f));
			m_pSchedules->freeMemory();
			m_pSchedules->add(newSched);
			newSched->setID(SCHED_RETURN_TO_INTEL);
			m_fDefendTime = engine->Time() + randomFloat(10.0f, 20.0f);
		}
	}
}

void CBotTF2::buildingSapped(eEngiBuild building, edict_t *pSapper, edict_t *pSpy)
{
	static edict_t *pBuilding;

	m_pSchedules->freeMemory();

	if (isVisible(pSpy))
	{
		foundSpy(pSpy, CTeamFortress2Mod::getSpyDisguise(pSpy));
	}
	else
	{
		pBuilding = CTeamFortress2Mod::getBuilding(building, m_pEdict);

		if (pBuilding)
			m_vLastSeeSpy = CBotGlobals::entityOrigin(pBuilding);

		m_fLastSeeSpyTime = engine->Time();
		// m_pPrevSpy = pSpy;
	}
}

void CBotTF2::sapperDestroyed(edict_t *pSapper)
{
	m_pSchedules->freeMemory();
}

CBotTF2::CBotTF2()
{
	CBotFortress();
	m_iDesiredResistType       = 0;
	m_pVTable                  = nullptr;
	m_fDispenserPlaceTime      = 0.0f;
	m_fDispenserHealAmount     = 0.0f;
	m_fTeleporterEntPlacedTime = 0;
	m_fTeleporterExtPlacedTime = 0;
	m_iTeleportedPlayers       = 0;
	m_fDoubleJumpTime          = 0;
	m_fSpySapTime              = 0;
	m_iCurrentDefendArea       = 0;
	m_iCurrentAttackArea       = 0;
	// m_bBlockPushing = false;
	// m_fBlockPushTime = 0;
	m_pDefendPayloadBomb       = nullptr;
	m_pPushPayloadBomb         = nullptr;
	m_pRedPayloadBomb          = nullptr;
	m_pBluePayloadBomb         = nullptr;

	m_iTrapType                = TF_TRAP_TYPE_NONE;
	m_pLastEnemySentry         = MyEHandle(nullptr);
	m_prevSentryHealth         = 0;
	m_prevDispHealth           = 0;
	m_prevTeleExtHealth        = 0;
	m_prevTeleEntHealth        = 0;
	m_fHealClickTime           = 0;
	m_fCheckHealTime           = 0;

	m_fAttackPointTime         = 0; // used in cart maps

	m_prevSentryHealth         = 0;
	m_prevDispHealth           = 0;
	m_prevTeleExtHealth        = 0;
	m_prevTeleEntHealth        = 0;
	m_prevSentryShells         = 0;
	m_prevSentryRockets        = 0;
	m_fThanksTime              = 0;

	m_iSentryArea              = 0;
	m_iDispenserArea           = 0;
	m_iTeleEntranceArea        = 0;
	m_iTeleExitArea            = 0;

	for (unsigned int i = 0; i < 10; i++)
		m_fClassDisguiseFitness[i] = 1.0f;

	memset(m_fClassDisguiseTime, 0, sizeof(float) * 10);

	m_fSpyRedisguiseTime      = 0.0f;
	m_fSpyInfiltrateTime      = 0.0f;
	m_fSpyLurkStart           = 0.0f;
	m_bSpyLurking             = false;
	m_bSpyOpportunisticStrike  = false;
	m_iSpyContext             = 0;
	memset(m_iSpySeenClassCount, 0, sizeof(m_iSpySeenClassCount));
	m_pSpyLurkTarget          = MyEHandle(nullptr);
}

void CBotTF2::init(bool bVarInit)
{
	if (bVarInit)
		CBotTF2();

	CBotFortress::init(bVarInit);
}

bool CBotFortress::getIgnoreBox(Vector *vLoc, float *fSize)
{
	if ((m_iClass == TF_CLASS_ENGINEER) && vLoc)
	{
		edict_t *pSentry;

		if ((pSentry = m_pSentryGun.get()) != nullptr)
		{
			*vLoc  = CBotGlobals::entityOrigin(pSentry);
			*fSize = pSentry->GetCollideable()->OBBMaxs().Length() / 2;
			return true;
		}
	}
	else if ((m_iClass == TF_CLASS_SPY) && vLoc)
	{
		edict_t *pSentry;

		if ((pSentry = m_pNearestEnemySentry.get()) != nullptr)
		{
			*vLoc  = CBotGlobals::entityOrigin(pSentry);
			*fSize = pSentry->GetCollideable()->OBBMaxs().Length() / 2;
			return true;
		}
	}

	return false;
}

CBotWeapon *CBotTF2::getCurrentWeapon()
{
	edict_t *pWeapon = CClassInterface::TF2_getActiveWeapon(m_pEdict);

	if (pWeapon && !pWeapon->IsFree())
	{
		const char *pszClassname = pWeapon->GetClassName();

		if (pszClassname && *pszClassname)
			return m_pWeapons->getActiveWeapon(pszClassname, pWeapon, overrideAmmoTypes());
	}

	return nullptr;
}
