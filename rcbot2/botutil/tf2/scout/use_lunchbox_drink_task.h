#pragma once

#include "botutil/base_task.h"

class CBotUseLunchBoxDrink : public CBotTask
{
  public:
	CBotUseLunchBoxDrink();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotUseLunchBoxDrink");
	}

  private:
	float m_fTime;
};