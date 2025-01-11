#pragma once

#include "botutil/base_task.h"

class CBotUseBuffItem : public CBotTask
{
  public:
	CBotUseBuffItem();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotUseBuffITem");
	}

  private:
	float m_fTime;
};