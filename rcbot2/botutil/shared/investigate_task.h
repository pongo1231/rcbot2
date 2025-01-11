#pragma once

#include "botutil/base_task.h"

class CBotInvestigateTask : public CBotTask
{
  public:
	CBotInvestigateTask(Vector vOrigin, float fRadius, Vector vPOV, bool bHasPOV, float fMaxTime = 0,
	                    int iInterrupt = CONDITION_SEE_CUR_ENEMY)
	{
		m_fMaxTime = fMaxTime;
		m_vOrigin  = vOrigin;
		m_fRadius  = fRadius;
		m_fTime    = 0;
		setCompleteInterrupt(iInterrupt);
		m_iState  = 0;
		m_vPOV    = vPOV;
		m_bHasPOV = bHasPOV;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotInvestigateTask");
	}

  private:
	int m_iState;
	float m_fTime;
	float m_fMaxTime;
	Vector m_vOrigin;
	float m_fRadius;
	int m_iCurPath;
	bool m_bHasPOV;
	Vector m_vPOV;
	std::vector<Vector> m_InvPoints; // investigation points (waypoint paths)
};