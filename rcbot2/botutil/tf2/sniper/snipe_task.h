#pragma once

#include "botutil/base_task.h"

class CBotTF2Snipe : public CBotTask
{
  public:
	CBotTF2Snipe(Vector vOrigin, int iWpt);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "CBotSnipe");
	}

  private:
	float m_fTime;        // time of task
	Vector m_vAim;        // base origin to aim at
	Vector m_vOrigin;     // origin of snipe waypoint
	Vector m_vEnemy;      // origin of last enemy
	int m_iArea;          // area of snipe waypoint
	int m_iHideWaypoint;  // waypoint id of place to hide
	int m_iSnipeWaypoint; // waypoint id of place to snipe
	Vector m_vHideOrigin; // origin of hiding place
	float m_fHideTime;    // if above engine time, hide
	int m_iPrevClip;      // used to check i actually fired a bullet or not
	float m_fEnemyTime;   // last time i saw an enemy
	float m_fAimTime;     // last time i got ready to aim - gives bots time to aim before shooting
	float m_fCheckTime;   // time to check if there is a wall in front of me while sniping
	float m_fOriginDistance;
};