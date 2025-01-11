#include "drop_ammo_task.h"

#include "bot_globals.h"

void CDODDropAmmoTask ::debugString(char *string)
{
	sprintf(string, "CDODDropAmmoTask");
}

void CDODDropAmmoTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	Vector vOrigin = CBotGlobals::entityOrigin(m_pPlayer.get());

	if (m_pPlayer.get() == nullptr)
	{
		fail();
		return;
	}

	if (!CBotGlobals::entityIsAlive(m_pPlayer))
	{
		fail();
		return;
	}

	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setLookVector(vOrigin);
	pBot->setMoveTo(vOrigin);

	if (pBot->isFacing(vOrigin))
	{
		if (!pBot->isVisible(m_pPlayer))
		{
			fail();
			return;
		}
	}

	if ((pBot->distanceFrom(m_pPlayer) < 200.0f) && pBot->isFacing(vOrigin))
	{
		CDODBot *pDODBot = ((CDODBot *)pBot);
		pDODBot->dropAmmo();
		complete();
		return;
	}
}