#pragma once

#include "botutil/base_task.h"

class CBotTF2WaitAmmoTask : public CBotTask
{
  public:
	CBotTF2WaitAmmoTask(Vector vOrigin);

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	float m_fWaitTime;
};