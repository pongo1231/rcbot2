#pragma once

#include "botutil/base_task.h"

class CBotTF2EngiLookAfter : public CBotTask
{
  public:
	CBotTF2EngiLookAfter(edict_t *pSentry)
	{
		m_pSentry    = pSentry;
		m_fTime      = 0;
		m_fHitSentry = 0;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2EngiLookAfter");
	}

  private:
	float m_fTime;
	float m_fHitSentry;
	MyEHandle m_pSentry;
};