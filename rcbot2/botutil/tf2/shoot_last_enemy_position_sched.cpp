#include "shoot_last_enemy_position_sched.h"

#include "botutil/tf2/shoot_last_enemy_position_task.h"

CBotTF2ShootLastEnemyPos::CBotTF2ShootLastEnemyPos(Vector vLastSeeEnemyPos, Vector vVelocity, edict_t *pLastEnemy)
{
	addTask(new CBotTF2ShootLastEnemyPosition(vLastSeeEnemyPos, pLastEnemy, vVelocity));
}

void CBotTF2ShootLastEnemyPos::init()
{
	setID(SCHED_SHOOT_LAST_ENEMY_POS);
}