#include "use_charger_task.h"

#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_hldm_bot.h"

void CBotHL2DMUseCharger ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static Vector vOrigin;

	if (m_pCharger.get() == nullptr)
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pCharger);

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + randomFloat(4.0f, 6.0f);
	}

	if (m_fTime < engine->Time())
		complete();

	if (CClassInterface::getAnimCycle(m_pCharger) == 1.0f)
		complete();

	if ((m_iType == CHARGER_HEALTH) && (pBot->getHealthPercent() >= 0.99f))
		complete();
	else if ((m_iType == CHARGER_ARMOR) && (((CHLDMBot *)pBot)->getArmorPercent() >= 0.99f))
		complete();

	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	if (pBot->distanceFrom(m_pCharger) > 96)
	{
		pBot->setMoveTo(vOrigin);
	}
	else if (pBot->isFacing(vOrigin))
	{
		pBot->use();
	}
}