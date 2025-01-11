#include "wait_health_task.h"

#include "bot_fortress.h"
#include "bot_mtrand.h"

CBotTF2WaitHealthTask ::CBotTF2WaitHealthTask(Vector vOrigin)
{
	m_vOrigin   = vOrigin;
	m_fWaitTime = 0;
}

void CBotTF2WaitHealthTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_fWaitTime)
		m_fWaitTime = engine->Time() + randomFloat(5.0f, 10.0f);

	if (!pBot->hasSomeConditions(CONDITION_NEED_HEALTH))
		complete();
	else if (m_fWaitTime < engine->Time())
		fail();
	else
	{
		// TO DO
		/*edict_t *pOtherPlayer = CBotGlobals::findNearestPlayer(m_vOrigin,50.0,pBot->getEdict());

		if ( pOtherPlayer )
		{
		    fail();
		    return;
		}*/

		pBot->setLookAtTask(LOOK_AROUND);

		if (pBot->distanceFrom(m_vOrigin) > 50)
			pBot->setMoveTo((m_vOrigin));
		else
			pBot->stopMoving();

		if (pBot->isTF())
		{
			((CBotTF2 *)pBot)->taunt();

			if (((CBotTF2 *)pBot)->isBeingHealed())
				complete();
		}
	}
}

void CBotTF2WaitHealthTask ::debugString(char *string)
{
	sprintf(string, "CBotTF2WaitHealthTask\nm_vOrigin = (%0.4f,%0.4f,%0.4f)", m_vOrigin.x, m_vOrigin.y, m_vOrigin.z);
}