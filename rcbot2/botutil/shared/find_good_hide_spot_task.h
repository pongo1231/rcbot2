#pragma once

#include "botutil/base_task.h"

class CFindGoodHideSpot : public CBotTask
{
  public:
	CFindGoodHideSpot(edict_t *pEntity);

	CFindGoodHideSpot(Vector vec);

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CFindGoodHideSpot");
	}

  private:
	Vector m_vHideFrom;
};