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
#include "engine_wrappers.h"

#include "bot.h"
#include "bot_configfile.h"
#include "bot_fortress.h"
#include "bot_getprop.h"
#include "bot_mods.h"
#include "bot_utility.h"

#define UTIL(x) { x, #x }
const std::unordered_map<int, const char *> g_szUtils = {
	UTIL(BOT_UTIL_BUILDSENTRY),
	UTIL(BOT_UTIL_BUILDTELENT),
	UTIL(BOT_UTIL_BUILDTELEXT),
	UTIL(BOT_UTIL_UPGSENTRY),
	UTIL(BOT_UTIL_UPGDISP),
	UTIL(BOT_UTIL_UPGTELENT),
	UTIL(BOT_UTIL_UPGTELEXT),
	UTIL(BOT_UTIL_UPGTMSENTRY),
	UTIL(BOT_UTIL_UPGTMDISP),
	UTIL(BOT_UTIL_UPGTMTELENT),
	UTIL(BOT_UTIL_UPGTMTELEXT),
	UTIL(BOT_UTIL_GOTODISP),
	UTIL(BOT_UTIL_GOTORESUPPLY_FOR_HEALTH),
	UTIL(BOT_UTIL_GETAMMOKIT),
	UTIL(BOT_UTIL_GETAMMOTMDISP),
	UTIL(BOT_UTIL_GETAMMODISP),
	UTIL(BOT_UTIL_GETFLAG),
	UTIL(BOT_UTIL_GETHEALTHKIT),
	UTIL(BOT_UTIL_GETFLAG_LASTKNOWN),
	UTIL(BOT_UTIL_SNIPE),
	UTIL(BOT_UTIL_ROAM),
	UTIL(BOT_UTIL_CAPTURE_FLAG),
	UTIL(BOT_UTIL_GOTORESUPPLY_FOR_AMMO),
	UTIL(BOT_UTIL_FIND_NEAREST_HEALTH),
	UTIL(BOT_UTIL_FIND_NEAREST_AMMO),
	UTIL(BOT_UTIL_ATTACK_POINT),
	UTIL(BOT_UTIL_DEFEND_POINT),
	UTIL(BOT_UTIL_DEFEND_FLAG),
	UTIL(BOT_UTIL_ENGI_LOOK_AFTER_SENTRY),
	UTIL(BOT_UTIL_DEFEND_FLAG_LASTKNOWN),
	UTIL(BOT_UTIL_PUSH_PAYLOAD_BOMB),
	UTIL(BOT_UTIL_DEFEND_PAYLOAD_BOMB),
	UTIL(BOT_UTIL_MEDIC_HEAL),
	UTIL(BOT_UTIL_MEDIC_HEAL_LAST),
	UTIL(BOT_UTIL_MEDIC_FINDPLAYER),
	UTIL(BOT_UTIL_SAP_NEAREST_SENTRY),
	UTIL(BOT_UTIL_SAP_ENEMY_SENTRY),
	UTIL(BOT_UTIL_SAP_LASTENEMY_SENTRY),
	UTIL(BOT_UTIL_SAP_DISP),
	UTIL(BOT_UTIL_BACKSTAB),
	UTIL(BOT_UTIL_REMOVE_SENTRY_SAPPER),
	UTIL(BOT_UTIL_REMOVE_DISP_SAPPER),
	UTIL(BOT_UTIL_REMOVE_TMSENTRY_SAPPER),
	UTIL(BOT_UTIL_REMOVE_TMDISP_SAPPER),
	UTIL(BOT_UTIL_DEMO_STICKYTRAP_LASTENEMY),
	UTIL(BOT_UTIL_DEMO_STICKYTRAP_POINT),
	UTIL(BOT_UTIL_DEMO_STICKYTRAP_FLAG),
	UTIL(BOT_UTIL_DEMO_STICKYTRAP_FLAG_LASTKNOWN),
	UTIL(BOT_UTIL_DEMO_STICKYTRAP_PL),
	UTIL(BOT_UTIL_REMOVE_TMTELE_SAPPER),
	UTIL(BOT_UTIL_SAP_NEAREST_TELE),
	UTIL(BOT_UTIL_SAP_ENEMY_TELE),
	UTIL(BOT_UTIL_SAP_LASTENEMY_TELE),
	UTIL(BOT_UTIL_GOTO_NEST),
	UTIL(BOT_UTIL_MESSAROUND),
	UTIL(BOT_UTIL_ENGI_MOVE_SENTRY),
	UTIL(BOT_UTIL_ENGI_MOVE_DISP),
	UTIL(BOT_UTIL_ENGI_MOVE_ENTRANCE),
	UTIL(BOT_UTIL_ENGI_MOVE_EXIT),
	UTIL(BOT_UTIL_ENGI_DESTROY_SENTRY),
	UTIL(BOT_UTIL_ENGI_DESTROY_DISP),
	UTIL(BOT_UTIL_ENGI_DESTROY_ENTRANCE),
	UTIL(BOT_UTIL_ENGI_DESTROY_EXIT),
	UTIL(BOT_UTIL_HIDE_FROM_ENEMY),
	UTIL(BOT_UTIL_MEDIC_FINDPLAYER_AT_SPAWN),
	UTIL(BOT_UTIL_HL2DM_GRAVIGUN_PICKUP),
	UTIL(BOT_UTIL_HL2DM_FIND_ARMOR),
	UTIL(BOT_UTIL_FIND_LAST_ENEMY),
	UTIL(BOT_UTIL_HL2DM_USE_CHARGER),
	UTIL(BOT_UTIL_HL2DM_USE_HEALTH_CHARGER),
	UTIL(BOT_UTIL_THROW_GRENADE),
	UTIL(BOT_UTIL_PICKUP_WEAPON),
	UTIL(BOT_UTIL_ATTACK_NEAREST_POINT),
	UTIL(BOT_UTIL_DEFEND_NEAREST_POINT),
	UTIL(BOT_UTIL_DEFEND_BOMB),
	UTIL(BOT_UTIL_DEFUSE_BOMB),
	UTIL(BOT_UTIL_PLANT_BOMB),
	UTIL(BOT_UTIL_PLANT_NEAREST_BOMB),
	UTIL(BOT_UTIL_DEFUSE_NEAREST_BOMB),
	UTIL(BOT_UTIL_DEFEND_NEAREST_BOMB),
	UTIL(BOT_UTIL_PICKUP_BOMB),
	UTIL(BOT_UTIL_PIPE_NEAREST_SENTRY),
	UTIL(BOT_UTIL_PIPE_LAST_ENEMY),
	UTIL(BOT_UTIL_PIPE_LAST_ENEMY_SENTRY),
	UTIL(BOT_UTIL_DOD_PICKUP_OBJ),
	UTIL(BOT_UTIL_HL2DM_USE_CRATE),
	UTIL(BOT_UTIL_PLACE_BUILDING),
	UTIL(BOT_UTIL_SPYCHECK_AIR),
	UTIL(BOT_UTIL_FIND_MEDIC_FOR_HEALTH),
	UTIL(BOT_UTIL_FIND_SQUAD_LEADER),
	UTIL(BOT_UTIL_FOLLOW_SQUAD_LEADER),
	UTIL(BOT_UTIL_ATTACK_SENTRY),
	UTIL(BOT_UTIL_SPAM_LAST_ENEMY),
	UTIL(BOT_UTIL_SPAM_NEAREST_SENTRY),
	UTIL(BOT_UTIL_SPAM_LAST_ENEMY_SENTRY),
	UTIL(BOT_UTIL_SAP_NEAREST_DISP),
	UTIL(BOT_UTIL_SAP_ENEMY_DISP),
	UTIL(BOT_UTIL_SAP_LASTENEMY_DISP),
	UTIL(BOT_UTIL_BUILDTELENT_SPAWN),
	UTIL(BOT_UTIL_INVESTIGATE_POINT),
	UTIL(BOT_UTIL_COVER_POINT),
	UTIL(BOT_UTIL_SNIPE_POINT),
	UTIL(BOT_UTIL_MOVEUP_MG),
	UTIL(BOT_UTIL_SNIPE_CROSSBOW),
	UTIL(BOT_UTIL_MAX),
};
#undef UTIL

CBotUtility ::CBotUtility(CBot *pBot, eBotAction id, bool bCanDo, float fUtil, CBotWeapon *pWeapon, int iData,
                          Vector vec)
{
	m_iData    = iData;
	m_fUtility = fUtil;
	m_id       = id;
	m_bCanDo   = bCanDo;
	m_pBot     = pBot;
	m_pWeapon  = pWeapon;
	m_vVector  = vec;

	if (m_pBot && m_pBot->isTF2())
	{
		int iClass = CClassInterface::getTF2Class(pBot->getEdict());

		if (CTeamFortress2Mod::isAttackDefendMap() && (m_pBot->getTeam() == TF2_TEAM_BLUE))
			m_fUtility += randomFloat(CRCBotTF2UtilFile::m_fUtils[BOT_ATT_UTIL][id][iClass].min,
			                          CRCBotTF2UtilFile::m_fUtils[BOT_ATT_UTIL][id][iClass].max);
		else
			m_fUtility += randomFloat(CRCBotTF2UtilFile::m_fUtils[BOT_NORM_UTIL][id][iClass].min,
			                          CRCBotTF2UtilFile::m_fUtils[BOT_NORM_UTIL][id][iClass].max);
	}
}

// Execute a list of possible actions and put them into order of available actions against utility
void CBotUtilities ::execute()
{
	unsigned int i = 0;
	CBotUtility *pUtil;
	float fUtil;

	util_node_t *temp;
	util_node_t *pnew;
	util_node_t *prev;

	m_pBest.head = nullptr;

	for (i = 0; i < m_Utilities.size(); i++)
	{
		pUtil = &(m_Utilities[i]);
		fUtil = pUtil->getUtility();

		// if bot can do this action
		if (pUtil->canDo())
		{
			// add to list
			temp = m_pBest.head;

			// put in correct order by making a linked list
			pnew = (util_node_t *)malloc(sizeof(util_node_t));

			if (pnew != nullptr)
			{
				pnew->util = pUtil;
				pnew->next = nullptr;
				prev       = nullptr;

				if (temp)
				{
					while (temp)
					{
						// put into correct position
						if (fUtil > temp->util->getUtility())
						{
							if (temp == m_pBest.head)
							{
								pnew->next   = temp;
								m_pBest.head = pnew;
								break;
							}
							else
							{
								prev->next = pnew;
								pnew->next = temp;
								break;
							}
						}

						prev = temp;
						temp = temp->next;
					}

					if (pnew->next == nullptr)
						prev->next = pnew;
				}
				else
					m_pBest.head = pnew;
			}
		}
	}

	// return pBest;
}

void CBotUtilities ::freeMemory()
{
	util_node_t *temp;
	m_Utilities.clear();

	// FREE LIST
	while ((temp = m_pBest.head) != nullptr)
	{
		temp         = m_pBest.head;
		m_pBest.head = m_pBest.head->next;
		free(temp);
	}
}

CBotUtility *CBotUtilities ::nextBest()
{
	CBotUtility *pBest;
	util_node_t *temp;

	if (m_pBest.head == nullptr)
		return nullptr;

	pBest        = m_pBest.head->util;

	temp         = m_pBest.head;

	m_pBest.head = m_pBest.head->next;

	free(temp);

	return pBest;
}