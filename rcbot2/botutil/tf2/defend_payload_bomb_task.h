#pragma once

#include "botutil/base_task.h"

class CBotTF2DefendPayloadBombTask : public CBotTask
{
  public:
	CBotTF2DefendPayloadBombTask(edict_t *pPayloadBomb);

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	MyEHandle m_pPayloadBomb;
	Vector m_vMoveTo;
	Vector m_vRandomOffset;
	float m_fDefendTime;
	float m_fTime;
	Vector m_vOrigin;
};