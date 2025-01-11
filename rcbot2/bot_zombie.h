#pragma once

#include "bot.h"
#include "edict.h"

class CBotZombie : public CBot
{
	bool isEnemy(edict_t *pEdict, bool bCheckWeapons = true);

	void modThink(void);

	void getTasks(unsigned int iIgnore);
};