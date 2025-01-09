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
#ifndef __RCBOT_EHANDLE_H__
#define __RCBOT_EHANDLE_H__

#include "edict.h"
#include "ehandle.h"
#include "eiface.h"
#include "entitylist_base.h"
#include "rcbot/logging.h"

class MyEHandle
{
  private:
	CHandle<IServerEntity> m_Handle;

  public:
	MyEHandle() = default;

	MyEHandle(edict_t *edict)
	{
		if (!edict)
			m_Handle.Term();
		else
			m_Handle = edict->GetIServerEntity();
	}

	static CBaseEntityList *&g_pEntityList()
	{
		static CBaseEntityList *g_pEntityList = nullptr;
		return g_pEntityList;
	}

	static void SetEntListPtr(CBaseEntityList *entList)
	{
		MyEHandle::g_pEntityList() = entList;
	}

	inline bool IsValid() const
	{
		return m_Handle.IsValid();
	}

	inline edict_t *Get() const
	{
		extern IServerGameEnts *servergameents;

		auto handleEntity      = g_pEntityList()->LookupEntity(m_Handle);
		static auto lastEntity = handleEntity;
		if (handleEntity != lastEntity)
		{
			lastEntity = handleEntity;
			logger->Log(LogLevel::WARN, "%lu", handleEntity);
		}
		return !handleEntity
		         ? nullptr
		         : servergameents->BaseEntityToEdict(reinterpret_cast<IServerEntity *>(handleEntity)->GetBaseEntity());
	}

	inline bool operator==(intptr_t a) const
	{
		return reinterpret_cast<intptr_t>(Get()) == a;
	}

	inline bool operator==(edict_t *edict) const
	{
		return Get() == edict;
	}

	inline bool operator==(MyEHandle &other) const
	{
		return Get() == other.Get();
	}

	inline edict_t *operator=(edict_t *edict)
	{
		if (!edict)
			m_Handle.Term();
		else
			m_Handle = edict->GetIServerEntity();

		return edict;
	}

	inline operator edict_t *() const
	{
		return Get();
	}
};

#endif