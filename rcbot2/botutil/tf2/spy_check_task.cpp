#include "spy_check_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"
#include "bot_weapons.h"

#include <in_buttons.h>

void CSpyCheckAir ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotTF2 *pBotTF2 = (CBotTF2 *)pBot;
	CBotWeapon *pWeapon;
	CBotWeapon *pChooseWeapon;
	CBotWeapons *pWeaponList;
	static int iAttackProb;

	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
		complete(); // don't waste uber spy checking

	if (m_fTime == 0.0f)
	{
		// record the number of people I see now
		int i;
		edict_t *pPlayer;
		IPlayerInfo *p;
		/*		edict_t *pDisguised;

		        int iClass;
		        int iTeam;
		        int iIndex;
		        int iHealth;
		*/
		seenlist     = 0;
		m_bHitPlayer = false;

		for (i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = INDEXENT(i);

			if (pPlayer == pBot->getEdict())
				continue;

			if (!CBotGlobals::entityIsValid(pPlayer))
				continue;

			p = playerinfomanager->GetPlayerInfo(pPlayer);

			if (p->IsDead() || p->IsObserver())
				continue;

			if (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY)
			{

				if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer))
					continue; // can't see
			}

			if (pBot->isVisible(pPlayer))
				seenlist |= (1 << (i - 1));
		}

		m_fTime            = engine->Time() + randomFloat(2.0f, 5.0f);
		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}

	if (m_fTime < engine->Time())
		complete();

	if (pBot->hasEnemy())
		complete();

	if ((m_pUnseenBefore == nullptr) && (m_fNextCheckUnseen < engine->Time()))
	{
		int i;
		edict_t *pPlayer;
		IPlayerInfo *p;

		seenlist = 0;

		for (i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = INDEXENT(i);

			if (pPlayer == pBot->getEdict())
				continue;

			if (!CBotGlobals::entityIsValid(pPlayer))
				continue;

			p = playerinfomanager->GetPlayerInfo(pPlayer);

			if (p->IsDead() || p->IsObserver())
				continue;

			if (CClassInterface::getTF2Class(pPlayer) == TF_CLASS_SPY)
			{
				// CClassInterface::getTF2SpyDisguised(pPlayer,&iClass,&iTeam,&iIndex,&iHealth);

				// if ( (iIndex > 0) && (iIndex <= gpGlobals->maxClients) )

				if (CTeamFortress2Mod::TF2_IsPlayerCloaked(pPlayer))
					continue; // can't see but may still be in visible list
			}

			if (pBot->isVisible(pPlayer))
			{
				if (CTeamFortress2Mod::isFlagCarrier(pPlayer))
					continue; // spies can't hold flag unless not disguised

				if (!(seenlist & (1 << (i - 1))))
				{
					m_pUnseenBefore = pPlayer;
					seenlist |= (1 << (i - 1)); // add to list
					break;
				}
			}
		}

		if (m_pUnseenBefore != nullptr)
		{
			// add more time
			m_fTime = engine->Time() + randomFloat(2.0f, 5.0f);
		}

		m_fNextCheckUnseen = engine->Time() + 0.1f;
	}
	else if (m_pUnseenBefore
	         && (!CBotGlobals::entityIsAlive(m_pUnseenBefore) || !CBotGlobals::entityIsValid(m_pUnseenBefore)
	             || !pBot->isVisible(m_pUnseenBefore)))
	{
		m_pUnseenBefore    = nullptr;
		m_fNextCheckUnseen = 0.0f;
	}

	if (m_pUnseenBefore == nullptr)
	{
		// automatically look at danger points
		pBot->updateDanger(50.0f);

		pBot->setLookAtTask(LOOK_WAYPOINT);
	}
	else
	{
		// smack him
		pBot->lookAtEdict(m_pUnseenBefore);
		pBot->setLookAtTask(LOOK_EDICT);
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}
	/*
	    TF_CLASS_CIVILIAN = 0,
	    TF_CLASS_SCOUT,
	    TF_CLASS_SNIPER,
	    TF_CLASS_SOLDIER,
	    TF_CLASS_DEMOMAN,
	    TF_CLASS_MEDIC,
	    TF_CLASS_HWGUY,
	    TF_CLASS_PYRO,
	    TF_CLASS_SPY,
	    TF_CLASS_ENGINEER,
	    TF_CLASS_MAX*/

	pWeapon = pBot->getCurrentWeapon();

	switch (pBotTF2->getClass())
	{
	case TF_CLASS_PYRO:

		pWeaponList   = pBot->getWeapons();

		pChooseWeapon = pWeaponList->getWeapon(CWeapons::getWeapon(TF2_WEAPON_FLAMETHROWER));

		if (pChooseWeapon && pChooseWeapon->hasWeapon() && !pChooseWeapon->outOfAmmo(pBot))
		{
			// use flamethrower
			iAttackProb = 90;
			break;
		}
		// move down to melee if out of ammo
	default:
		iAttackProb   = 75;
		pChooseWeapon = pBot->getBestWeapon(nullptr, true, true, true);
		break;
	}

	if (m_pUnseenBefore)
	{
		pBot->setMoveTo(CBotGlobals::entityOrigin(m_pUnseenBefore));
	}

	if (pChooseWeapon && (pWeapon != pChooseWeapon))
	{
		if (!pBot->selectBotWeapon(pChooseWeapon))
		{
			fail();
		}
	}
	else
	{
		if (randomInt(0, 100) < iAttackProb)
		{
			pBot->primaryAttack();

			if (m_pUnseenBefore != nullptr)
			{
				if (!m_bHitPlayer && (pBot->distanceFrom(CBotGlobals::entityOrigin(m_pUnseenBefore)) < 80.0f))
				{
					m_bHitPlayer = true;
					m_fTime      = engine->Time() + randomFloat(0.5f, 1.5f);
				}
			}
		}

		if (m_pUnseenBefore && pBot->isVisible(m_pUnseenBefore))
		{
			IPlayerInfo *p = playerinfomanager->GetPlayerInfo(m_pUnseenBefore);

			// stop if I see the player im hitting attacking
			if (p->GetLastUserCommand().buttons & IN_ATTACK)
			{
				complete();
				return;
			}
		}
	}
}

void CSpyCheckAir ::debugString(char *string)
{
	sprintf(string, "CSpyCheckAir: checking for spies");
}