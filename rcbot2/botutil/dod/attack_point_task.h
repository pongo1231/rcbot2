#pragma once

#include "botutil/base_task.h"

class CBotDODAttackPoint : public CBotTask
{
  public:
	CBotDODAttackPoint(int iFlagID, Vector vOrigin, float fRadius);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	Vector m_vMoveTo;
	float m_fAttackTime;
	float m_fTime;
	int m_iFlagID;
	float m_fRadius;
	bool m_bProne;
};