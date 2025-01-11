#pragma once

#include "botutil/base_task.h"

// automatically detonate pipes from a standing location, make sure
// the bot is not standing in a location visible to the enemy
// in vStand
class CBotTF2DemomanPipeEnemy : public CBotTask
{
  public:
	// CBotTF2DemomanPipeEnemy ( Vector vStand, Vector vBlastPoint, CBotWeapon *pPipeLauncher, Vector vEnemy, edict_t
	// *pEnemy );
	CBotTF2DemomanPipeEnemy(CBotWeapon *pPipeLauncher, Vector vEnemy, edict_t *pEnemy);
	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotTF2DemomanPipeEnemy");
	}

  private:
	Vector m_vStand;
	Vector m_vEnemy;
	MyEHandle m_pEnemy;
	Vector m_vAim;
	float m_fTime;
	float m_fHoldAttackTime;
	float m_fHeldAttackTime;
	CBotWeapon *m_pPipeLauncher;
};