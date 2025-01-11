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
	MyEHandle m_pFriendly;
	int m_iMaxVoiceCmd;
	int m_iType; // 0 = attack friendly , 1 = taunt, 2 = random voice command
};