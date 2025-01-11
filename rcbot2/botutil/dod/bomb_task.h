#pragma once

#include "botutil/base_task.h"

class CBotDODBomb : public CBotTask
{
  public:
	CBotDODBomb(int iBombType, int iBombID, edict_t *m_pBombTarget, Vector vPosition, int iPrevOwner);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	Vector m_vOrigin;
	float m_fTime;
	int m_iBombID;
	int m_iPrevTeam; // prev owner
	edict_t *m_pBombTarget;
	int m_iType;
};