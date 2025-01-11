#pragma once

#include "botutil/base_task.h"

class CBotHL2DMUseButton : public CBotTask
{
  public:
	CBotHL2DMUseButton(edict_t *pButton)
	{
		m_pButton = pButton;
		m_fTime   = 0.0f;
	}

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "Use Button");
	}

  private:
	MyEHandle m_pButton;
	float m_fTime;
};