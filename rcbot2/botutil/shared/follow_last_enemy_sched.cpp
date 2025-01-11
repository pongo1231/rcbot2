#include "follow_last_enemy_sched.h"

#include "bot_client.h"
#include "bot_getprop.h"
#include "botutil/shared/find_last_enemy_task.h"
#include "botutil/shared/find_path_task.h"

CBotFollowLastEnemy ::CBotFollowLastEnemy(CBot *pBot, edict_t *pEnemy, Vector vLastSee)
{
	Vector vVelocity         = Vector(0, 0, 0);
	CClient *pClient         = CClients::get(pEnemy);

	CFindPathTask *pFindPath = new CFindPathTask(vLastSee, LOOK_LAST_ENEMY);

	if (CClassInterface ::getVelocity(pEnemy, &vVelocity))
	{
		if (pClient && (vVelocity == Vector(0, 0, 0)))
			vVelocity = pClient->getVelocity();
	}
	else if (pClient)
		vVelocity = pClient->getVelocity();

	pFindPath->setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);

	addTask(pFindPath);

	/*if ( pBot->isTF2() )
	{
	    int playerclass = ((CBotTF2*)pBot)->getClass();

	    if ( ( playerclass == TF_CLASS_SOLDIER ) || (playerclass == TF_CLASS_DEMOMAN) )
	        addTask(new CBotTF2ShootLastEnemyPosition(vLastSee,pEnemy,vVelocity));
	}*/

	addTask(new CFindLastEnemy(vLastSee, vVelocity));

	//////////////
	pFindPath->setNoInterruptions();
}

void CBotFollowLastEnemy ::init()
{
	setID(SCHED_FOLLOW_LAST_ENEMY);
}