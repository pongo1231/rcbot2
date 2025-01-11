#include "taunt_sched.h"

#include "bot_globals.h"
#include "botutil/shared/find_path_task.h"
#include "botutil/shared/move_to_task.h"
#include "botutil/tf2/taunt_task.h"

CBotTauntSchedule ::CBotTauntSchedule(edict_t *pPlayer, float fYaw)
{
	QAngle angles = QAngle(0, fYaw, 0);
	Vector forward;
	Vector vOrigin;
	Vector vGoto;
	const float fTauntDist = 40.0f;

	m_pPlayer              = pPlayer;
	m_fYaw                 = 180 - fYaw;

	AngleVectors(angles, &forward);

	// forward = forward/forward.Length();
	vOrigin = CBotGlobals::entityOrigin(pPlayer);

	vGoto   = vOrigin + (forward * fTauntDist);

	CBotGlobals::fixFloatAngle(&m_fYaw);

	addTask(new CFindPathTask(vGoto));
	addTask(new CMoveToTask(vGoto));
	addTask(new CTF2_TauntTask(pPlayer, vGoto, 5.f));
}

void CBotTauntSchedule ::init()
{
	setID(SCHED_TAUNT);
}