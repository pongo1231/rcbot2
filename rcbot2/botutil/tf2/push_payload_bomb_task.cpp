#include "push_payload_bomb_task.h"

#include "bot_globals.h"

CBotTF2PushPayloadBombTask ::CBotTF2PushPayloadBombTask(edict_t *pPayloadBomb)
{
	m_pPayloadBomb  = pPayloadBomb;
	m_fPushTime     = 0;
	m_fTime         = 0;
	m_vRandomOffset = Vector(0, 0, 0);
}

void CBotTF2PushPayloadBombTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToInvestigateSound(false);

	if (m_fPushTime == 0)
	{
		m_fPushTime     = engine->Time() + randomFloat(10.0, 30.0);
		m_vRandomOffset = Vector(randomFloat(-50, 50), randomFloat(-50, 50), 0);
	}
	else if (m_fPushTime < engine->Time())
	{
		complete();
	}
	else if (m_pPayloadBomb.get() == nullptr)
	{
		complete();
		return;
	}
	else
	{

		m_vOrigin = CBotGlobals::entityOrigin(m_pPayloadBomb);
		// m_vMoveTo = m_vOrigin + Vector(randomFloat(-10,10),randomFloat(-10,10),0);
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if (pBot->distanceFrom(m_vMoveTo) < 100)
		{
			if ((((CBotTF2 *)pBot)->getClass() == TF_CLASS_SPY) && (((CBotTF2 *)pBot)->isDisguised()))
				pBot->primaryAttack(); // remove disguise to capture

			((CBotFortress *)pBot)->wantToDisguise(false);
		}
		else
			pBot->setMoveTo(m_vMoveTo);

		pBot->setLookAtTask(LOOK_AROUND);
	}
}

void CBotTF2PushPayloadBombTask ::debugString(char *string)
{
	sprintf(string, "CBotTF2PushPayloadBombTask (%0.1f,%0.1f,%0.1f)", m_vOrigin.x, m_vOrigin.y, m_vOrigin.z);
}