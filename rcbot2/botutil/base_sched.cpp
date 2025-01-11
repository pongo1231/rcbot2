/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "base_sched.h"

#include "bot.h"
#include "bot_client.h"
#include "bot_weapons.h"
#include "botutil/base_task.h"

////////////////////////////////////
// these must match the SCHED IDs
const char *szSchedules[SCHED_MAX + 1] = { "SCHED_NONE",
	                                       "SCHED_ATTACK",
	                                       "SCHED_RUN_FOR_COVER",
	                                       "SCHED_GOTO_ORIGIN",
	                                       "SCHED_GOOD_HIDE_SPOT",
	                                       "SCHED_TF2_GET_FLAG",
	                                       "SCHED_TF2_GET_HEALTH",
	                                       "SCHED_TF_BUILD",
	                                       "SCHED_HEAL",
	                                       "SCHED_GET_METAL",
	                                       "SCHED_SNIPE",
	                                       "SCHED_UPGRADE",
	                                       "SCHED_USE_TELE",
	                                       "SCHED_SPY_SAP_BUILDING",
	                                       "SCHED_USE_DISPENSER",
	                                       "SCHED_PICKUP",
	                                       "SCHED_TF2_GET_AMMO",
	                                       "SCHED_TF2_FIND_FLAG",
	                                       "SCHED_LOOKAFTERSENTRY",
	                                       "SCHED_DEFEND",
	                                       "SCHED_ATTACKPOINT",
	                                       "SCHED_DEFENDPOINT",
	                                       "SCHED_TF2_PUSH_PAYLOADBOMB",
	                                       "SCHED_TF2_DEFEND_PAYLOADBOMB",
	                                       "SCHED_TF2_DEMO_PIPETRAP",
	                                       "SCHED_TF2_DEMO_PIPESENTRY",
	                                       "SCHED_BACKSTAB",
	                                       "SCHED_REMOVESAPPER",
	                                       "SCHED_GOTONEST",
	                                       "SCHED_MESSAROUND",
	                                       "SCHED_TF2_ENGI_MOVE_BUILDING",
	                                       "SCHED_FOLLOW_LAST_ENEMY",
	                                       "SCHED_SHOOT_LAST_ENEMY_POS",
	                                       "SCHED_GRAVGUN_PICKUP",
	                                       "SCHED_HELP_PLAYER",
	                                       "SCHED_BOMB",
	                                       "SCHED_TF_SPYCHECK",
	                                       "SCHED_FOLLOW",
	                                       "SCHED_DOD_DROPAMMO",
	                                       "SCHED_INVESTIGATE_NOISE",
	                                       "SCHED_CROUCH_AND_HIDE",
	                                       "SCHED_DEPLOY_MACHINE_GUN",
	                                       "SCHED_ATTACK_SENTRY_GUN",
	                                       "SCHED_RETURN_TO_INTEL",
	                                       "SCHED_INVESTIGATE_HIDE",
	                                       "SCHED_TAUNT",
	                                       "SCHED_MAX" };

void CBotSchedule ::execute(CBot *pBot)
{
	// current task
	static CBotTask *pTask;
	static eTaskState iState;

	if (m_Tasks.empty())
	{
		m_bFailed = true;
		return;
	}

	// why would task ever be null??
	pTask = m_Tasks.front();

	if (pTask == nullptr)
	{
		m_bFailed = true;
		return;
	}

	iState = pTask->isInterrupted(pBot);

	if (iState == STATE_FAIL)
		pTask->fail();
	else if (iState == STATE_COMPLETE)
		pTask->complete();
	else // still running
	{
		// timed out ??
		if (pTask->timedOut())
			pTask->fail(); // fail
		else
		{
			if (CClients::clientsDebugging(BOT_DEBUG_TASK))
			{
				char dbg[512];

				pTask->debugString(dbg);

				CClients::clientDebugMsg(BOT_DEBUG_TASK, dbg, pBot);
			}

			pTask->execute(pBot, this); // run
		}
	}

	if (pTask->hasFailed())
	{
		m_bFailed = true;
	}
	else if (pTask->isComplete())
	{
		removeTop();
	}
}

void CBotSchedule ::addTask(CBotTask *pTask)
{
	// initialize
	pTask->init();
	// add
	m_Tasks.push_back(pTask);
}

void CBotSchedule ::removeTop()
{
	CBotTask *pTask = m_Tasks.front();
	m_Tasks.pop_front();
	delete pTask;
}

const char *CBotSchedule ::getIDString()
{
	return szSchedules[m_iSchedId];
}

/////////////////////

CBotSchedule ::CBotSchedule()
{
	_init();
}

void CBotSchedule ::_init()
{
	m_bFailed  = false;
	m_bitsPass = 0;
	m_iSchedId = SCHED_NONE;

	// pass information
	iPass      = 0;
	fPass      = 0;
	vPass      = Vector(0, 0, 0);
	pPass      = 0;

	init();
}

void CBotSchedule ::passInt(int i)
{
	iPass = i;
	m_bitsPass |= BITS_SCHED_PASS_INT;
}
void CBotSchedule ::passFloat(float f)
{
	fPass = f;
	m_bitsPass |= BITS_SCHED_PASS_FLOAT;
}
void CBotSchedule ::passVector(Vector v)
{
	vPass = v;
	m_bitsPass |= BITS_SCHED_PASS_VECTOR;
}
void CBotSchedule ::passEdict(edict_t *p)
{
	pPass = p;
	m_bitsPass |= BITS_SCHED_PASS_EDICT;
}
////////////////////