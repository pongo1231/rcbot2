#include "find_last_enemy_task.h"

#include "bot_mtrand.h"

CFindLastEnemy::CFindLastEnemy(Vector vLast, Vector vVelocity)
{
	setCompleteInterrupt(CONDITION_SEE_CUR_ENEMY);
	m_vLast = vLast + (vVelocity * 10);
	m_fTime = 0;
}

void CFindLastEnemy::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fTime == 0)
		m_fTime = engine->Time() + randomFloat(2.0, 4.0);

	if (!pBot->moveToIsValid() || pBot->moveFailed())
		fail();
	if (pBot->distanceFrom(m_vLast) > 80)
		pBot->setMoveTo(m_vLast);
	else
		pBot->stopMoving();

	pBot->setLookAtTask(LOOK_AROUND);

	if (m_fTime < engine->Time())
		complete();
}