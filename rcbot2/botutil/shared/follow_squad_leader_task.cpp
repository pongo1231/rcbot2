#include "follow_squad_leader_task.h"

#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_squads.h"

void CBotFollowSquadLeader ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	edict_t *pSquadLeader;
	float fDist;

	if (!pBot->inSquad(m_pSquad))
	{
		fail();
		return;
	}

	pSquadLeader = m_pSquad->GetLeader();

	if (pSquadLeader == pBot->getEdict())
	{
		fail();
		return;
	}

	if (CClassInterface::getMoveType(pSquadLeader) == MOVETYPE_LADDER)
	{
		pBot->stopMoving();
		return;
	}

	if ((m_fVisibleTime > 0.0f) && !pBot->hasEnemy() && !pBot->isVisible(pSquadLeader))
	{
		// haven't seen leader for five seconds
		if ((engine->Time() - m_fVisibleTime) > 0.2f)
		{
			complete();
			return;
		}
	}
	else
		m_fVisibleTime = engine->Time();

	if (pBot->hasSomeConditions(CONDITION_SQUAD_IDLE))
	{
		// squad leader idle for 3 seconds. see what else i can do
		complete();
		return;
	}

	if (m_fUpdateMovePosTime < engine->Time())
	{
		float fRand;
		Vector vVelocity;

		fRand = randomFloat(1.0f, 2.0f);

		CClassInterface::getVelocity(pSquadLeader, &vVelocity);

		m_fLeaderSpeed       = (MIN(m_fLeaderSpeed, 100.0f) * 0.5f) + (vVelocity.Length() * 0.5f);

		m_fUpdateMovePosTime = engine->Time() + (fRand * (1.0f - (vVelocity.Length() / 320)));
		m_vPos               = m_pSquad->GetFormationVector(pBot->getEdict());

		m_vForward           = m_vPos + vVelocity;
	}

	fDist = pBot->distanceFrom(m_vPos); // More than reachable range

	if (fDist > 400.0f)
	{
		fail(); // find a path instead
		return;
	}
	else if (fDist > m_pSquad->GetSpread())
	{
		pBot->setMoveTo(m_vPos);
		pBot->setSquadIdleTime(engine->Time());

		if (pBot->isVisible(pSquadLeader))
			pBot->setMoveSpeed(m_fLeaderSpeed);
	}
	else
	{
		// idle
		pBot->stopMoving();
		pBot->SquadInPosition();
	}

	pBot->setLookVector(m_vForward);
	pBot->setLookAtTask(LOOK_VECTOR);
}