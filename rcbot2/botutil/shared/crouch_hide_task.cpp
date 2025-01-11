#include "crouch_hide_task.h"

#include "bot_globals.h"

CCrouchHideTask ::CCrouchHideTask(edict_t *pHideFrom)
{
	m_pHideFrom      = pHideFrom;
	m_vLastSeeVector = CBotGlobals::entityOrigin(pHideFrom);
	m_bCrouching     = true; // duck
	m_fChangeTime    = 0.0f;
	m_fHideTime      = 0.0f;
}

void CCrouchHideTask ::init()
{
	m_bCrouching  = true; // duck
	m_fChangeTime = 0.0f;
	m_fHideTime   = 0.0f;
}

void CCrouchHideTask ::debugString(char *string)
{
	sprintf(string, "CCrouchHideTask\nm_pHideFrom =(%s)", engine->GetPlayerNetworkIDString(m_pHideFrom));
}

void CCrouchHideTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	if (m_pHideFrom.get() == nullptr)
	{
		complete();
		return;
	}
	if (!CBotGlobals::entityIsAlive(m_pHideFrom))
	{
		complete();
		return;
	}

	if (m_fHideTime == 0)
		m_fHideTime = engine->Time() + randomFloat(7.0f, 14.0f);

	if (m_fChangeTime == 0.0f)
		m_fChangeTime = engine->Time() + randomFloat(0.9f, 2.9f);

	pBot->stopMoving();

	if (pBot->isVisible(m_pHideFrom))
		m_vLastSeeVector = CBotGlobals::entityOrigin(m_pHideFrom);

	pBot->setLookVector(m_vLastSeeVector);

	pBot->setLookAtTask(LOOK_VECTOR);

	if (m_fChangeTime < engine->Time())
	{
		m_bCrouching  = !m_bCrouching;
		m_fChangeTime = engine->Time() + randomFloat(1.0f, 3.0f);
	}

	if (m_bCrouching)
	{
		pBot->duck(true);
		pBot->wantToShoot(false);
	}
	else if (pBot->hasEnemy())
	{
		m_fHideTime = engine->Time() + 10.0f;
	}

	// refrain from proning
	pBot->updateCondition(CONDITION_RUN);
	pBot->removeCondition(CONDITION_PRONE);

	if (m_fHideTime < engine->Time())
		complete();
}