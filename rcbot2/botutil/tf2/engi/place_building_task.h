#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotTaskEngiPlaceBuilding : public CBotTask
{
  public:
	CBotTaskEngiPlaceBuilding(eEngiBuild iObject, Vector vOrigin); // going to use this
	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	float m_fTime;
	eEngiBuild m_iObject;
	int m_iState;
	int m_iTries;
};