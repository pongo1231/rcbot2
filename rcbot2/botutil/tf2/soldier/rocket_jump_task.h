#pragma once

#include "botutil/base_task.h"

class CBotTFRocketJump : public CBotTask
{
  public:
	CBotTFRocketJump(); // going here

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	float m_fTime;
	float m_fJumpTime;
	int m_iState;
};