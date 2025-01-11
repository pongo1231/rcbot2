#pragma once

#include "botutil/base_task.h"

class CFindPathTask : public CBotTask
{
  public:
	CFindPathTask()
	{
		m_pEdict                    = nullptr;
		m_LookTask                  = LOOK_WAYPOINT;
		m_iWaypointId               = -1;
		m_flags.m_data              = 0;
		m_fRange                    = 0;
		m_iDangerPoint              = -1;
		m_bGetPassedIntAsWaypointId = false;
	}

	CFindPathTask(Vector vOrigin, eLookTask looktask = LOOK_WAYPOINT)
	{
		m_vVector                   = vOrigin;
		m_pEdict                    = nullptr; // no edict
		m_LookTask                  = looktask;
		m_iWaypointId               = -1;
		m_flags.m_data              = 0;
		m_fRange                    = 0;
		m_iDangerPoint              = -1;
		m_bGetPassedIntAsWaypointId = false;
	}

	// having this constructor saves us trying to find the goal waypoint again if we
	// already know it
	CFindPathTask(int iWaypointId, eLookTask looktask = LOOK_WAYPOINT);

	CFindPathTask(edict_t *pEdict);

	void setRange(float fRange)
	{
		m_fRange = fRange;
	}

	void setEdict(edict_t *pEdict)
	{
		m_pEdict = pEdict;
	}

	void setDangerPoint(int iWpt)
	{
		m_iDangerPoint = iWpt;
	}

	void getPassedVector()
	{
		m_flags.bits.m_bGetPassedVector = true;
	}

	void getPassedIntAsWaypointId()
	{
		m_bGetPassedIntAsWaypointId = true;
	}

	void dontGoToEdict()
	{
		m_flags.bits.m_bDontGoToEdict = true;
	}

	void setNoInterruptions()
	{
		clearFailInterrupts();
		m_flags.bits.m_bNoInterruptions = true;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void completeOutOfRangeFromEdict()
	{
		m_flags.bits.m_bCompleteOutOfRangeEdict = true;
	}

	void completeInRangeFromEdict()
	{
		m_flags.bits.m_bCompleteInRangeOfEdict = true;
	}

	void completeIfSeeTaskEdict()
	{
		m_flags.bits.m_bCompleteSeeTaskEdict = true;
	}

	void failIfTaskEdictDead()
	{
		m_flags.bits.m_bFailTaskEdictDied = true;
	}

	void init();

	void setLookTask(eLookTask task)
	{
		m_LookTask = task;
	}

	virtual void debugString(char *string);

  private:
	Vector m_vVector;
	MyEHandle m_pEdict;
	eLookTask m_LookTask;
	int m_iInt;
	int m_iWaypointId;
	int m_iDangerPoint;
	float m_fRange;

	union
	{
		byte m_data;

		struct
		{
			bool m_bNoInterruptions : 1; // quick path finding - non concurrent
			bool m_bGetPassedVector : 1; // receive vector from previous task
			bool m_bDontLookAtWaypoints : 1;
			bool m_bCompleteSeeTaskEdict : 1;    // complete when see the edict
			bool m_bFailTaskEdictDied : 1;       // fail if the edict no longer exists or dead
			bool m_bDontGoToEdict : 1;           // don't complete if nearby edict
			bool m_bCompleteOutOfRangeEdict : 1; // complete if outside of m_fRange from edict (grenades)
			bool m_bCompleteInRangeOfEdict : 1;
		} bits;
	} m_flags;

	bool m_bGetPassedIntAsWaypointId;
	// bool m_bWaitUntilReached;
};