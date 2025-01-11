#pragma once

#include "botutil/base_task.h"

class CBotInvestigateHidePoint : public CBotTask
{
	// investigate a  possible enemy hiding point

  public:
	CBotInvestigateHidePoint(int iWaypointIndexToInvestigate, int iOriginalWaypointIndex);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	std::vector<Vector> m_CheckPoints;
	unsigned int m_iCurrentCheckPoint;
	float m_fInvestigateTime;
	float m_fTime;
	int m_iState;
};