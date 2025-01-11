#pragma once

#include "botutil/base_task.h"

#define CHARGER_HEALTH 0
#define CHARGER_ARMOR 1

class CBotHL2DMUseCharger : public CBotTask
{
  public:
	CBotHL2DMUseCharger(edict_t *pCharger, int type)
	{
		m_pCharger = pCharger;
		m_fTime    = 0;
		m_iType    = type;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "Use Charger");
	}

  private:
	MyEHandle m_pCharger;
	float m_fTime;
	int m_iType;
};