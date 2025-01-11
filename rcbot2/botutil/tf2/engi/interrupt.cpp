#include "interrupt.h"

#include "bot_getprop.h"
#include "bot_mods.h"

CBotTF2EngineerInterrupt ::CBotTF2EngineerInterrupt(CBot *pBot)
{
	m_pSentryGun = CTeamFortress2Mod::getMySentryGun(pBot->getEdict());

	if (m_pSentryGun.get() != nullptr)
	{
		m_fPrevSentryHealth = CClassInterface::getSentryHealth(m_pSentryGun);
	}
	else
		m_fPrevSentryHealth = 0;
}

bool CBotTF2EngineerInterrupt ::isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted)
{
	if (m_pSentryGun.get() != nullptr)
	{
		if (!CClassInterface::getTF2BuildingIsMini(m_pSentryGun))
		{
			float m_fCurrentHealth = CClassInterface::getSentryHealth(m_pSentryGun);

			if ((((CBotFortress *)pBot)->getMetal() > 75) && (m_fCurrentHealth < m_fPrevSentryHealth))
			{
				*bFailed = true;
				return true;
			}

			m_fPrevSentryHealth = m_fCurrentHealth;
		}
	}

	return false;
}