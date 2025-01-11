#pragma once

#include "botutil/base_task.h"

class CDODDropAmmoTask : public CBotTask
{
  public:
	CDODDropAmmoTask(edict_t *pPlayer)
	{
		m_fTime   = 0.0f;
		m_pPlayer = pPlayer;
	}

	virtual void debugString(char *string);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

  private:
	MyEHandle m_pPlayer;
	float m_fTime;
};