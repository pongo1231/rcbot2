#include "attack_point_task.h"

#include "bot_mods.h"
#include "bot_navigator.h"

CBotDODAttackPoint ::CBotDODAttackPoint(int iFlagID, Vector vOrigin, float fRadius)
{
	m_vOrigin     = vOrigin;
	m_fAttackTime = 0;
	m_fTime       = 0;
	m_iFlagID     = iFlagID;
	m_fRadius     = fRadius;
}

void CBotDODAttackPoint ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static int iTeam;

	iTeam = pBot->getTeam();

	if (CDODMod::m_Flags.ownsFlag(m_iFlagID, iTeam))
	{
		complete();

		pBot->resetAreaClear();

		if (pBot->inSquad() && pBot->isSquadLeader())
		{
			float fDanger       = MAX_BELIEF;
			IBotNavigator *pNav = pBot->getNavigator();

			fDanger             = pNav->getBelief(CDODMod::m_Flags.getWaypointAtFlag(m_iFlagID));

			if (randomFloat(0.0f, MAX_BELIEF) < fDanger)
			{
				pBot->addVoiceCommand(DOD_VC_HOLD);
				pBot->updateCondition(CONDITION_DEFENSIVE);
			}
		}

		return;
	}
	else if (m_fAttackTime == 0)
	{
		m_fAttackTime = engine->Time() + randomFloat(30.0, 60.0);
	}
	else if (m_fAttackTime < engine->Time())
	{
		complete();
		return;
	}
	else
	{
		if (m_fTime == 0)
		{
			m_fTime   = engine->Time() + randomFloat(2.0, 4.0);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_fRadius, m_fRadius), randomFloat(-m_fRadius, m_fRadius), 0);
			m_bProne  = (randomFloat(0, 1) * (1.0f - pBot->getHealthPercent())) > 0.75f;

			if (CDODMod::m_Flags.numFriendliesAtCap(m_iFlagID, iTeam)
			    < CDODMod::m_Flags.numCappersRequired(m_iFlagID, iTeam))
			{
				// count players I see
				CDODBot *pDODBot = (CDODBot *)pBot;

				pDODBot->addVoiceCommand(DOD_VC_NEED_BACKUP);
			}
		}
		else if (m_fTime < engine->Time())
		{
			m_fTime = 0;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if (m_bProne && !pBot->hasSomeConditions(CONDITION_RUN))
				pBot->duck();

			if (fdist < m_fRadius)
			{
				pBot->stopMoving();
				pBot->setLookAtTask(LOOK_AROUND);
			}
			else if (fdist > 400)
				fail();
			else
			{
				pBot->setMoveTo(m_vMoveTo);
			}
		}
	}
}

void CBotDODAttackPoint ::debugString(char *string)
{
	sprintf(string, "CBotDODAttackPoint\nm_iFlagID = %d\n m_vOrigin = (%0.1f,%0.1f,%0.1f,radius = %0.1f)", m_iFlagID,
	        m_vOrigin.x, m_vOrigin.y, m_vOrigin.z, m_fRadius);
}