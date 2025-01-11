#pragma once

#include "botutil/base_task.h"

class CBotTF2SpyDisguise : public CBotTask
{
  public:
	CBotTF2SpyDisguise();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2SpyDisguise");
	}
};