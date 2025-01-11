#pragma once

#include "botutil/base_task.h"

class CBotTF2CoverInterrupt : public IBotTaskInterrupt
{
  public:
	bool isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted);
};
