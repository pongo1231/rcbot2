#include "defend_task.h"

#include "bot_mods.h"
#include "bot_mtrand.h"

void CBotDefendTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static float fDist;

	if (m_fTime == 0)
	{
		if (pBot->inSquad() && pBot->isSquadLeader())
		{
			m_fTime = engine->Time() + randomFloat(15.0f, 45.0f);
		}
		else if (m_fMaxTime > 0)
			m_fTime = engine->Time() + m_fMaxTime;
		else
		{
			m_fTime = engine->Time() + randomFloat(20.0f, 90.0f);

			if (pBot->isTF2())
			{
				if (CTeamFortress2Mod::isMapType(TF_MAP_MVM))
				{
					Vector vFlag;

					if (CTeamFortress2Mod::getFlagLocation(TF2_TEAM_BLUE, &vFlag))
					{
						// FOR DEBUGGING
						float fDist     = (vFlag - m_vOrigin).Length();
						float fWaitTime = (90.0f - (fDist / 45)); // MAX Dist 4050
						//
						m_fTime         = engine->Time() + fWaitTime;
					}
				}
			}
		}
	}

	fDist = pBot->distanceFrom(m_vOrigin);

	if (fDist > 200)
		fail(); // too far -- bug
	else if (fDist > 100)
		pBot->setMoveTo(m_vOrigin);
	else
	{
		pBot->defending();

		pBot->stopMoving();

		if (m_iWaypointType & CWaypointTypes::W_FL_CROUCH)
			pBot->duck();

		if (m_bDefendOrigin)
		{
			pBot->setAiming(m_vDefendOrigin);
			pBot->setLookVector(m_vDefendOrigin);
		}

		pBot->setLookAtTask(m_LookTask);

		// pBot->setAiming(m_vDefendOrigin);

		/*if ( m_bDefendOrigin )
		{
		    pBot->setLookAtTask(LOOK_AROUND);
		    //pBot->setAiming(m_vDefendOrigin);
		}
		else
		    pBot->setLookAtTask(LOOK_SNIPE);*/
	}

	if (m_fTime < engine->Time())
	{
		if (pBot->inSquad() && pBot->isSquadLeader())
			pBot->addVoiceCommand(DOD_VC_GOGOGO);

		complete();
	}
}