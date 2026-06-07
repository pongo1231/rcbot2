#pragma once

#include "botutil/base_task.h"

class CMessAround : public CBotTask
{
  public:
	CMessAround(edict_t *pFriendly, int iMaxVoiceCmd);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CMessAround");
	}

  private:
	float m_fTime;
	float m_fSubTime;
	float m_fWanderTime;
	MyEHandle m_pFriendly;
	int m_iMaxVoiceCmd;
	int m_iType;
	int m_iVoiceCmd;
	int m_iSubState;
	bool m_bTagIsRunner;
};
