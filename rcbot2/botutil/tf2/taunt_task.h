#pragma once

#include "botutil/base_task.h"

class CTF2_TauntTask : public CBotTask
{
  public:
	CTF2_TauntTask(edict_t *pPlayer, Vector vOrigin, float fDist)
	{
		m_pPlayer = pPlayer;
		m_vOrigin = vOrigin;
		m_fDist   = fDist;
	}

	void init();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pPlayer;
	Vector m_vOrigin;
	float m_fDist;
	float m_fTime;
	float m_fTauntUntil;
	float m_fActionTime;
};