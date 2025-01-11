#pragma once

#include "botutil/base_task.h"

class CBotHL2DMSnipe : public CBotTask
{
  public:
	CBotHL2DMSnipe(CBotWeapon *pWeaponToUse, Vector vOrigin, float fYaw, bool bUseZ = false, float z = 0,
	               int iWaypointType = 0);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string);

  private:
	float m_fTime;
	float m_fEnemyTime;
	float m_fScopeTime;
	Vector m_vAim;
	Vector m_vOrigin;
	CBotWeapon *m_pWeaponToUse;
	bool m_bUseZ;
	float m_z; // z = ground level
	int m_iWaypointType;
};