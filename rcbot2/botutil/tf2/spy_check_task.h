#pragma once

#include "botutil/base_task.h"

class CSpyCheckAir : public CBotTask
{
  public:
	CSpyCheckAir()
	{
		m_fTime         = 0.f;
		m_pUnseenBefore = nullptr;
		m_bHitPlayer    = false;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string);

  private:
	edict_t *m_pUnseenBefore;
	int seenlist;
	float m_fNextCheckUnseen;
	float m_fTime;
	bool m_bHitPlayer;
};