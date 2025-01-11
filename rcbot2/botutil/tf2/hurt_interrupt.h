#pragma once

#include "botutil/base_task.h"

class CBotTF2HurtInterrupt : public IBotTaskInterrupt
{
  public:
	CBotTF2HurtInterrupt(CBot *pBot);

	bool isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted);

  private:
	float m_iHealth;
};