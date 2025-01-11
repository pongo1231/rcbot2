#include "use_lunchbox_drink_task.h"

#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_weapons.h"

CBotUseLunchBoxDrink::CBotUseLunchBoxDrink()
{
	m_fTime = 0.0f;
}

void CBotUseLunchBoxDrink::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotWeapon *pWeapon;

	pBot->wantToShoot(false);
	pBot->wantToChangeWeapon(false);

	if (m_fTime == 0)
		m_fTime = engine->Time() + 5.0f;
	else if (m_fTime < engine->Time())
		fail();

	pWeapon = pBot->getCurrentWeapon();

	if (CClassInterface::TF2_getEnergyDrinkMeter(pBot->getEdict()) < 100.0f)
	{
		fail();
		return;
	}

	if (!pWeapon || (pWeapon->getID() != TF2_WEAPON_LUNCHBOX_DRINK))
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_LUNCHBOX_DRINK)))
			fail();
	}
	else
	{
		if (randomInt(0, 1))
			pBot->primaryAttack();
	}
}