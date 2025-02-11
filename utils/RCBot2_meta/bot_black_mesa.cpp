/*
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

#include <cstring>

#include "bot.h"
#include "bot_black_mesa.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_mtrand.h"

 //caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

void CBotBMS::init(const bool bVarInit)
{
    CBot::init(bVarInit);
}

void CBotBMS::setup()
{
    CBot::setup();
}

bool CBotBMS::startGame()
{
    return true;
}

void CBotBMS::killed(edict_t* pVictim)
{
    return;
}

void CBotBMS::died(edict_t* pKiller, const char* pszWeapon)
{
    spawnInit();

    if (randomInt(0, 1))
        m_pButtons->attack();
}

void CBotBMS::spawnInit()
{
    CBot::spawnInit();
}

bool CBotBMS::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
    if (!pEdict)
        return false;

    if (pEdict == m_pEdict)
        return false;

    if (!ENTINDEX(pEdict))
        return false;

    /*if (ENTINDEX(pEdict) > CBotGlobals::maxClients()) // monster
    {
        const char* cname = pEdict->GetNetworkable()->GetClassName();

        if (std::strncmp("monster_", cname, 8) == 0)
        {
            if (std::strcmp(cname, "monster_barney") == 0)
                return false;
            if (std::strcmp(cname, "monster_scientist") == 0)
                return false;
            if (std::strcmp(cname, "monster_gman") == 0)
                return false;
            if (std::strcmp(cname, "monster_furniture") == 0)
                return false;

            return true;
        }
    }*/

    if (CBotGlobals::getTeamplayOn())
    {
        if (CBotGlobals::getTeam(pEdict) == getTeam())
            return false;
    }

    return true;
}

void CBotBMS::modThink()
{
    // find weapons and neat stuff
}