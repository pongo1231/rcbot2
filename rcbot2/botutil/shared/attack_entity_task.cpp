#include "attack_entity_task.h"

#include "bot_fortress.h"
#include "bot_mods.h"
#include "bot_weapons.h"

CAttackEntityTask::CAttackEntityTask(edict_t *pEdict)
{
	m_pEdict = pEdict;
}

void CAttackEntityTask::debugString(char *string)
{
	int id = -1;

	if (m_pEdict)
		id = ENTINDEX(m_pEdict);

	sprintf(string, "CAttackEntityTask (%d)", id);
}

void CAttackEntityTask::init()
{
	// setFailInterrupt ( CONDITION_ENEMY_OBSCURED );
	// setCompleteInterrupt ( CONDITION_ENEMY_DEAD );
}

void CAttackEntityTask::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon;

	if (m_pEdict.get() == nullptr)
	{
		fail();
		return;
	}

	if (!pBot->isEnemy(m_pEdict))
	{
		complete();
		return;
	}

	if (!pBot->isVisible(m_pEdict))
	{
		// Tanks are enormous -- don't fail on brief visibility loss during strafing
		if (!(CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(m_pEdict.get())))
		{
			fail();
			return;
		}
	}

	if (pBot->hasSomeConditions(CONDITION_ENEMY_DEAD))
	{
		fail();
		return;
	}

	// Tank: skip generic weapon selection -- let handleAttack() force class-optimal weapon
	bool bSkipWeaponSelect = false;
	if (CTeamFortress2Mod::isMapType(TF_MAP_MVM) && CTeamFortress2Mod::isTankBoss(m_pEdict.get()))
	{
		int iClass = ((CBotFortress *)pBot)->getClass();
		bSkipWeaponSelect = (iClass == TF_CLASS_PYRO || iClass == TF_CLASS_SOLDIER
		                     || iClass == TF_CLASS_DEMOMAN || iClass == TF_CLASS_SCOUT
		                     || iClass == TF_CLASS_HWGUY);
	}

	pWeapon = bSkipWeaponSelect ? pBot->getCurrentWeapon() : pBot->getBestWeapon(m_pEdict);

	if (!bSkipWeaponSelect && (pWeapon != nullptr) && (pWeapon != pBot->getCurrentWeapon()) && pWeapon->getWeaponIndex())
		pBot->selectWeapon(pWeapon->getWeaponIndex());

	pBot->setEnemy(m_pEdict);

	pBot->setLookAtTask(LOOK_ENEMY);

	if (!pBot->handleAttack(pWeapon, m_pEdict))
		fail();
}