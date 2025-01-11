#include "taunt_task.h"

#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"

void CTF2_TauntTask ::init()
{
	m_fTime       = 0;
	m_fTauntUntil = 0.f;
	m_fActionTime = 0.f;
}

void CTF2_TauntTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_pPlayer)
	{
		// Stop taunting
		/*CClassInterface::g_GetProps[GETPROP_TF2_CONDITIONS].getData(pBot->getEdict());
		 *static_cast<int *>(CClassInterface::g_GetProps[GETPROP_TF2_CONDITIONS].m_data) &= ~(1 << 7);*/

		fail();
		return;
	}

	float fTime = engine->Time();

	if (m_fTauntUntil == 0.f)
	{
		if (m_fTime == 0)
			m_fTime = engine->Time() + randomFloat(2.5f, 5.f);
		else if (m_fTime < engine->Time())
		{
			fail();
			return;
		}

		pBot->lookAtEdict(m_pPlayer.get());
		pBot->setLookAtTask(LOOK_EDICT, 10.f);

		if (pBot->distanceFrom(m_vOrigin) > m_fDist)
			pBot->setMoveTo(m_vOrigin);
		else
		{
			CBotTF2 *pTF2Bot = static_cast<CBotTF2 *>(pBot);

			if (pTF2Bot->getClass() == TF_CLASS_SPY)
			{
				if (pTF2Bot->isCloaked())
				{
					pTF2Bot->spyUnCloak();
					return;
				}

				if (pTF2Bot->isDisguised())
				{
					pTF2Bot->spyDisguise(pTF2Bot->getTeam(), 8);
					return;
				}
			}

			pTF2Bot->taunt(true);

			m_fTauntUntil = fTime + randomFloat(15.f, 30.f);
		}

		return;
	}

	bool bIsTaunting     = CClassInterface::getTF2Conditions(pBot->getEdict()) & (1 << 7);
	Vector vPlayerOrigin = m_pPlayer.get()->GetCollideable()->GetCollisionOrigin();

	// Don't do anything but follow target player as long as they are still taunting and bot isn't wandering off
	if (bIsTaunting && (CClassInterface::getTF2Conditions(m_pPlayer.get()) & (1 << 7)) && vPlayerOrigin.IsValid()
	    && pBot->distanceFrom(vPlayerOrigin) < m_fDist * 4.f)
	{
		// In case taunt time has expired, extend it a bit as long as target player is still taunting
		if (fTime > m_fTauntUntil)
			m_fTauntUntil = fTime + randomFloat(2.f, 10.f);

		// Do random actions in case the taunt permits it
		/*if (fTime > m_fActionTime)
		{
		    m_fActionTime = fTime + randomFloat(.5f, 2.f);

		    int iRand     = randomInt(0, 20);
		    if (iRand == 0)
		        pBot->primaryAttack(true);
		    else if (iRand == 1)
		        pBot->secondaryAttack(true);
		}*/

		// Chase player if the taunt allows movement
		if (pBot->distanceFrom(vPlayerOrigin) > m_fDist * 2)
		{
			vPlayerOrigin.x += randomFloat(-m_fDist * 2.f, m_fDist * 2.f);
			vPlayerOrigin.y += randomFloat(-m_fDist * 2.f, m_fDist * 2.f);
			pBot->setMoveTo(vPlayerOrigin);
		}

		return;
	}

	if (bIsTaunting)
	{
		if (fTime > m_fTauntUntil)
		{
			// Stop taunting
			/*CClassInterface::g_GetProps[GETPROP_TF2_CONDITIONS].getData(pBot->getEdict());
			 *static_cast<int *>(CClassInterface::g_GetProps[GETPROP_TF2_CONDITIONS].m_data) &= ~(1 << 7);*/
		}

		// return;
	}

	complete();
}

void CTF2_TauntTask ::debugString(char *string)
{
	sprintf(string, "CTF2_TauntTask");
}