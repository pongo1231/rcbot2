#pragma once

#include "botutil/base_task.h"

class CBotTFUseTeleporter : public CBotTask
{
  public:
	CBotTFUseTeleporter(edict_t *pTele); // going to use this

	void execute(CBot *pBot, CBotSchedule *pSchedule);
	virtual void debugString(char *string);

  private:
	MyEHandle m_pTele;
	float m_fTime;
	Vector m_vLastOrigin;
};