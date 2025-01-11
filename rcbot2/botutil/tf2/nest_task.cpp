#include "nest_task.h"

#include "bot_fortress.h"
#include "bot_mtrand.h"

void CBotNest ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotTF2 *pBotTF2 = (CBotTF2 *)pBot;

	if ((pBotTF2->getClass() == TF_CLASS_MEDIC) && pBotTF2->someoneCalledMedic())
		fail(); // Follow player

	if (!pBotTF2->wantToNest())
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}
	else if (pBot->hasSomeConditions(CONDITION_PUSH))
	{
		complete();
		pBot->removeCondition(CONDITION_PUSH);
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
		return;
	}

	if (m_fTime == 0)
	{
		m_fTime = engine->Time() + randomFloat(6.0f, 12.0f);

		if (randomInt(0, 1))
			pBotTF2->addVoiceCommand(TF_VC_HELP);
	}
	else if (m_fTime < engine->Time())
	{
		complete();
		pBotTF2->addVoiceCommand(TF_VC_GOGOGO);
	}

	// wait around
	// wait for more friendlies
	// heal up
	//

	pBot->setLookAtTask(LOOK_AROUND);

	pBot->stopMoving();
}

CBotNest::CBotNest()
{
	m_fTime  = 0.0f;
	m_pEnemy = nullptr;
}