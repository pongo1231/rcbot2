#pragma once

#include "botutil/base_task.h"

class CBotTF2ShootLastEnemyPosition : public CBotTask
{
  public:
	CBotTF2ShootLastEnemyPosition(Vector vPosition, edict_t *pEnemy, Vector velocity);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	virtual void debugString(char *string);

  private:
	MyEHandle m_pEnemy;
	Vector m_vPosition;
	float m_fTime;
};