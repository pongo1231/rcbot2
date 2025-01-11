#pragma once

#include "botutil/base_task.h"

class CHideTask : public CBotTask
{
  public:
	CHideTask(Vector vHideFrom);

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	Vector m_vHideFrom;
	float m_fHideTime;
};