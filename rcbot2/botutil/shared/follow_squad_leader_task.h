#pragma once

#include "botutil/base_task.h"

class CBotFollowSquadLeader : public CBotTask
{
  public:
	CBotFollowSquadLeader(CBotSquad *pSquad)
	{
		m_fLeaderSpeed       = 0.0f;
		m_pSquad             = pSquad;
		m_fVisibleTime       = 0.0f;
		m_fUpdateMovePosTime = 0.0f;
		m_vPos               = Vector(0, 0, 0);
		m_vForward           = Vector(0, 0, 0);
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotFollowSquadLeader");
	}

  private:
	CBotSquad *m_pSquad;
	float m_fVisibleTime;
	float m_fUpdateMovePosTime;
	float m_fLeaderSpeed;
	Vector m_vPos;
	Vector m_vForward;
};