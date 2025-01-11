#include "hide_task.h"
#include "bot_mtrand.h"

CHideTask ::CHideTask(Vector vHideFrom)
{
	m_vHideFrom = vHideFrom;
}

void CHideTask ::debugString(char *string)
{
	sprintf(string, "CHideTask\nm_vHideFrom =(%0.4f,%0.4f,%0.4f)", m_vHideFrom.x, m_vHideFrom.y, m_vHideFrom.z);
}

void CHideTask ::init()
{
	m_fHideTime = 0;
}

void CHideTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->stopMoving();
	pBot->setLookVector(m_vHideFrom);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->duck(true);

	if (m_fHideTime == 0)
		m_fHideTime = engine->Time() + randomFloat(5.0, 10.0);

	if (m_fHideTime < engine->Time())
		complete();
}