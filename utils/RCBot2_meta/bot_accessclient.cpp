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
#include "bot.h"
#include "bot_strings.h"
#include "bot_accessclient.h"
#include "bot_globals.h"

#include "rcbot/logging.h"

#include <vector>
///////////

std::vector<CAccessClient*> CAccessClients :: m_Clients;

///////////

CAccessClient :: CAccessClient( char *szSteamId, int iAccessLevel )
{
	m_iAccessLevel = iAccessLevel;
	m_szSteamId = CStrings::getString(szSteamId);
}

bool CAccessClient :: forBot ()
{
	return isForSteamId("BOT");
}

bool CAccessClient :: isForSteamId ( const char *szSteamId )
{
	logger->Log(LogLevel::DEBUG, "AccessClient: '%s','%s'", m_szSteamId, szSteamId);
	return FStrEq(m_szSteamId,szSteamId);
}

void CAccessClient :: save ( std::fstream &fp )
{
	fp << '"' << m_szSteamId << '"' << ":" << m_iAccessLevel << "\n";
}

void CAccessClient :: giveAccessToClient ( CClient *pClient )
{
	// notify player
	if ( !forBot() )
		CBotGlobals::botMessage(pClient->getPlayer(),0,"%s authenticated for bot commands",pClient->getName());
	// notify server
	logger->Log(LogLevel::INFO, "%s authenticated for bot commands", pClient->getName());

	pClient->setAccessLevel(m_iAccessLevel);
}

//////////////

void CAccessClients :: showUsers ( edict_t *pEntity )
{
	CAccessClient *pPlayer;
	CClient *pClient;

	CBotGlobals::botMessage(pEntity,0,"showing users...");

	if ( m_Clients.empty() )
		logger->Log(LogLevel::DEBUG, "showUsers() : No users to show");

	for ( unsigned int i = 0; i < m_Clients.size(); i ++ )
	{
		pPlayer = m_Clients[i];
		
		pClient = CClients::findClientBySteamID(pPlayer->getSteamID());
		
		if ( pClient )
			CBotGlobals::botMessage(pEntity,0,"[ID: %s]/[AL: %d] (currently playing as : %s)\n",pPlayer->getSteamID(),pPlayer->getAccessLevel(),pClient->getName());
		else
			CBotGlobals::botMessage(pEntity,0,"[ID: %s]/[AL: %d]\n",pPlayer->getSteamID(),pPlayer->getAccessLevel());

	}	
}

void CAccessClients :: createFile ()
{
	char filename[1024];
	
	CBotGlobals::buildFileName(filename,BOT_ACCESS_CLIENT_FILE,BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	std::fstream fp = CBotGlobals::openFile(filename, std::fstream::out);

	logger->Log(LogLevel::WARN, "Making an accessclients.ini file for you... Edit it in %s", filename);

	if ( fp )
	{
		fp << "# format is ";
		fp << "# \"<STEAM ID>\" <access level>\n";
		fp << "# see http://rcbot.bots-united.com/accesslev.htm for access\n";
		fp << "# levels\n";
		fp << "#\n";
		fp << "# example:\n";
		fp << "#\n";
		fp << "# \"STEAM_0:123456789\" 63\n";
		fp << "# don't put one of '#' these before a line you want to be read \n";
		fp << "# by the bot!\n";
		fp << "# \n";
	}
	else
		logger->Log(LogLevel::ERROR, "Failed to create config file %s", filename);
}

void CAccessClients :: freeMemory ()
{
	for ( unsigned int i = 0; i < m_Clients.size(); i ++ )
	{
		delete m_Clients[i];
		m_Clients[i] = NULL;
	}

	m_Clients.clear();
}

void CAccessClients :: load ()
{
	char filename[1024];
	
	CBotGlobals::buildFileName(filename,BOT_ACCESS_CLIENT_FILE,BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	std::fstream fp = CBotGlobals::openFile(filename, std::fstream::in);

	if ( fp )
	{
		char buffer[256];

		char szSteamId[32];
		int iAccess;

		int i;
		int len;
		int n;

		int iLine = 0;

		while (fp.getline(buffer,255))
		{
			iLine++;

			buffer[255] = 0;

			if ( buffer[0] == 0 )
				continue;
			if ( buffer[0] == '\n' )
				continue;
			if ( buffer[0] == '\r' )
				continue;
			if ( buffer[0] == '#' )
				continue;

			len = strlen(buffer);

			i = 0;

			while (( i < len ) && ((buffer[i] == '\"') || (buffer[i] == ' ')))
				i++;

			n = 0;

			// parse Steam ID
			while ( (n<31) && (i < len) && (buffer[i] != '\"') )			
				szSteamId[n++] = buffer[i++];

			szSteamId[n] = 0;

			i++;

			while (( i < len ) && (buffer[i] == ' '))
				i++;

			if ( i == len )
			{
				logger->Log(LogLevel::WARN, "line %d invalid in access client config, missing access level", iLine);
				continue; // invalid
			}

			iAccess = atoi(&buffer[i]);

			// invalid
			if ( (szSteamId[0] == 0) || (szSteamId[0] == ' ' ) )
			{
				logger->Log(LogLevel::WARN, "line %d invalid in access client config, steam id invalid", iLine);
				continue;
			}
			if ( iAccess == 0 )
			{
				logger->Log(LogLevel::WARN, "line %d invalid in access client config, access level can't be 0", iLine);
				continue;
			}

			m_Clients.push_back(new CAccessClient(szSteamId,iAccess));
		}
	}
	else
		CAccessClients :: createFile();
}

void CAccessClients :: save ()
{
	char filename[1024];
	
	CBotGlobals::buildFileName(filename,BOT_ACCESS_CLIENT_FILE,BOT_CONFIG_FOLDER,BOT_CONFIG_EXTENSION);

	std::fstream fp = CBotGlobals::openFile(filename, std::fstream::out);

	if ( fp )
	{
		for ( unsigned int i = 0; i < m_Clients.size(); i ++ )
		{
			m_Clients[i]->save(fp);
		}
	}
}

void CAccessClients :: checkClientAccess ( CClient *pClient )
{
	for ( unsigned int i = 0; i < m_Clients.size(); i ++ )
	{
		CAccessClient *pAC = m_Clients[i];

		if ( pAC->isForSteamId(pClient->getSteamID()) )
			pAC->giveAccessToClient(pClient);
	}
}
