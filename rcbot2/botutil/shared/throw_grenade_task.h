#pragma once

#include "botutil/base_task.h"

class CThrowGrenadeTask : public CBotTask
{
  public:
	CThrowGrenadeTask(CBotWeapon *pWeapon, int ammo, Vector vLoc);
	void init();
	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	Vector m_vLoc;
	CBotWeapon *m_pWeapon;
	float m_fTime;
	float m_fHoldAttackTime;
	int m_iAmmo;
};