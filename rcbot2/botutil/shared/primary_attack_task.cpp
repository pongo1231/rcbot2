#include "primary_attack_task.h"

void CPrimaryAttack::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->primaryAttack();
	complete();
}