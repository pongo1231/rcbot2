#include "use_button_task.h"

#include "bot_globals.h"

void CBotHL2DMUseButton ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static Vector vOrigin;

	if (m_pButton.get() == nullptr)
	{
		fail();
		return;
	}

	vOrigin = CBotGlobals::entityOrigin(m_pButton);

	if (m_fTime == 0.0f)
	{
		m_fTime = engine->Time() + randomFloat(4.0f, 6.0f);
	}

	if (m_fTime < engine->Time())
		complete();

	// if ( CClassInterface::getAnimCycle(m_pCharger) == 1.0f )
	//	complete();

	pBot->setLookVector(vOrigin);
	pBot->setLookAtTask(LOOK_VECTOR);

	if (pBot->distanceFrom(m_pButton) > 96)
	{
		pBot->setMoveTo(vOrigin);
	}
	else if (pBot->isFacing(vOrigin))
	{
		pBot->use();
		complete();
	}
}