#include "wait_ammo_task.h"

#include "bot_mtrand.h"

CBotTF2WaitAmmoTask ::CBotTF2WaitAmmoTask(Vector vOrigin)
{
	m_vOrigin   = vOrigin;
	m_fWaitTime = 0.0f;
}

void CBotTF2WaitAmmoTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_fWaitTime)
		m_fWaitTime = engine->Time() + randomFloat(5.0f, 10.0f);

	if (!pBot->hasSomeConditions(CONDITION_NEED_AMMO))
	{
		complete();
	}
	else if (m_fWaitTime < engine->Time())
		fail();
	else if (pBot->distanceFrom(m_vOrigin) > 100)
	{
		pBot->setMoveTo(m_vOrigin);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotTF2WaitAmmoTask ::debugString(char *string)
{
	sprintf(string, "CBotTF2WaitAmmoTask");
}