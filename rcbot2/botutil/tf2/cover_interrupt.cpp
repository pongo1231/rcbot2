#include "cover_interrupt.h"

#include "bot_mods.h"

bool CBotTF2CoverInterrupt::isInterrupted(CBot *pBot, bool *bFailed, bool *bCompleted)
{
	if (CTeamFortress2Mod::TF2_IsPlayerInvuln(pBot->getEdict()))
	{
		*bFailed = true;
		return true;
	}

	return false;
}