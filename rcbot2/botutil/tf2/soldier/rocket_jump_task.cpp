#include "rocket_jump_task.h"

#include "bot_fortress.h"
#include "bot_mtrand.h"
#include "bot_weapons.h"

CBotTFRocketJump ::CBotTFRocketJump()
{
	m_fTime     = 0.0f;
	m_fJumpTime = 0.0f;
	m_iState    = 0;
}

void CBotTFRocketJump ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pBotWeapon;
	CWeapon *pWeapon;

	pBot->wantToListen(false);

	pBotWeapon = pBot->getCurrentWeapon();

	if (!pBotWeapon)
	{
		fail();
		return;
	}

	pWeapon = pBotWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	if (!pBot->isTF() || (((CBotFortress *)pBot)->getClass() != TF_CLASS_SOLDIER) || (pBot->getHealthPercent() < 0.3))
	{
		fail();
	}
	else if (pWeapon->getID() != TF2_WEAPON_ROCKETLAUNCHER)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_ROCKETLAUNCHER)))
		{
			fail();
		}
	}
	else
	{
		CBotTF2 *tf2Bot = ((CBotTF2 *)pBot);

		if (!m_fTime)
		{
			m_fTime = engine->Time() + randomFloat(4.0f, 5.0f);
		}

		if (tf2Bot->rocketJump(&m_iState, &m_fJumpTime) == BOT_FUNC_COMPLETE)
			complete();
		else if (m_fTime < engine->Time())
		{
			fail();
		}
	}
}

void CBotTFRocketJump ::debugString(char *string)
{
	sprintf(string, "CBotTFRocketJump");
}