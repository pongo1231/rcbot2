#include "collect_cash_task.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/shared/move_to_task.h"

#include "bot_globals.h"
#include "bot_getprop.h"

CBotMVMCollectCashSched::CBotMVMCollectCashSched(edict_t *pCashPack)
{
	if (pCashPack)
	{
		addTask(new CFindPathTask(pCashPack));
		addTask(new CMoveToTask(pCashPack));
	}
}

void CBotMVMCollectCashSched::init()
{
	setID(SCHED_MVM_COLLECT_CASH);
}
