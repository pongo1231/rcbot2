#pragma once

#include "bot_fortress.h"
#include "botutil/base_task.h"

class CBotTF2SpySap : public CBotTask
{
  public:
	CBotTF2SpySap(edict_t *pBuilding, eEngiBuild id);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pBuilding;
	float m_fTime;
	float m_fEvadeTime;
	float m_fStrafeTime;
	float m_fStateChangeTime;
	eEngiBuild m_id;
	int m_iState;
	Vector m_vBuildingOrigin;
	bool m_bEvadeRight;

	bool buildingIsSapped(edict_t *pBuilding);
};

enum
{
	SAP_APPROACH = 0,
	SAP_EVADE
};
