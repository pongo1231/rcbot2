#pragma once

#include "botutil/base_task.h"

class CBotTaskEngiPickupBuilding : public CBotTask
{
  public:
	CBotTaskEngiPickupBuilding(edict_t *pBuilding); // going to use this
	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	MyEHandle m_pBuilding;
	float m_fTime;
};