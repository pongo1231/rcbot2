#include "hurt_interrupt.h"

CBotTF2HurtInterrupt ::CBotTF2HurtInterrupt(CBot *pBot)
{
	m_iHealth = pBot->getHealthPercent();
}

bool CBotTF2HurtInterrupt::isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted)
{
	if (m_iHealth > pBot->getHealthPercent())
	{
		*bFailed = true;
		pBot->secondaryAttack();
		return true;
	}

	return false;
}