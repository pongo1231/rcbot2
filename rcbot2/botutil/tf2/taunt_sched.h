#pragma once

#include "botutil/base_sched.h"

class CBotTauntSchedule : public CBotSchedule
{
  public:
	CBotTauntSchedule(edict_t *pPlayer, float fYaw);

	void init();

  private:
	MyEHandle m_pPlayer;
	float m_fYaw;
};