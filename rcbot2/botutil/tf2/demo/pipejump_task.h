#pragma once

#include "botutil/base_task.h"

#define TASK_TF2_DEMO_STATE_LAY_BOMB 0
#define TASK_TF2_DEMO_STATE_RUN_UP 1

class CBotTF2DemomanPipeJump : public CBotTask
{
  public:
	CBotTF2DemomanPipeJump(CBot *pBot, Vector vWaypointGround, Vector vWaypointNext, CBotWeapon *pWeapon);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2DemomanPipeJump");
	}

  private:
	Vector m_vStart;
	Vector m_vPipe;
	Vector m_vEnd;
	edict_t *m_pPipeBomb;
	bool m_bFired;
	float m_fTime;
	int m_iState;
	int m_iStartingAmmo;
	CBotWeapon *m_pWeapon;
};