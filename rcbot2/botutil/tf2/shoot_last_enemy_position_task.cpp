#include "shoot_last_enemy_position_task.h"

#include "bot_globals.h"
#include "bot_weapons.h"

CBotTF2ShootLastEnemyPosition ::CBotTF2ShootLastEnemyPosition(Vector vPosition, edict_t *pEnemy, Vector m_vVelocity)
{
	float len   = m_vVelocity.Length();

	m_vPosition = vPosition;

	if (len > 0)
		m_vPosition = m_vPosition - ((m_vVelocity / m_vVelocity.Length()) * 16);

	m_pEnemy = pEnemy;
	m_fTime  = 0;
}

void CBotTF2ShootLastEnemyPosition ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon   = pBot->getCurrentWeapon();
	CBotTF2 *pTF2Bot      = (CBotTF2 *)pBot;
	CWeapon *pChange      = nullptr;
	CBotWeapon *pChangeTo = nullptr;

	if (m_fTime == 0)
		m_fTime = engine->Time() + randomFloat(2.0f, 4.5f);

	if (m_fTime < engine->Time())
	{
		complete();
		return;
	}

	if (!CBotGlobals::entityIsValid(m_pEnemy) || !CBotGlobals::entityIsAlive(m_pEnemy))
	{
		complete();
		return;
	}

	if (pBot->getEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY))
	{
		fail();
		return;
	}

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);
	pBot->wantToListen(false);

	if (pTF2Bot->getClass() == TF_CLASS_SOLDIER)
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER);
	}
	else if (pTF2Bot->getClass() == TF_CLASS_DEMOMAN)
	{
		pChange = CWeapons::getWeapon(TF2_WEAPON_GRENADELAUNCHER);
	}

	if (!pChange)
	{
		fail();
		return;
	}

	pChangeTo = pBot->getWeapons()->getWeapon(pChange);

	if (pChangeTo->getAmmo(pBot) < 1)
	{
		complete();
		return;
	}

	if (pChangeTo != pWeapon)
	{
		pBot->selectBotWeapon(pChangeTo);
	}
	else
	{
		if (randomInt(0, 1))
			pBot->primaryAttack(false);
	}

	pBot->setLookVector(m_vPosition);
	pBot->setLookAtTask(LOOK_VECTOR);
}

void CBotTF2ShootLastEnemyPosition ::debugString(char *string)
{
	sprintf(string, "CBotTF2ShootLastEnemyPosition\nm_vPosition = (%0.4f,%0.4f,%0.4f)", m_vPosition.x, m_vPosition.y,
	        m_vPosition.z);
}