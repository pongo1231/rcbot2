#include "bomb_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_mods.h"

CBotDODBomb ::CBotDODBomb(int iBombType, int iBombID, edict_t *pBomb, Vector vPosition, int iPrevOwner)
{
	m_iType   = iBombType;
	m_iBombID = iBombID;
	m_fTime   = 0;

	if (m_iBombID == -1)
		m_iBombID = CDODMod::m_Flags.getBombID(pBomb);

	m_pBombTarget = pBomb;
	m_vOrigin     = vPosition;
	m_iPrevTeam   = iPrevOwner;
}

void CBotDODBomb ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	bool bWorking = false;

	pBot->wantToShoot(false);

	if (m_fTime == 0)
	{
		m_fTime = engine->Time() + randomFloat(8.0f, 12.0f);
		((CDODBot *)pBot)->setNearestBomb(m_pBombTarget);

		if ((m_iType == DOD_BOMB_PLANT) || (m_iType == DOD_BOMB_PATH_PLANT))
		{
			if (CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(), pBot->getTeam(), m_iBombID))
				fail();
		}
		else
		{
			if (CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(), pBot->getTeam(), m_iBombID))
				fail();
		}
	}
	else if (m_fTime < engine->Time())
	{
		fail();
	}

	if (m_iType == DOD_BOMB_PLANT)
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if (CDODMod::m_Flags.isBombPlanted(m_iBombID))
		{
			complete();
		}
		else if (CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(), pBot->getTeam(), m_iBombID))
			complete(); // team mate doing my job

		// else if ( !CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict()) )// it is still planted
		//	complete(); // bomb is being defused by someone else - give up
	}
	else if (m_iType == DOD_BOMB_DEFUSE)
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if (!CDODMod::m_Flags.isBombPlanted(m_iBombID))
			complete();
		else if (CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(), pBot->getTeam(), m_iBombID))
			complete(); // team mate doing my job

		// else if ( CDODMod::m_Flags.isBombBeingDefused(m_iBombID) &&
		// !CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict()) )// it is still planted 	complete(); // bomb
		// is
		// being defused by someone else - give up
	}
	else if (m_iType == DOD_BOMB_PATH_PLANT) // a bomb that's in the way
	{
		bWorking = CClassInterface::isPlayerPlantingBomb_DOD(pBot->getEdict());

		if (CClassInterface::getDODBombState(m_pBombTarget) != DOD_BOMB_STATE_AVAILABLE)
		{
			// CDODBot *pDODBot = (CDODBot*)pBot;

			// pDODBot->removeBomb();

			complete();
		}
		else if (CDODMod::m_Flags.isTeamMatePlanting(pBot->getEdict(), pBot->getTeam(),
		                                             CBotGlobals::entityOrigin(m_pBombTarget)))
			complete(); // team mate doing my job
	}
	else if (m_iType == DOD_BOMB_PATH_DEFUSE) // a bomb that's in the way
	{
		bWorking = CClassInterface::isPlayerDefusingBomb_DOD(pBot->getEdict());

		if (CClassInterface::getDODBombState(m_pBombTarget) == DOD_BOMB_STATE_AVAILABLE)
			complete();
		else if (CDODMod::m_Flags.isTeamMateDefusing(pBot->getEdict(), pBot->getTeam(),
		                                             CBotGlobals::entityOrigin(m_pBombTarget)))
			complete(); // team mate doing my job
	}

	pBot->setLookVector(m_vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	pBot->use();

	if (!bWorking)
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict()) / 4);
	}
	else
	{
		pBot->stopMoving();
	}
}

void CBotDODBomb ::debugString(char *string)
{
	sprintf(string, "CBotDODBomb\nm_iType = %d\nm_iBombID = %d\nm_fTime = %0.2f\nm_iPrevTeam = %d", m_iType, m_iBombID,
	        m_fTime, m_iPrevTeam);
}