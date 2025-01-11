#pragma once

#include "botutil/base_task.h"

class CDODWaitForBombTask : public CBotTask
{
  public:
	CDODWaitForBombTask(edict_t *pBombTarget, CWaypoint *pBlocking)
	{
		m_pBombTarget = pBombTarget;
		m_fTime       = 0.0f;
		m_pBlocking   = pBlocking;
	}
	virtual void debugString(char *string);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

  private:
	MyEHandle m_pBombTarget;
	float m_fTime;
	CWaypoint *m_pRunTo;
	CWaypoint *m_pBlocking;
};