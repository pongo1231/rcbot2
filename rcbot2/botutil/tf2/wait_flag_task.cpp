#include "wait_flag_task.h"

#include "bot_fortress.h"

CBotTF2WaitFlagTask ::CBotTF2WaitFlagTask(Vector vOrigin, bool bFind)
{
	m_vOrigin   = vOrigin;
	m_fWaitTime = 0;
	m_bFind     = bFind;
}

void CBotTF2WaitFlagTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_fWaitTime)
	{
		if (m_bFind)
			m_fWaitTime = engine->Time() + 5.0f;
		else
			m_fWaitTime = engine->Time() + 10.0f;
	}

	if (((CBotTF2 *)pBot)->hasFlag())
		complete();
	else if (pBot->getHealthPercent() < 0.2)
	{
		fail();
	}
	else if (m_fWaitTime < engine->Time())
	{
		((CBotFortress *)pBot)->flagReset();
		fail();
	}
	else if (!pBot->isTF())
	{
		fail();
	}
	else
	{
		if (!((CBotFortress *)pBot)->waitForFlag(&m_vOrigin, &m_fWaitTime, m_bFind))
		{
			fail();
		}
	}
}

void CBotTF2WaitFlagTask ::debugString(char *string)
{
	sprintf(string, "CBotTF2WaitFlagTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)", m_vOrigin.x, m_vOrigin.y, m_vOrigin.z);
}