#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotTF2DemomanPipeTrap : public CBotTask
{
  public:
	// Set up a pipe trap or fire pipe bombs --
	// if autodetonate, detonate them when I've shot them rather than wait for an enemy
	// such as when attacking a sentry
	CBotTF2DemomanPipeTrap(eDemoTrapType type, Vector vStand, Vector vLoc, Vector vSpread, bool bAutoDetonate = false,
	                       int m_iWptArea = -1);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2DemomanPipeTrap");
	}

  private:
	Vector m_vPoint;
	Vector m_vStand;
	Vector m_vLocation;
	Vector m_vSpread;
	float m_fTime;
	eDemoTrapType m_iTrapType;
	int m_iState;
	int m_iStickies;
	bool m_bAutoDetonate;
	int m_iWptArea;
};