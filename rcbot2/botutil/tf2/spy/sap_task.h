#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotTF2SpySap : public CBotTask
{
  public:
	CBotTF2SpySap(edict_t *pBuilding, eEngiBuild id); // going to use this

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pBuilding;
	float m_fTime;
	eEngiBuild m_id;
};