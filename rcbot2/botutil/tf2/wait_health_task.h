#pragma once

#include "botutil/base_task.h"

class CBotTF2WaitHealthTask : public CBotTask
{
  public:
	CBotTF2WaitHealthTask(Vector vOrigin);

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	float m_fWaitTime;
};