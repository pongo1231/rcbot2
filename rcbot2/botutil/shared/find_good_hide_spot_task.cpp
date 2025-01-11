#include "find_good_hide_spot_task.h"

#include "bot_globals.h"
#include "bot_navigator.h"
#include "botutil/base_sched.h"

CFindGoodHideSpot ::CFindGoodHideSpot(edict_t *pEntity)
{
	m_vHideFrom = CBotGlobals::entityOrigin(pEntity);
}

CFindGoodHideSpot ::CFindGoodHideSpot(Vector vec)
{
	m_vHideFrom = vec;
}

void CFindGoodHideSpot ::init()
{
	// not required, should have been constructed properly
	// m_vHideFrom = Vector(0,0,0);
}

void CFindGoodHideSpot ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	Vector vFound;

	if (!pBot->getNavigator()->getHideSpotPosition(m_vHideFrom, &vFound))
		fail();
	else
	{
		pSchedule->passVector(vFound);
		complete();
	}
}