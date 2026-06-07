#include "mess_around_task.h"

#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_weapons.h"

CMessAround::CMessAround(edict_t *pFriendly, int iMaxVoiceCmd)
{
	m_fTime        = 0.0f;
	m_fSubTime     = 0.0f;
	m_fWanderTime  = 0.0f;
	m_pFriendly    = pFriendly;
	m_iType        = randomInt(0, 11);
	m_iMaxVoiceCmd = iMaxVoiceCmd;
	m_iVoiceCmd    = randomInt(0, iMaxVoiceCmd - 1);
	m_iSubState    = 0;
	m_bTagIsRunner = (randomInt(0, 1) == 0);
}

void CMessAround::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_pFriendly || !CBotGlobals::entityIsValid(m_pFriendly))
	{
		// Find a new random teammate
		float fBestDist = 768.0f;
		edict_t *pNew   = nullptr;
		for (int i = 1; i <= CBotGlobals::maxClients(); i++)
		{
			edict_t *pEd = INDEXENT(i);
			if (!pEd || !CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd))
				continue;
			if (CClassInterface::getTeam(pEd) != pBot->getTeam())
				continue;
			float fD = pBot->distanceFrom(pEd);
			if (fD < fBestDist && pBot->isVisible(pEd))
			{
				fBestDist   = fD;
				pNew        = pEd;
			}
		}
		if (pNew)
			m_pFriendly = pNew;
		else
		{
			fail();
			return;
		}
	}

	switch (m_iType)
	{
	case 0: // melee attack teammate
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (pBot->distanceFrom(m_pFriendly) > 100)
			pBot->setMoveTo(origin);
		else if (pBot->FInViewCone(m_pFriendly))
		{
			CBotWeapon *pWeapon = pBot->getBestWeapon(nullptr, true, true);
			if (pWeapon)
			{
				pBot->selectBotWeapon(pWeapon);
				if (randomInt(0, 1))
					pBot->primaryAttack();
			}
		}
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(3.5f, 8.0f);
	}
	break;

	case 1: // taunt at teammate
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		bool ok       = true;
		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (!pBot->FInViewCone(m_pFriendly))
			ok = false;
		if (pBot->distanceFrom(m_pFriendly) > 100)
		{
			pBot->setMoveTo(origin);
			ok = false;
		}
		if (ok)
		{
			if (pBot->isTF2())
				((CBotTF2 *)pBot)->taunt(true);
		}
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(3.5f, 6.5f);
	}
	break;

	case 2: // random voice commands, aimless wandering
	{
		if (!m_fTime)
			pBot->addVoiceCommand(randomInt(0, m_iMaxVoiceCmd - 1));
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(1.5f, 3.0f);

		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 3: // random buttons, aimless wandering
	{
		if (randomInt(0, 1))
			pBot->jump();
		else if (pBot->isTF2())
		{
			if (((CBotTF2 *)pBot)->getClass() == TF_CLASS_HWGUY)
				pBot->secondaryAttack(true);
		}
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(1.5f, 3.0f);

		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 4: // synchronized voice spam: spam same voice command every 0.5s, wander nearby
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (!m_fTime)
		{
			m_fTime    = engine->Time() + randomFloat(2.5f, 5.0f);
			m_fSubTime = 0.0f;
		}
		if (m_fSubTime < engine->Time())
		{
			pBot->addVoiceCommand(m_iVoiceCmd);
			m_fSubTime = engine->Time() + 0.4f;
		}
		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 5: // tag: chase teammates, switch if meleed
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (m_bTagIsRunner)
		{
			// Run away from the chaser
			Vector vAway = pBot->getOrigin() - origin;
			vAway.z      = 0;
			if (vAway.Length() < 50.0f)
			{
				vAway = Vector(randomFloat(-1, 1), randomFloat(-1, 1), 0);
			}
			if (vAway.Length() > 0.1f)
			{
				vAway = vAway / vAway.Length();
				pBot->setMoveTo(pBot->getOrigin() + (vAway * 250.0f));
			}
		}
		else
		{
			// Chase the runner
			if (pBot->distanceFrom(m_pFriendly) > 80)
				pBot->setMoveTo(origin);
			else if (pBot->FInViewCone(m_pFriendly))
			{
				CBotWeapon *pWeapon = pBot->getBestWeapon(nullptr, true, true);
				if (pWeapon)
				{
					pBot->selectBotWeapon(pWeapon);
					if (randomInt(0, 1))
						pBot->primaryAttack();
				}
			}
		}
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(4.0f, 10.0f);
	}
	break;

	case 6: // group stare: find nearby messing teammate, stare and spam voice command
	{
		// Look for another teammate within 200 units to form a group
		edict_t *pGroupMate = nullptr;
		float fBestDist     = 200.0f;
		for (int i = 1; i <= CBotGlobals::maxClients(); i++)
		{
			edict_t *pEd = INDEXENT(i);
			if (!pEd || pEd == pBot->getEdict() || pEd == m_pFriendly.get())
				continue;
			if (!CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd))
				continue;
			if (CClassInterface::getTeam(pEd) != pBot->getTeam())
				continue;
			float fD = pBot->distanceFrom(pEd);
			if (fD < fBestDist && pBot->isVisible(pEd))
			{
				fBestDist   = fD;
				pGroupMate  = pEd;
			}
		}

		if (pGroupMate)
		{
			m_pFriendly = pGroupMate;
			Vector origin = CBotGlobals::entityOrigin(pGroupMate);
			pBot->setLookVector(origin);
			pBot->setLookAtTask(LOOK_VECTOR);

			// Move within 80 units to form tight group
			if (pBot->distanceFrom(pGroupMate) > 80)
				pBot->setMoveTo(origin);

			// Spam voice command in tandem
			if (!m_fTime)
			{
				m_fTime    = engine->Time() + randomFloat(4.0f, 8.0f);
				m_fSubTime = 0.0f;
			}
			if (m_fSubTime < engine->Time())
			{
				pBot->addVoiceCommand(m_iVoiceCmd);
				m_fSubTime = engine->Time() + 0.35f;
			}
		}
		else
		{
			// No group mate nearby, just stand and spam voice commands
			if (!m_fTime)
				pBot->addVoiceCommand(randomInt(0, m_iMaxVoiceCmd - 1));
			if (!m_fTime)
				m_fTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 7: // shoot props/walls nearby while wandering
	{
		if (!m_fTime)
		{
			m_fTime      = engine->Time() + randomFloat(3.0f, 6.0f);
			m_fSubTime   = 0.0f;
			m_iSubState  = 0;
		}

		// Spray bullets at random nearby point
		if (m_fSubTime < engine->Time())
		{
			Vector vLook = pBot->getOrigin();
			vLook.x     += randomFloat(-200, 200);
			vLook.y     += randomFloat(-200, 200);
			vLook.z     += randomFloat(-80, 80);
			pBot->setLookVector(vLook);
			pBot->setLookAtTask(LOOK_VECTOR);
			m_fSubTime = engine->Time() + 0.3f;
		}

		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}

		CBotWeapon *pWeapon = pBot->getBestWeapon(nullptr, false, false);
		if (pWeapon && !pWeapon->isMelee() && !pWeapon->outOfAmmo(pBot))
		{
			pBot->selectBotWeapon(pWeapon);
			if (randomInt(0, 2))
				pBot->primaryAttack();
		}
	}
	break;

	case 8: // weapon switch spam: rapidly cycle weapons while wandering
	{
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(2.0f, 4.0f);

		if (m_fSubTime < engine->Time())
		{
			// Cycle through slots randomly
			int iSlot  = randomInt(0, 3);
			int iWeap  = 0;
			if (iSlot == 0) iWeap = TF2_WEAPON_SCATTERGUN;
			else if (iSlot == 1) iWeap = TF2_WEAPON_PISTOL;
			else iWeap = TF2_WEAPON_BAT;

			pBot->select_CWeapon(CWeapons::getWeapon(iWeap));
			m_fSubTime = engine->Time() + randomFloat(0.3f, 0.6f);
		}

		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 9: // crouch spam while wandering
	{
		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(2.0f, 4.0f);

		if (m_fSubTime < engine->Time())
		{
			if (m_iSubState == 0)
			{
				pBot->duck();
				m_iSubState = 1;
			}
			else
			{
				pBot->duck(false); // stand up
				m_iSubState = 0;
			}
			m_fSubTime = engine->Time() + randomFloat(0.2f, 0.4f);
		}

		// Look at friendly while crouching
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (m_fWanderTime < engine->Time())
		{
			float fAngle = randomFloat(0, M_PI * 2);
			float fDist  = randomFloat(100, 300);
			Vector vWander = pBot->getOrigin();
			vWander.x     += cos(fAngle) * fDist;
			vWander.y     += sin(fAngle) * fDist;
			pBot->setMoveTo(vWander);
			m_fWanderTime = engine->Time() + randomFloat(1.5f, 3.0f);
		}
	}
	break;

	case 10: // medic chain: two medics heal each other for silly beam chain
	{
		if (!pBot->isTF2() || ((CBotTF2 *)pBot)->getClass() != TF_CLASS_MEDIC)
		{
			m_iType = randomInt(0, 7); // fallback to another type
			break;
		}

		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(4.0f, 7.0f);

		// Find another medic nearby to form a heal chain
		if (!m_pFriendly.get() || !CBotGlobals::entityIsValid(m_pFriendly)
		    || CClassInterface::getTF2Class(m_pFriendly) != TF_CLASS_MEDIC)
		{
			float fBest = 384.0f;
			edict_t *pFound = nullptr;
			for (int i = 1; i <= CBotGlobals::maxClients(); i++)
			{
				edict_t *pEd = INDEXENT(i);
				if (!pEd || pEd == pBot->getEdict()) continue;
				if (!CBotGlobals::entityIsValid(pEd) || !CBotGlobals::entityIsAlive(pEd)) continue;
				if (CClassInterface::getTeam(pEd) != pBot->getTeam()) continue;
				if (CClassInterface::getTF2Class(pEd) != TF_CLASS_MEDIC) continue;
				float fD = pBot->distanceFrom(pEd);
				if (fD < fBest && pBot->isVisible(pEd))
				{
					fBest   = fD;
					pFound  = pEd;
				}
			}
			if (pFound)
				m_pFriendly = pFound;
		}

		edict_t *pMedicBuddy = m_pFriendly.get();
		if (pMedicBuddy)
		{
			Vector origin = CBotGlobals::entityOrigin(pMedicBuddy);
			pBot->setLookVector(origin);
			pBot->setLookAtTask(LOOK_VECTOR);
			if (pBot->distanceFrom(pMedicBuddy) > 100)
				pBot->setMoveTo(origin);
			// Heal the other medic to create a silly beam chain
			((CBotTF2 *)pBot)->healPlayer();
		}
	}
	break;

	default:
		break;
	}

	if (m_fTime < engine->Time())
		complete();

	if (pBot->isTF2())
	{
		if (CTeamFortress2Mod::hasRoundStarted())
			complete();
	}
	else if (pBot->isDOD())
	{
		if (CDODMod::m_Flags.getNumFlags() > 0)
			complete();
	}
}
