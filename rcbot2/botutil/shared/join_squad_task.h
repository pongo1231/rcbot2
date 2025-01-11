#pragma once

#include "botutil/base_task.h"

class CBotJoinSquad : public CBotTask
{
  public:
	CBotJoinSquad(edict_t *pPlayerToJoin)
	{
		m_pPlayer = pPlayerToJoin;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotFollowSquadLeader");
	}

  private:
	edict_t *m_pPlayer;
};