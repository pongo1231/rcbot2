#pragma once

#include "botutil/base_task.h"

class CBotNest : public CBotTask
{
  public:
	CBotNest();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotNest");
	}

  private:
	float m_fTime;
	MyEHandle m_pEnemy;
};
