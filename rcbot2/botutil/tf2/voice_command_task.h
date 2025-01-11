#pragma once

#include "botutil/base_task.h"

class CTaskVoiceCommand : public CBotTask
{
  public:
	CTaskVoiceCommand(int iVoiceCmd)
	{
		m_iVoiceCmd = iVoiceCmd;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule)
	{
		pBot->addVoiceCommand(m_iVoiceCmd);

		complete();
	}

  private:
	int m_iVoiceCmd;
};