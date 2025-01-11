#pragma once

#include "botutil/base_task.h"

class CFindLastEnemy : public CBotTask
{
  public:
	CFindLastEnemy(Vector vLast, Vector vVelocity);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CFindLastEnemy");
	}

  private:
	Vector m_vLast;
	float m_fTime;
};