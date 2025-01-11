#pragma once

#include "botutil/base_task.h"

class CBotGravGunPickup : public CBotTask
{
  public:
	CBotGravGunPickup(edict_t *pWeapon, edict_t *pProp)
	{
		m_Weapon = pWeapon;
		m_Prop   = pProp;
		m_fTime  = 0;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "Grav Gun Pickup");
	}

  private:
	MyEHandle m_Weapon;
	MyEHandle m_Prop;
	float m_fTime;
	float m_fSecAttTime;
};