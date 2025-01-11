#pragma once

#include "botutil/base_task.h"

class CBotTF2UpgradeBuilding : public CBotTask
{
  public:
	CBotTF2UpgradeBuilding(edict_t *pBuilding);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pBuilding;
	float m_fTime;
};