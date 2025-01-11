#include "auto_buy_task.h"

#include "bot_mtrand.h"

void CAutoBuy ::init()
{
	m_bTimeset = false;
}

void CAutoBuy ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	if (!m_bTimeset)
	{
		m_bTimeset = true;
		m_fTime    = engine->Time() + randomFloat(2.0, 4.0);
	}
	else if (m_fTime < engine->Time())
	{
		engine->SetFakeClientConVarValue(pBot->getEdict(), "cl_autobuy",
		                                 "m4a1 ak47 famas galil p90 mp5 primammo secammo defuser vesthelm vest");
		// helpers->ClientCommand(pBot->getEdict(),"setinfo cl_autobuy \"m4a1 ak47 famas galil p90 mp5 primammo secammo
		// defuser vesthelm vest\"\n");
		helpers->ClientCommand(pBot->getEdict(), "autobuy\n");
		complete();
	}
}