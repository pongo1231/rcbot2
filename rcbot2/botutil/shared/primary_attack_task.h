#pragma once

#include "botutil/base_task.h"

class CPrimaryAttack : public CBotTask
{
  public:
	void execute(CBot *pBot, CBotSchedule *pSchedule);
};