#pragma once

#include "botutil/base_task.h"

// defensive technique
class CBotTF2Spam : public CBotTask
{
  public:
	CBotTF2Spam(Vector vStart, Vector vTarget, CBotWeapon *pWeapon);

	CBotTF2Spam(CBot *pBot, Vector vStart, int iYaw, CBotWeapon *pWeapon);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	Vector getTarget()
	{
		return m_vTarget;
	}

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2Spam");
	}

	float getDistance();

  private:
	Vector m_vTarget;
	Vector m_vStart;
	CBotWeapon *m_pWeapon;
	float m_fTime;

	float m_fNextAttack;
};