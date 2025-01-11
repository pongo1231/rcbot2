#pragma once

#include "botutil/base_task.h"

class CBotTF2AttackSentryGunTask : public CBotTask
{
  public:
	CBotTF2AttackSentryGunTask(edict_t *pSentryGun, CBotWeapon *pWeapon);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void init()
	{
		m_fTime = 0.0f;
	}

	void debugString(char *string);

  private:
	MyEHandle m_pSentryGun;
	CBotWeapon *m_pWeapon;
	int m_iStartingWaypoint;
	int m_iSentryWaypoint;
	Vector m_vStart;
	Vector m_vHide;
	float m_fDist;
	float m_fTime;
};