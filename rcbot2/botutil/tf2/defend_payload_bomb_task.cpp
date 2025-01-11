#include "defend_payload_bomb_task.h"

#include "bot_globals.h"

CBotTF2DefendPayloadBombTask ::CBotTF2DefendPayloadBombTask(edict_t *pPayloadBomb)
{
	m_pPayloadBomb  = pPayloadBomb;
	m_fDefendTime   = 0;
	m_fTime         = 0;
	m_vRandomOffset = Vector(0, 0, 0);
}

void CBotTF2DefendPayloadBombTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (m_fDefendTime == 0)
	{
		m_fDefendTime   = engine->Time() + randomFloat(10.0f, 30.0f);
		m_vRandomOffset = Vector(randomFloat(-150.0f, 150.0f), randomFloat(-150.0f, 150.0f), 0);
	}
	else if (m_fDefendTime < engine->Time())
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
		m_vMoveTo = m_vOrigin + m_vRandomOffset;

		if (pBot->distanceFrom(m_vMoveTo) > 200)
			pBot->setMoveTo(m_vMoveTo);
		else
			pBot->stopMoving();

		pBot->setLookAtTask(LOOK_EDICT);
		pBot->lookAtEdict(m_pPayloadBomb);
	}
}

void CBotTF2DefendPayloadBombTask ::debugString(char *string)
{
	sprintf(string, "CBotTF2DefendPayloadBombTask (%0.1f,%0.1f,%0.1f)", m_vOrigin.x, m_vOrigin.y, m_vOrigin.z);
}