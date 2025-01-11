#include "investigate_noise_sched.h"

#include "botutil/shared/find_path_task.h"
#include "botutil/shared/investigate_task.h"

CBotInvestigateNoiseSched::CBotInvestigateNoiseSched(Vector vSource, Vector vAttackPoint)
{
	addTask(new CFindPathTask(vSource, LOOK_NOISE));
	addTask(new CBotInvestigateTask(vSource, 200.0f, vAttackPoint, true, 3.0f));
}

void CBotInvestigateNoiseSched::init()
{
	setID(SCHED_INVESTIGATE_NOISE);
}