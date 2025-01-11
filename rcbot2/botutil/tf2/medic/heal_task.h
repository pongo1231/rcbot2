#pragma once

#include "botutil/base_task.h"

class CBotTF2MedicHeal : public CBotTask
{
  public:
	CBotTF2MedicHeal();

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2MedicHeal");
	}

  private:
	MyEHandle m_pHeal;
	Vector m_vJump;
	bool m_bHealerJumped;
};