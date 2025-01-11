#include "move_to_task.h"

#include "bot_globals.h"

void CMoveToTask ::init()
{
	fPrevDist = 0;
	// m_vVector = Vector(0,0,0);
	// m_pEdict = nullptr;
}

void CMoveToTask ::debugString(char *string)
{
	sprintf(string, "CMoveToTask\nm_vVector =(%0.4f,%0.4f,%0.4f)", m_vVector.x, m_vVector.y, m_vVector.z);
}

CMoveToTask ::CMoveToTask(edict_t *pEdict)
{
	m_pEdict  = pEdict;
	m_vVector = CBotGlobals::entityOrigin(m_pEdict);

	// setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

void CMoveToTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{

	float fDistance;

	fDistance = pBot->distanceFrom(m_vVector);

	// sort out looping move to origins by using previous distance check
	if ((fDistance < 64) || (fPrevDist && (fPrevDist < fDistance)))
	{
		complete();
		return;
	}
	else
	{
		pBot->setMoveTo(m_vVector);

		if (pBot->moveFailed())
			fail();
	}

	fPrevDist = fDistance;
}