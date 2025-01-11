#include "place_building_task.h"

#include "bot_globals.h"
#include "bot_mods.h"

CBotTaskEngiPlaceBuilding ::CBotTaskEngiPlaceBuilding(eEngiBuild iObject, Vector vOrigin)
{
	m_vOrigin = vOrigin;
	m_fTime   = 0.0f;
	m_iState  = 1; // BEGIN HERE , otherwise bot will try to destroy the building
	m_iObject = iObject;
	m_iTries  = 0;
}

// unused
void CBotTaskEngiPlaceBuilding ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToInvestigateSound(false);

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + 6.0f;
		pBot->resetLookAroundTime();

		if (CTeamFortress2Mod::buildingNearby(pBot->getTeam(), m_vOrigin))
			m_vOrigin = m_vOrigin + Vector(randomFloat(-200.0f, 200.0f), randomFloat(-200.0f, 200.0f), 0);
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	((CBotTF2 *)pBot)->updateCarrying();

	if (!(((CBotTF2 *)pBot)->isCarrying()))
		complete();
	else if (m_fTime < engine->Time())
		fail();
	else if (pBot->distanceFrom(m_vOrigin) < 100)
	{
		if (CBotGlobals::yawAngleFromEdict(pBot->getEdict(), m_vOrigin) < 25)
		{
			int state = ((CBotTF2 *)pBot)->engiBuildObject(&m_iState, m_iObject, &m_fTime, &m_iTries);

			if (state == 1)
				complete();
			else if (state == 0)
				fail();

			// pBot->primaryAttack();
		}
	}
	else
		pBot->setMoveTo(m_vOrigin);

	if (pBot->hasEnemy())
	{
		if (randomInt(0, 1))
			pBot->primaryAttack();
	}
}

void CBotTaskEngiPlaceBuilding ::debugString(char *string)
{
	sprintf(string, "CBotTaskEngiPlaceBuilding");
}