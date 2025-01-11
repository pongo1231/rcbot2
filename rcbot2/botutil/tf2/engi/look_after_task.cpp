#include "look_after_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_weapons.h"

void CBotTF2EngiLookAfter ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotFortress *tfBot = (CBotFortress *)pBot;

	if (!m_fTime)
	{
		m_fTime      = engine->Time() + randomFloat(21.0f, 60.0f);
		m_fHitSentry = engine->Time() + randomFloat(1.0f, 3.0f);
	}
	else if (m_fTime < engine->Time())
	{
		tfBot->nextLookAfterSentryTime(engine->Time() + randomFloat(20.0f, 50.0f));
		complete();
	}
	else
	{
		CBotWeapon *pWeapon = pBot->getCurrentWeapon();
		edict_t *pSentry    = m_pSentry.get();

		pBot->wantToListen(false);

		pBot->setLookAtTask(LOOK_AROUND);

		if (!pWeapon)
		{
			fail();
			return;
		}
		else if (pWeapon->getID() != TF2_WEAPON_WRENCH)
		{
			if (!pBot->select_CWeapon(CWeapons::getWeapon(TF2_WEAPON_WRENCH)))
			{
				fail();
				return;
			}
		}

		if (pSentry != nullptr)
		{
			if (pBot->distanceFrom(pSentry) > 100)
				pBot->setMoveTo(CBotGlobals::entityOrigin(pSentry));
			else
			{
				pBot->stopMoving();

				if ((CClassInterface::getSentryEnemy(pSentry) != nullptr)
				    || (CClassInterface::getSentryHealth(pSentry)
				        < CClassInterface::getTF2GetBuildingMaxHealth(pSentry))
				    || (CClassInterface::getTF2SentryRockets(pSentry) < 20)
				    || (CClassInterface::getTF2SentryShells(pSentry) < 150))
				{
					pBot->primaryAttack();
				}

				pBot->duck(true); // crouch too
			}

			pBot->lookAtEdict(pSentry);
			pBot->setLookAtTask(LOOK_EDICT); // LOOK_EDICT fix engineers not looking at their sentry
		}
		else
			fail();
	}
}