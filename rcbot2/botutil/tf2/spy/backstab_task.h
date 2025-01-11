#pragma once

#include "botutil/base_task.h"

class CBotBackstab : public CBotTask
{
  public:
	CBotBackstab(edict_t *_pEnemy);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotBackstab");
	}

  private:
	float m_fTime;
	MyEHandle pEnemy;
};