#pragma once

#include "botutil/base_task.h"

class CBotTF2EngineerInterrupt : public IBotTaskInterrupt
{
  public:
	CBotTF2EngineerInterrupt(CBot *pBot);

	bool isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted);

  private:
	float m_fPrevSentryHealth;
	MyEHandle m_pSentryGun;
	CBotWeapon *pWrench;
};