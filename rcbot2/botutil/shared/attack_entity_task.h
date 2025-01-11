#pragma once

#include "botutil/base_task.h"

class CAttackEntityTask : public CBotTask
{
  public:
	CAttackEntityTask(edict_t *pEdict);
	void init();
	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	MyEHandle m_pEdict;
};