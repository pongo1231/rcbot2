#pragma once

#include "botutil/base_task.h"

class CBotTF2PushPayloadBombTask : public CBotTask
{
  public:
	CBotTF2PushPayloadBombTask(edict_t *pPayloadBomb);

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	MyEHandle m_pPayloadBomb;
	Vector m_vMoveTo;
	Vector m_vRandomOffset;
	float m_fPushTime;
	float m_fTime;
	Vector m_vOrigin;
};