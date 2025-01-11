#include "find_path_task.h"

#include "bot_globals.h"
#include "bot_navigator.h"
#include "bot_profiling.h"
#include "bot_waypoint_locations.h"
#include "botutil/base_sched.h"

CFindPathTask ::CFindPathTask(int iWaypointId, eLookTask looktask)
{
	m_iWaypointId               = iWaypointId;
	m_LookTask                  = looktask;
	m_vVector                   = CWaypoints::getWaypoint(iWaypointId)->getOrigin();
	m_flags.m_data              = 0;
	m_fRange                    = 0.0f;
	m_iDangerPoint              = -1;
	m_bGetPassedIntAsWaypointId = false;
}

void CFindPathTask ::init()
{
	m_flags.m_data              = 0;
	m_iInt                      = 0;
	m_fRange                    = 0.0f;
	m_iDangerPoint              = -1;
	m_bGetPassedIntAsWaypointId = false;
	// setFailInterrupt(CONDITION_SEE_CUR_ENEMY);
}

CFindPathTask ::CFindPathTask(edict_t *pEdict)
{
	m_iWaypointId               = -1;
	m_pEdict                    = pEdict;
	m_vVector                   = CBotGlobals::entityOrigin(pEdict);
	m_LookTask                  = LOOK_WAYPOINT;
	m_flags.m_data              = 0;
	m_fRange                    = 0.0f;
	m_iDangerPoint              = -1;
	m_bGetPassedIntAsWaypointId = false;
}

void CFindPathTask ::debugString(char *string)
{
	sprintf(string, "CFindPathTask\n m_iInt = %d\n m_vVector = (%0.4f,%0.4f,%0.4f)", m_iInt, m_vVector.x, m_vVector.y,
	        m_vVector.z);
}

void CFindPathTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	bool bFail = false;

	if (m_LookTask == LOOK_NOISE)
		pBot->wantToListen(false); // vector already set before find path task

	if (m_bGetPassedIntAsWaypointId)
	{
		m_iWaypointId = pSchedule->passedInt();

		if (m_iWaypointId == -1)
		{
			fail();
			return;
		}

		m_vVector = CWaypoints::getWaypoint(m_iWaypointId)->getOrigin();
	}
	else if (pSchedule->hasPassVector())
	{
		m_vVector = pSchedule->passedVector();
		pSchedule->clearPass();
	}

	if ((m_iInt == 0) || (m_iInt == 1))
	{
		IBotNavigator *pNav        = pBot->getNavigator();

		pBot->m_fWaypointStuckTime = 0;

#ifdef _DEBUG
		CProfileTimer *timer = CProfileTimers::getTimer(BOT_ROUTE_TIMER);

		if (CClients::clientsDebugging(BOT_DEBUG_PROFILE))
		{
			timer->Start();
		}
#endif

		if (pNav->workRoute(pBot->getOrigin(), m_vVector, &bFail, (m_iInt == 0), m_flags.bits.m_bNoInterruptions,
		                    m_iWaypointId, pBot->getConditions(), m_iDangerPoint))
		{
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(10.0f, 15.0f);
			pBot->moveFailed(); // reset
			m_iInt = 2;
		}
		else
			m_iInt = 1;

#ifdef _DEBUG
		if (CClients::clientsDebugging(BOT_DEBUG_PROFILE))
		{
			timer->Stop();
		}
#endif

		pBot->debugMsg(BOT_DEBUG_NAV, "Trying to work out route");
	}

	if (bFail)
	{
		pBot->debugMsg(BOT_DEBUG_NAV, "Route failed");
		fail();
	}
	else if (m_iInt == 2)
	{
		if (pBot->m_fWaypointStuckTime == 0)
			pBot->m_fWaypointStuckTime = engine->Time() + randomFloat(5.0f, 10.0f);

		// if ( m_bNoInterruptions )
		//{
		// pBot->debugMsg(BOT_DEBUG_NAV,"Found route");
		// complete(); // ~fin~
		//}

		if (!pBot->getNavigator()->hasNextPoint())
		{
			pBot->debugMsg(BOT_DEBUG_NAV, "Nowhere to go");
			complete(); // reached goal
		}
		else
		{
			if (pBot->moveFailed())
			{
				pBot->debugMsg(BOT_DEBUG_NAV, "moveFailed() == true");
				fail();
				pBot->getNavigator()->failMove();
			}

			if (m_pEdict)
			{
				if (CBotGlobals::entityIsValid(m_pEdict))
				{
					pBot->lookAtEdict(m_pEdict);

					if (m_flags.bits.m_bCompleteInRangeOfEdict && m_flags.bits.m_bCompleteSeeTaskEdict)
					{
						// complete if inrange AND see edict
						if ((m_flags.bits.m_bCompleteInRangeOfEdict && (pBot->distanceFrom(m_pEdict) < m_fRange))
						    && pBot->isVisible(m_pEdict))
							complete();
					}
					else if (!m_flags.bits.m_bDontGoToEdict && pBot->isVisible(m_pEdict))
					{
						if (pBot->distanceFrom(m_pEdict) < pBot->distanceFrom(pBot->getNavigator()->getNextPoint()))
							complete();
					}
					else if (m_flags.bits.m_bCompleteOutOfRangeEdict && (pBot->distanceFrom(m_pEdict) > m_fRange))
						complete();
					else if (m_flags.bits.m_bCompleteInRangeOfEdict && (pBot->distanceFrom(m_pEdict) < m_fRange))
						complete();
				}
				else
					fail();
			}

			//// running path
			// if ( !pBot->hasEnemy() && !pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )

			pBot->setLookAtTask(m_LookTask);
		}
	}

	if (m_pEdict.get() != nullptr) // task edict
	{
		if (m_flags.bits.m_bCompleteSeeTaskEdict)
		{
			if (pBot->isVisible(m_pEdict.get())
			    && (pBot->distanceFrom(CBotGlobals::entityOrigin(m_pEdict)) < CWaypointLocations::REACHABLE_RANGE))
				complete();
		}

		if (m_flags.bits.m_bFailTaskEdictDied)
		{
			if ((m_pEdict == nullptr) || !CBotGlobals::entityIsAlive(m_pEdict))
			{
				fail();
			}
		}
	}
	else if (m_flags.bits.m_bFailTaskEdictDied)
		fail();
}