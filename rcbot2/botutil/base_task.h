#pragma once

#include "bot.h"
#include "bot_const.h"

class CBotSchedule;

class IBotTaskInterrupt
{
  public:
	virtual bool isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted) = 0;
};

class CBotTask
{
  public:
	CBotTask();
	~CBotTask()
	{
		if (m_pInterruptFunc != nullptr)
		{
			delete m_pInterruptFunc;
			m_pInterruptFunc = nullptr;
		}
	}
	void _init();
	virtual void init();
	virtual void execute(CBot *pBot, CBotSchedule *pSchedule);

	// void setTimeout ();

	bool hasFailed();
	bool isComplete();
	// void setVector ( Vector vOrigin );
	// void setFloat ( float fFloat );
	bool timedOut();
	// void setEdict ( edict_t *pEdict );
	void setFailInterrupt(int iInterruptHave, int iInterruptDontHave = 0);
	void setCompleteInterrupt(int iInterruptHave, int iInterruptDontHave = 0);
	void setInterruptFunction(IBotTaskInterrupt *func)
	{
		m_pInterruptFunc = func;
	}
	virtual eTaskState isInterrupted(CBot *pBot);
	void fail();
	void complete();
	inline bool hasFlag(int iFlag)
	{
		return (m_iFlags & iFlag) == iFlag;
	}
	inline void setFlag(int iFlag)
	{
		m_iFlags |= iFlag;
	}
	void clearFailInterrupts()
	{
		m_iFailInterruptConditionsHave = m_iFailInterruptConditionsDontHave = 0;
	}
	virtual void debugString(char *string)
	{
		string[0] = 0;
		return;
	}

	// bool isID ( eTaskID eTaskId ) { };

  protected:
	IBotTaskInterrupt *m_pInterruptFunc;
	// void setID();
	//  flags
	int m_iFlags;
	// conditions that may happen to fail task
	int m_iFailInterruptConditionsHave;
	int m_iCompleteInterruptConditionsHave;
	int m_iFailInterruptConditionsDontHave;
	int m_iCompleteInterruptConditionsDontHave;
	// current state
	eTaskState m_iState;
	// time out
	float m_fTimeOut;
	// vars
	// edict_t *m_pEdict;
	// float m_fFloat;
	// int m_iInt;
	// Vector m_vVector;
};
