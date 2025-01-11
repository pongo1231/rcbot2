#include "mess_around_task.h"

#include "bot_globals.h"

CMessAround::CMessAround(edict_t *pFriendly, int iMaxVoiceCmd)
{
	m_fTime        = 0.0f;
	m_pFriendly    = pFriendly;
	m_iType        = randomInt(0, 3);
	m_iMaxVoiceCmd = iMaxVoiceCmd;
}

void CMessAround::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_pFriendly || !CBotGlobals::entityIsValid(m_pFriendly))
	{
		fail();
		return;
	}

	// smack the friendly player with my melee attack
	switch (m_iType)
	{
	case 0:
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);

		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (pBot->distanceFrom(m_pFriendly) > 100)
		{
			pBot->setMoveTo((origin));
		}
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
	break; // taunt at my friendly player
	case 1:
	{
		Vector origin = CBotGlobals::entityOrigin(m_pFriendly);
		bool ok       = true;

		pBot->setLookVector(origin);
		pBot->setLookAtTask(LOOK_VECTOR);

		if (!pBot->FInViewCone(m_pFriendly))
		{
			ok = false;
		}

		if (pBot->distanceFrom(m_pFriendly) > 100)
		{
			pBot->setMoveTo((origin));
			ok = false;
		}

		if (ok)
		{
			if (pBot->isTF2())
				((CBotTF2 *)pBot)->taunt(true);
			// else if ( pBot->isDOD() )
			//	((CDODBot*)pBot)->taunt(); pBot->impulse(100);
		}

		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(3.5f, 6.5f);
	}
	// say some random voice commands
	break;
	case 2:
	{
		if (!m_fTime)
			pBot->addVoiceCommand(randomInt(0, m_iMaxVoiceCmd - 1));

		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(1.5f, 3.0f);
	}
	// press some random buttons, such as attack2, jump
	break;
	case 3:
	{
		if (randomInt(0, 1))
			pBot->jump();
		else
		{
			if (pBot->isTF2())
			{
				if (((CBotTF2 *)pBot)->getClass() == TF_CLASS_HWGUY)
					pBot->secondaryAttack(true);
			}
		}

		if (!m_fTime)
			m_fTime = engine->Time() + randomFloat(1.5f, 3.0f);
	}
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