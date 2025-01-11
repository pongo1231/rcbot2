#include "pipe_enemy_sched.h"

CBotTF2DemoPipeEnemySched ::CBotTF2DemoPipeEnemySched(CBotWeapon *pLauncher, Vector vStand, edict_t *pEnemy)
{
	// addTask(new CFindPathTask(vStand));
	// addTask(new CBotTF2DemomanPipeEnemy(pLauncher,pEnemy));
}

void CBotTF2DemoPipeEnemySched ::init()
{
	setID(SCHED_TF2_DEMO_PIPEENEMY);
}