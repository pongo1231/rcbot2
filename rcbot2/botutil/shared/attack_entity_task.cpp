#include "attack_entity_task.h"

#include "bot_weapons.h"

CAttackEntityTask ::CAttackEntityTask(edict_t *pEdict)
{
	m_pEdict = pEdict;
}

void CAttackEntityTask ::debugString(char *string)
{
	int id = -1;

	if (m_pEdict)
		id = ENTINDEX(m_pEdict);

	sprintf(string, "CAttackEntityTask (%d)", id);
}

void CAttackEntityTask ::init()
{
	// setFailInterrupt ( CONDITION_ENEMY_OBSCURED );
	// setCompleteInterrupt ( CONDITION_ENEMY_DEAD );
}

void CAttackEntityTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
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
		fail();
		return;
	}

	if (pBot->hasSomeConditions(CONDITION_ENEMY_DEAD))
	{
		fail();
		return;
	}

	pWeapon = pBot->getBestWeapon(m_pEdict);

	if ((pWeapon != nullptr) && (pWeapon != pBot->getCurrentWeapon()) && pWeapon->getWeaponIndex())
	{
		pBot->selectWeapon(pWeapon->getWeaponIndex());
	}

	pBot->setEnemy(m_pEdict);

	pBot->setLookAtTask(LOOK_ENEMY);

	if (!pBot->handleAttack(pWeapon, m_pEdict))
		fail();
}