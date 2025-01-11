#pragma once

#include "bot_fortress.h"
#include "botutil/base_sched.h"

class CBotEngiMoveBuilding : public CBotSchedule
{
  public:
	CBotEngiMoveBuilding(edict_t *pBotEdict, edict_t *pBuilding, eEngiBuild iObject, Vector vNewLocation,
	                     bool bCarrying);

	void init();
};