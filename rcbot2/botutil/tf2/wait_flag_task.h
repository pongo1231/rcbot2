#pragma once

#include "botutil/base_task.h"

class CBotTF2WaitFlagTask : public CBotTask
{
  public:
	CBotTF2WaitFlagTask(Vector vOrigin, bool bFind = false);

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	float m_fWaitTime;
	bool m_bFind;
};