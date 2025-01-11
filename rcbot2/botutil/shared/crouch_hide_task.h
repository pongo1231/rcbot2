#pragma once

#include "botutil/base_task.h"

class CCrouchHideTask : public CBotTask
{
  public:
	CCrouchHideTask(edict_t *pHideFrom);

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pHideFrom;
	float m_fHideTime;
	float m_fChangeTime;
	bool m_bCrouching;
	Vector m_vLastSeeVector;
};