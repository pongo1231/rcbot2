#pragma once

#include "botutil/base_task.h"

class CBotDefendTask : public CBotTask
{
  public:
	CBotDefendTask(Vector vOrigin, float fMaxTime = 0, int iInterrupt = CONDITION_SEE_CUR_ENEMY,
	               bool bDefendOrigin = false, Vector vDefendOrigin = Vector(0, 0, 0), eLookTask looktask = LOOK_SNIPE,
	               int iWaypointType = 0)
	{
		m_fMaxTime = fMaxTime;
		m_vOrigin  = vOrigin;
		m_fTime    = 0;
		setCompleteInterrupt(iInterrupt);
		m_bDefendOrigin = bDefendOrigin;
		m_vDefendOrigin = vDefendOrigin;
		m_LookTask      = looktask;
		m_iWaypointType = iWaypointType;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotDefendTask");
	}

  private:
	float m_fTime;
	float m_fMaxTime;
	Vector m_vOrigin;
	bool m_bDefendOrigin;
	Vector m_vDefendOrigin;
	eLookTask m_LookTask;
	int m_iWaypointType;
};