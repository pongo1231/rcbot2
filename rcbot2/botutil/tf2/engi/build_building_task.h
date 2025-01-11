#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotTFEngiBuildTask : public CBotTask
{
  public:
	CBotTFEngiBuildTask(eEngiBuild iObject, CWaypoint *pWaypoint);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

	void oneTryOnly()
	{
		m_iTries = 2;
	}

  private:
	Vector m_vOrigin;
	eEngiBuild m_iObject;
	int m_iState;
	float m_fTime;
	int m_iTries;
	float m_fNextUpdateAngle;
	Vector m_vAimingVector;
	int m_iArea;
	Vector m_vBaseOrigin;
	float m_fRadius;
};