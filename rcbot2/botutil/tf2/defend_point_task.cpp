#include "defend_point_task.h"

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
		m_fDefendTime = engine->Time() + randomFloat(30.0, 60.0);

		// Generate 3-5 patrol points around the origin
		m_iPointCount = randomInt(3, 6);
		for (int i = 0; i < m_iPointCount; i++)
		{
			m_PatrolPoints[i] = m_vOrigin + Vector(
				randomFloat(-m_iRadius, m_iRadius),
				randomFloat(-m_iRadius, m_iRadius), 0);
		}
		m_iCurrentPoint = 0;
		m_fTime         = 0;
		pBot->resetLookAroundTime();
	}
	else if (m_fDefendTime < engine->Time())
		complete();
	else
	{
		// Active patrol: cycle through patrol points every ~2-4 seconds
		if (m_fTime == 0)
		{
			m_fTime          = engine->Time() + randomFloat(2.0f, 4.0f);
			m_vMoveTo        = m_PatrolPoints[m_iCurrentPoint];

			// Avoid clustering with teammates
			Vector vOffset(0, 0, 0);
			int iNearbyCount = 0;
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				edict_t *pPlayer = INDEXENT(i);
				if (!pPlayer || pPlayer == pBot->getEdict())
					continue;
				if (!CBotGlobals::entityIsValid(pPlayer) || !CBotGlobals::entityIsAlive(pPlayer))
					continue;
				if (CTeamFortress2Mod::getTeam(pPlayer) != iTeam)
					continue;

				Vector vTeammatePos = CBotGlobals::entityOrigin(pPlayer);
				float fTeammateDist = (m_vMoveTo - vTeammatePos).Length();

				if (fTeammateDist < 80.0f)
				{
					Vector vAway = m_vMoveTo - vTeammatePos;
					vAway.z      = 0;
					if (vAway.Length() > 0.1f)
					{
						vAway  = vAway / vAway.Length();
						vOffset = vOffset + (vAway * (80.0f - fTeammateDist));
						iNearbyCount++;
					}
				}
			}
			if (iNearbyCount > 0)
				m_vMoveTo = m_vMoveTo + vOffset;

			m_iCurrentPoint = (m_iCurrentPoint + 1) % m_iPointCount;
		}
		else if (m_fTime < engine->Time())
			m_fTime = 0;

		float fdist = pBot->distanceFrom(m_vMoveTo);
		if (fdist < 32)
		{
			pBot->stopMoving();
			m_fTime = 0; // immediately pick next point
		}
		else if (fdist > 400)
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
