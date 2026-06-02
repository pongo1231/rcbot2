#pragma once

#include "botutil/base_sched.h"
#include "botutil/base_task.h"

class CBotMVMCollectCashTask : public CBotTask
{
  public:
	CBotMVMCollectCashTask()
	{
		setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);
};

class CBotMVMCollectCashSched : public CBotSchedule
{
  public:
	CBotMVMCollectCashSched(edict_t *pCashPack);

	void init();
};
