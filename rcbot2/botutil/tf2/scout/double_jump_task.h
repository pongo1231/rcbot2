#pragma once

#include "botutil/base_task.h"

class CBotTFDoubleJump : public CBotTask
{
  public:
	CBotTFDoubleJump(); // going here

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	float m_fTime;
};