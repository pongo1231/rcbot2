#include "base_task.h"

#include "bot_profiling.h"
#include "bot_weapons.h"
#include "botutil/base_sched.h"

#include <in_buttons.h>

CBotTask ::CBotTask()
{
	_init();
}

bool CBotTask ::timedOut()
{
	return (this->m_fTimeOut != 0) && (engine->Time() < this->m_fTimeOut);
}

eTaskState CBotTask ::isInterrupted(CBot *pBot)
{
	if (m_pInterruptFunc != nullptr)
	{
		bool bFailed    = false;
		bool bCompleted = false;

		if (m_pInterruptFunc->isInterrupted(pBot, &bFailed, &bCompleted))
		{
			if (bFailed)
				return STATE_FAIL;

			return STATE_COMPLETE;
		}
	}

	if (m_iCompleteInterruptConditionsHave)
	{
		if (pBot->hasAllConditions(m_iCompleteInterruptConditionsHave))
			return STATE_COMPLETE;
	}

	if (m_iCompleteInterruptConditionsDontHave)
	{
		if (!pBot->hasAllConditions(m_iCompleteInterruptConditionsDontHave))
			return STATE_COMPLETE;
	}

	if (m_iFailInterruptConditionsHave)
	{
		if (pBot->hasAllConditions(m_iFailInterruptConditionsHave))
			return STATE_FAIL;
	}

	if (m_iFailInterruptConditionsDontHave)
	{
		if (!pBot->hasAllConditions(m_iFailInterruptConditionsDontHave))
			return STATE_FAIL;
	}

	return STATE_RUNNING;
}

void CBotTask ::_init()
{
	m_pInterruptFunc                       = nullptr;
	m_iFlags                               = 0;
	m_iState                               = STATE_IDLE;
	m_fTimeOut                             = 0;
	//	m_pEdict = nullptr;
	//	m_fFloat = 0;
	//	m_iInt = 0;
	//	m_vVector = Vector(0,0,0);
	m_iFailInterruptConditionsHave         = 0;
	m_iFailInterruptConditionsDontHave     = 0;
	m_iCompleteInterruptConditionsHave     = 0;
	m_iCompleteInterruptConditionsDontHave = 0;
	init();
}

void CBotTask ::init()
{
	return;
}

void CBotTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	return;
}

bool CBotTask ::hasFailed()
{
	return m_iState == STATE_FAIL;
}

bool CBotTask ::isComplete()
{
	return m_iState == STATE_COMPLETE;
}
/*
void CBotTask :: setVector ( Vector vOrigin )
{
    m_vVector = vOrigin;
}

void CBotTask :: setFloat ( float fFloat )
{
    m_fFloat = fFloat;
}

void CBotTask :: setEdict ( edict_t *pEdict )
{
    m_pEdict = pEdict;
}
*/
// if this condition is true it will complete, if bUnset is true, the condition must be false to be complete
void CBotTask ::setCompleteInterrupt(int iInterruptHave, int iInterruptDontHave)
{
	m_iCompleteInterruptConditionsHave     = iInterruptHave;
	m_iCompleteInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask ::setFailInterrupt(int iInterruptHave, int iInterruptDontHave)
{
	m_iFailInterruptConditionsHave     = iInterruptHave;
	m_iFailInterruptConditionsDontHave = iInterruptDontHave;
}

void CBotTask ::fail()
{
	m_iState = STATE_FAIL;
}

void CBotTask ::complete()
{
	m_iState = STATE_COMPLETE;
}