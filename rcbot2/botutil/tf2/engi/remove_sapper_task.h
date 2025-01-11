#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotRemoveSapper : public CBotTask
{
  public:
	CBotRemoveSapper(edict_t *pBuilding, eEngiBuild id);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotRemoveSapper");
	}

  private:
	float m_fTime;
	float m_fHealTime;
	MyEHandle m_pBuilding;
	eEngiBuild m_id;
};