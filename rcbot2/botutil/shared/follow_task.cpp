#include "follow_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"

CFollowTask ::CFollowTask(edict_t *pFollow)
{
	m_pFollow        = pFollow;
	m_fFollowTime    = 0;
	m_vLastSeeVector = CBotGlobals::entityOrigin(pFollow);
	CClassInterface::getVelocity(pFollow, &m_vLastSeeVelocity);
}

void CFollowTask ::init()
{
}

void CFollowTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_pFollow.get() == nullptr)
	{
		fail();
		return;
	}

	if (!CBotGlobals::entityIsAlive(m_pFollow.get()))
	{
		fail();
		return;
	}

	pBot->setLookVector(m_vLastSeeVector);
	pBot->setLookAtTask(LOOK_VECTOR);

	if (pBot->isVisible(m_pFollow))
	{
		m_vLastSeeVector = CBotGlobals::entityOrigin(m_pFollow);

		if (pBot->distanceFrom(m_pFollow) > 150.0f)
			pBot->setMoveTo(m_vLastSeeVector);
		else
			pBot->stopMoving();

		CClassInterface::getVelocity(m_pFollow, &m_vLastSeeVelocity);

		m_vLastSeeVector = m_vLastSeeVector + m_vLastSeeVelocity;

		m_fFollowTime    = engine->Time() + 1.0f;
	}

	if (m_fFollowTime < engine->Time())
		complete();
}

void CFollowTask::debugString(char *string)
{
	sprintf(string, "CFollowTask\nm_pFollow =(%s)", engine->GetPlayerNetworkIDString(m_pFollow));
}