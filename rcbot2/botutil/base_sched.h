#pragma once

#include "bot.h"
#include "botutil/base_task.h"

#define BITS_SCHED_PASS_INT (1 << 0)
#define BITS_SCHED_PASS_FLOAT (1 << 1)
#define BITS_SCHED_PASS_VECTOR (1 << 2)
#define BITS_SCHED_PASS_EDICT (1 << 3)

typedef enum
{
	SCHED_NONE = 0,
	SCHED_ATTACK,
	SCHED_RUN_FOR_COVER,
	SCHED_GOTO_ORIGIN,
	SCHED_GOOD_HIDE_SPOT,
	SCHED_TF2_GET_FLAG,
	SCHED_TF2_GET_HEALTH,
	SCHED_TF_BUILD,
	SCHED_HEAL,
	SCHED_GET_METAL,
	SCHED_SNIPE,
	SCHED_UPGRADE,
	SCHED_USE_TELE,
	SCHED_SPY_SAP_BUILDING,
	SCHED_USE_DISPENSER,
	SCHED_PICKUP,
	SCHED_TF2_GET_AMMO,
	SCHED_TF2_FIND_FLAG,
	SCHED_LOOKAFTERSENTRY,
	SCHED_DEFEND,
	SCHED_ATTACKPOINT,
	SCHED_DEFENDPOINT,
	SCHED_TF2_PUSH_PAYLOADBOMB,
	SCHED_TF2_DEFEND_PAYLOADBOMB,
	SCHED_TF2_DEMO_PIPETRAP,
	SCHED_TF2_DEMO_PIPEENEMY,
	SCHED_BACKSTAB,
	SCHED_REMOVESAPPER,
	SCHED_GOTONEST,
	SCHED_MESSAROUND,
	SCHED_TF2_ENGI_MOVE_BUILDING,
	SCHED_FOLLOW_LAST_ENEMY,
	SCHED_SHOOT_LAST_ENEMY_POS,
	SCHED_GRAVGUN_PICKUP,
	SCHED_HELP_PLAYER,
	SCHED_BOMB,
	SCHED_TF_SPYCHECK,
	SCHED_FOLLOW,
	SCHED_DOD_DROPAMMO,
	SCHED_INVESTIGATE_NOISE,
	SCHED_CROUCH_AND_HIDE,
	SCHED_DEPLOY_MACHINE_GUN,
	SCHED_ATTACK_SENTRY_GUN,
	SCHED_RETURN_TO_INTEL,
	SCHED_INVESTIGATE_HIDE,
	SCHED_TAUNT,
	SCHED_MAX
	// SCHED_HIDE_FROM_ENEMY
} eBotSchedule;

class CBotSchedule
{
  public:
	CBotSchedule(CBotTask *pTask)
	{
		_init();

		addTask(pTask);
	}

	CBotSchedule();

	void _init();
	virtual void init()
	{
		return;
	} // nothing, used by sub classes

	void addTask(CBotTask *pTask);

	void execute(CBot *pBot);

	const char *getIDString();

	CBotTask *currentTask()
	{
		return m_Tasks.empty() ? nullptr : m_Tasks.front();
	}

	bool hasFailed()
	{
		return m_bFailed;
	}

	bool isComplete()
	{
		return m_Tasks.empty();
	}

	void freeMemory()
	{
		for (CBotTask *task : m_Tasks)
		{
			delete task;
		}
		m_Tasks.clear();
	}

	void removeTop();

	//////////////////////////

	void clearPass()
	{
		m_bitsPass = 0;
	}

	void passInt(int i);
	void passFloat(float f);
	void passVector(Vector v);
	void passEdict(edict_t *p);
	//////////////////////////

	bool hasPassInfo()
	{
		return (m_bitsPass != 0);
	}

	inline int passedInt()
	{
		return iPass;
	}
	inline float passedFloat()
	{
		return fPass;
	}
	inline Vector passedVector()
	{
		return vPass;
	}
	inline edict_t *passedEdict()
	{
		return pPass;
	}
	inline bool isID(eBotSchedule iId)
	{
		return m_iSchedId == iId;
	}

	inline bool hasPassInt()
	{
		return ((m_bitsPass & BITS_SCHED_PASS_INT) > 0);
	}
	inline bool hasPassFloat()
	{
		return ((m_bitsPass & BITS_SCHED_PASS_FLOAT) > 0);
	}
	inline bool hasPassVector()
	{
		return ((m_bitsPass & BITS_SCHED_PASS_VECTOR) > 0);
	}
	inline bool hasPassEdict()
	{
		return ((m_bitsPass & BITS_SCHED_PASS_EDICT) > 0);
	}

	inline void setID(eBotSchedule iId)
	{
		m_iSchedId = iId;
	}

  private:
	std::deque<CBotTask *> m_Tasks;
	bool m_bFailed;
	eBotSchedule m_iSchedId;

	// passed information to next task(s)
	int iPass;
	float fPass;
	Vector vPass;
	edict_t *pPass;

	int m_bitsPass;
};

class CBotSchedules
{
  public:
	bool hasSchedule(eBotSchedule iSchedule)
	{
		for (CBotSchedule *sched : m_Schedules)
		{
			if (sched->isID(iSchedule))
			{
				return true;
			}
		}
		return false;
	}

	bool isCurrentSchedule(eBotSchedule iSchedule)
	{
		if (m_Schedules.empty())
			return false;
		return m_Schedules.front()->isID(iSchedule);
	}

	// remove the first schedule in the queue matching this schedule identifier
	void removeSchedule(eBotSchedule iSchedule)
	{
		for (auto it = m_Schedules.begin(); it != m_Schedules.end();)
		{
			if ((*it)->isID(iSchedule))
			{
				m_Schedules.erase(it);
				return;
			}
			else
			{
				++it;
			}
		}
	}

	void execute(CBot *pBot)
	{
		if (isEmpty())
			return;

		CBotSchedule *pSched = m_Schedules.front();
		pSched->execute(pBot);

		if (pSched->isComplete() || pSched->hasFailed())
			removeTop();
	}

	void removeTop()
	{
		CBotSchedule *pSched = m_Schedules.front();
		m_Schedules.pop_front();

		// TODO: eradicate freeMemory from the codebase
		pSched->freeMemory();

		delete pSched;
	}

	void freeMemory()
	{
		for (CBotSchedule *sched : m_Schedules)
		{
			delete sched;
		}
		m_Schedules.clear();
	}

	void add(CBotSchedule *pSchedule)
	{
		// initialize
		pSchedule->init();
		// add
		m_Schedules.push_back(pSchedule);
	}

	void addFront(CBotSchedule *pSchedule)
	{
		pSchedule->init();
		m_Schedules.push_front(pSchedule);
	}

	inline bool isEmpty()
	{
		return m_Schedules.empty();
	}

	CBotTask *getCurrentTask()
	{
		if (!m_Schedules.empty())
		{
			CBotSchedule *sched = m_Schedules.front();

			if (sched != nullptr)
			{
				return sched->currentTask();
			}
		}

		return nullptr;
	}

	CBotSchedule *getCurrentSchedule()
	{
		if (isEmpty())
			return nullptr;

		return m_Schedules.front();
	}

  private:
	std::deque<CBotSchedule *> m_Schedules;
};