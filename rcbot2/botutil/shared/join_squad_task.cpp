#include "join_squad_task.h"

#include "bot_squads.h"

void CBotJoinSquad::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!pBot->inSquad())
	{
		pBot->setSquad(CBotSquads::AddSquadMember(m_pPlayer, pBot->getEdict()));
		complete();
	}
	// CBotSquads::SquadJoin(pBot->getEdict(),m_pPlayer);
}