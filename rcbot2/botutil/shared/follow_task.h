#pragma once

#include "botutil/base_task.h"

class CFollowTask : public CBotTask
{
  public:
	CFollowTask(edict_t *pFollow);

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pFollow;
	float m_fFollowTime;
	Vector m_vLastSeeVector;
	Vector m_vLastSeeVelocity;
};