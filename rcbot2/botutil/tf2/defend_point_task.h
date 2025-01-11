#pragma once

#include "botutil/base_task.h"

class CBotTF2DefendPoint : public CBotTask
{
  public:
	CBotTF2DefendPoint(int iArea, Vector vOrigin, int iRadius);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	float m_fDefendTime;
	float m_fTime;
	int m_iArea;
	int m_iRadius;
};