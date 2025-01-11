#include "use_buff_item_task.h"

#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_weapons.h"

CBotUseBuffItem ::CBotUseBuffItem()
{
	m_fTime = 0.0f;
}

void CBotUseBuffItem::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon;

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if (m_fTime == 0)
		m_fTime = engine->Time() + 5.0f;
	else if (m_fTime < engine->Time())
		fail();

	pWeapon = pBot->getCurrentWeapon();

	if (CClassInterface::getRageMeter(pBot->getEdict()) < 100.0f)
	{
		fail();
		return;
	}

	if (!pWeapon || (pWeapon->getID() != TF2_WEAPON_BUFF_ITEM))
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_BUFF_ITEM)))
			fail();
	}
	else
	{
		if (randomInt(0, 1))
			pBot->primaryAttack();
	}
}