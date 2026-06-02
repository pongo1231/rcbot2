#include "defend_point_task.h"

#include "bot_const.h"
#include "bot_globals.h"
#include "bot_mods.h"

CBotTF2DefendPoint::CBotTF2DefendPoint(int iArea, Vector vOrigin, int iRadius)
{
	m_vOrigin     = vOrigin;
	m_fDefendTime = 0;
	m_fTime       = 0;
	m_iArea       = iArea;
	m_iRadius     = iRadius;
	m_iPointCount = 0;

	setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
	setCompleteInterrupt(CONDITION_CHANGED);
}

void CBotTF2DefendPoint::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	int iCpIndex = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iArea];
	int iTeam    = pBot->getTeam();

	if (m_iArea && (CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(iCpIndex) != iTeam))
	{
		((CBotTF2 *)pBot)->updateAttackDefendPoints();
		complete();
	}
	else if (m_iArea && !CTeamFortress2Mod::m_ObjectiveResource.isCPValid(iCpIndex, iTeam, TF2_POINT_DEFEND))
	{
		((CBotTF2 *)pBot)->updateAttackDefendPoints();
		fail();
	}
	else if (m_fDefendTime == 0)
	{
		m_fDefendTime = engine->Time() + randomFloat(15.0, 30.0);

		// Count how many teammates are already defending this area to avoid overstacking
		int iDefendersHere = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t *pPlayer = INDEXENT(i);
			if (!pPlayer || pPlayer == pBot->getEdict()) continue;
			if (!CBotGlobals::entityIsValid(pPlayer) || !CBotGlobals::entityIsAlive(pPlayer)) continue;
			if (CTeamFortress2Mod::getTeam(pPlayer) != iTeam) continue;
				Vector vDiff = CBotGlobals::entityOrigin(pPlayer) - m_vOrigin;
			vDiff.z = 0;
			if (vDiff.Length() < 512.0f)
				iDefendersHere++;
		}

		// If crowded, bail so getTasks() can pick a different area
		if (iDefendersHere >= 3)
		{
			((CBotTF2 *)pBot)->updateAttackDefendPoints();
			fail();
			return;
		}

		// Use at least 400 unit patrol radius so bots actually spread out
		float fPatrolRadius = (float)m_iRadius;
		if (fPatrolRadius < 400.0f)
			fPatrolRadius = 400.0f;

		// Generate 4-6 patrol points spread around the perimeter, not randomly inside
		m_iPointCount = randomInt(4, 7);
		for (int i = 0; i < m_iPointCount; i++)
		{
			float fAngle = ((float)i / (float)m_iPointCount) * 6.28318f
			             + randomFloat(-0.3f, 0.3f); // small jitter to avoid exact alignment
			float fDist  = fPatrolRadius * randomFloat(0.5f, 1.0f); // some variation in distance
			m_PatrolPoints[i] = m_vOrigin + Vector(
				cos(fAngle) * fDist,
				sin(fAngle) * fDist, 0);
		}
		m_iCurrentPoint = 0;
		m_fTime         = 0;
		pBot->resetLookAroundTime();
	}
	else if (m_fDefendTime < engine->Time())
		complete();
	else
	{
		// Active patrol: cycle through patrol points every ~3-5 seconds
		if (m_fTime == 0)
		{
			m_fTime          = engine->Time() + randomFloat(3.0f, 5.0f);
			m_vMoveTo        = m_PatrolPoints[m_iCurrentPoint];

			// Avoid clustering with teammates — if any teammate is within 150 units
			// of our chosen point, skip to the next one
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				edict_t *pPlayer = INDEXENT(i);
				if (!pPlayer || pPlayer == pBot->getEdict()) continue;
				if (!CBotGlobals::entityIsValid(pPlayer) || !CBotGlobals::entityIsAlive(pPlayer)) continue;
				if (CTeamFortress2Mod::getTeam(pPlayer) != iTeam) continue;

				Vector vTeammatePos = CBotGlobals::entityOrigin(pPlayer);
				Vector vToMate = m_vMoveTo - vTeammatePos;
				vToMate.z = 0;
				float fTeammateDist = vToMate.Length();

				if (fTeammateDist < 150.0f)
				{
					// Skip this point, try next one
					m_iCurrentPoint = (m_iCurrentPoint + 1) % m_iPointCount;
					m_fTime         = 0;
					return;
				}
			}

			m_iCurrentPoint = (m_iCurrentPoint + 1) % m_iPointCount;
		}
		else if (m_fTime < engine->Time())
			m_fTime = 0;

		float fdist = pBot->distanceFrom(m_vMoveTo);
		if (fdist < 48.0f)
		{
			pBot->stopMoving();
			m_fTime = 0; // pick next point
		}
		else if (fdist > 600.0f)
			fail();
		else
			pBot->setMoveTo(m_vMoveTo);

		// Actively look around while patrolling, not just stand and snipe
		pBot->setLookAtTask(LOOK_AROUND);
	}
}

void CBotTF2DefendPoint::debugString(char *string)
{
	sprintf(string, "CBotTF2DefendPoint\nm_iArea=%d\nm_vOrigin=(%0.1f,%0.1f,%0.1f,%d)", m_iArea, m_vOrigin.x,
	        m_vOrigin.y, m_vOrigin.z, m_iRadius);
}
