#include "gravgun_pickup_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_hldm_bot.h"
#include "bot_weapons.h"

void CBotGravGunPickup ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static Vector vOrigin;
	static Vector vBotOrigin;

	if (m_fTime == 0.0f)
	{
		m_fSecAttTime = 0;
		m_fTime       = engine->Time() + randomFloat(2.0f, 4.0f);
	}

	if (m_fTime < engine->Time())
	{
		CHLDMBot *HL2DMBot = ((CHLDMBot *)pBot);

		if (HL2DMBot->getFailedObject()
		    && (HL2DMBot->distanceFrom(HL2DMBot->getFailedObject()) <= (pBot->distanceFrom(m_Prop) + 48)))
			pBot->primaryAttack();

		HL2DMBot->setFailedObject(m_Prop);

		fail();
		return;
	}

	if (!CBotGlobals::entityIsValid(m_Prop) || !pBot->isVisible(m_Prop))
	{
		((CHLDMBot *)pBot)->setFailedObject(m_Prop);
		fail();
		return;
	}

	pBot->wantToChangeWeapon(false);

	vBotOrigin          = pBot->getOrigin();
	vOrigin             = CBotGlobals::entityOrigin(m_Prop);

	CBotWeapon *pWeapon = pBot->getCurrentWeapon();

	if (!pWeapon || (pWeapon->getID() != HL2DM_WEAPON_PHYSCANNON))
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(HL2DM_WEAPON_PHYSCANNON)))
		{
			fail();
			return;
		}
	}
	else if (pBot->distanceFrom(vOrigin) > 100)
		pBot->setMoveTo(vOrigin);
	else if (((vOrigin - vBotOrigin).Length2D() < 16) && (vOrigin.z < vBotOrigin.z))
		pBot->setMoveTo(vBotOrigin + (vBotOrigin - vOrigin) * 100);
	else
		pBot->stopMoving();

	if (pWeapon)
		m_Weapon = INDEXENT(pWeapon->getWeaponIndex());
	else
	{
		fail();
		return;
	}

	pBot->setMoveLookPriority(MOVELOOK_OVERRIDE);
	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);
	pBot->setMoveLookPriority(MOVELOOK_TASK);

	if (pBot->isFacing(vOrigin))
	{
		edict_t *pPhys = CClassInterface::gravityGunObject(m_Weapon);

		if (pPhys == m_Prop.get())
			complete();
		else if (pPhys || CClassInterface::gravityGunOpen(m_Weapon))
		{
			if (m_fSecAttTime < engine->Time())
			{
				pBot->secondaryAttack();
				m_fSecAttTime = engine->Time() + randomFloat(0.25, 0.75);
			}
		}
	}
}