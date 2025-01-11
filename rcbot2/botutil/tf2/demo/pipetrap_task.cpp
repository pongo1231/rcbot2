#include "pipetrap_task.h"

#include "bot_mods.h"
#include "bot_weapons.h"

CBotTF2DemomanPipeTrap ::CBotTF2DemomanPipeTrap(eDemoTrapType type, Vector vStand, Vector vLoc, Vector vSpread,
                                                bool bAutoDetonate, int wptarea)
{
	m_vPoint        = vLoc;
	m_vLocation     = vLoc;
	m_vSpread       = vSpread;
	m_iState        = 0;
	m_iStickies     = 6;
	m_iTrapType     = type;
	m_vStand        = vStand;
	m_fTime         = 0.f;
	m_bAutoDetonate = bAutoDetonate;
	m_iWptArea      = wptarea;
}

void CBotTF2DemomanPipeTrap ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	bool bFail       = false;
	CBotTF2 *pTF2Bot = static_cast<CBotTF2 *>(pBot);
	if (pBot->getWeapons()->getWeapon(CWeapons::getWeapon(TF2_WEAPON_PIPEBOMBS)) == nullptr)
	{
		// don't have the weapon
		fail();
		return;
	}
	pBot->wantToChangeWeapon(false);

	if (pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		if (CTeamFortress2Mod::hasRoundStarted())
		{
			pBot->secondaryAttack();
			fail();
		}
	}

	if (pTF2Bot->deployStickies(m_iTrapType, m_vStand, m_vLocation, m_vSpread, &m_vPoint, &m_iState, &m_iStickies,
	                            &bFail, &m_fTime, m_iWptArea))
	{
		complete();

		if (m_bAutoDetonate)
			pBot->secondaryAttack();
	}

	if (bFail)
		fail();
}