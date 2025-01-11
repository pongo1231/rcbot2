#pragma once

#include "botutil/base_task.h"

class CDODWaitForGrenadeTask : public CBotTask
{
  public:
	CDODWaitForGrenadeTask(edict_t *pGrenade)
	{
		m_pGrenade = pGrenade;
		m_fTime    = 0.0f;
	}

	virtual void debugString(char *string);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

  private:
	MyEHandle m_pGrenade;
	float m_fTime;
};