#pragma once

#include "bot_waypoint_visibility.h"
#include "botutil/base_task.h"

class CBotTF2FindPipeWaypoint : public CBotTask
{
  public:
	CBotTF2FindPipeWaypoint(Vector vOrigin, Vector vTarget);

	void execute(CBot *pBot, CBotSchedule *pSchedule);

	void debugString(char *string)
	{
		sprintf(string, "Finding Pipe Waypoint");
	}

  private:
	int m_iters;
	unsigned short int m_i;
	unsigned short int m_j;
	Vector m_vOrigin;
	Vector m_vTarget;
	short int m_iTargetWaypoint;
	float m_fNearesti;
	float m_fNearestj;
	short int m_iNearesti;
	short int m_iNearestj;

	CWaypointVisibilityTable *m_pTable;
	CWaypoint *m_pTarget;
	WaypointList m_WaypointsI;
	WaypointList m_WaypointsJ;
};