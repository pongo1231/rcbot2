#pragma once

#include "botutil/base_task.h"

class CAutoBuy : public CBotTask
{
  public:
	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

  private:
	float m_fTime;
	bool m_bTimeset;
};