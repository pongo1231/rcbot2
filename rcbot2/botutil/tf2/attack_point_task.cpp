#include "attack_point_task.h"

#include "bot_mods.h"

#include <in_buttons.h>

CBotTF2AttackPoint ::CBotTF2AttackPoint(int iArea, Vector vOrigin, int iRadius)
{
	m_vOrigin     = vOrigin;
	m_fAttackTime = 0;
	m_fTime       = 0;
	m_iArea       = iArea;
	m_iRadius     = iRadius;
}

void CBotTF2AttackPoint ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	int iCpIndex     = CTeamFortress2Mod::m_ObjectiveResource.m_WaypointAreaToIndexTranslation[m_iArea];
	int iTeam        = pBot->getTeam();
	CBotTF2 *pTF2Bot = (CBotTF2 *)pBot;

	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
		fail();

	pBot->wantToInvestigateSound(false);

	if (m_iArea && (CTeamFortress2Mod::m_ObjectiveResource.GetOwningTeam(iCpIndex) == iTeam))
	{
		complete(); // done
		pTF2Bot->updateAttackDefendPoints();
	}
	else if (m_iArea && !CTeamFortress2Mod::m_ObjectiveResource.isCPValid(iCpIndex, iTeam, TF2_POINT_ATTACK))
	{
		fail(); // too slow
		pTF2Bot->updateAttackDefendPoints();
	}
	else if (m_fAttackTime == 0)
		m_fAttackTime = engine->Time() + randomFloat(30.0, 60.0);
	else if (m_fAttackTime < engine->Time())
		complete();
	else
	{
		if (m_fTime == 0)
		{

			m_fTime   = engine->Time() + randomFloat(5.0, 10.0);
			m_vMoveTo = m_vOrigin + Vector(randomFloat(-m_iRadius, m_iRadius), randomFloat(-m_iRadius, m_iRadius), 0);
		}
		else if (m_fTime < engine->Time())
		{
			m_fTime = 0;
		}
		else
		{
			static float fdist;

			fdist = pBot->distanceFrom(m_vMoveTo);

			if (pTF2Bot->getClass() == TF_CLASS_SPY)
			{
				if (pTF2Bot->isDisguised())
					pBot->primaryAttack(); // remove disguise to capture

				pTF2Bot->wantToDisguise(false);

				// block cloaking
				if (pTF2Bot->isCloaked())
				{
					// uncloak
					pTF2Bot->spyUnCloak();
				}
				else
				{
					pBot->letGoOfButton(IN_ATTACK2);
				}

				pTF2Bot->waitCloak();
			}

			if (fdist < 52)
			{
				pBot->stopMoving();
			}
			else if (fdist > 400)
				fail();
			else
			{
				pBot->setMoveTo((m_vMoveTo));
			}

			pBot->setLookAtTask(LOOK_AROUND);

			if (((CBotTF2 *)pBot)->checkAttackPoint())
				complete();
		}
	}
}

void CBotTF2AttackPoint ::debugString(char *string)
{
	sprintf(string, "CBotTF2AttackPoint (%d,%0.1f,%0.1f,%0.1f,%d)", m_iArea, m_vOrigin.x, m_vOrigin.y, m_vOrigin.z,
	        m_iRadius);
}