#pragma once

#include "botutil/base_task.h"

class CMoveToTask : public CBotTask
{
  public:
	CMoveToTask(Vector vOrigin)
	{
		m_vVector = vOrigin;
		m_pEdict  = nullptr;

		setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	}

	CMoveToTask(edict_t *pEdict);

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	float fPrevDist;
	Vector m_vVector;
	MyEHandle m_pEdict;
};