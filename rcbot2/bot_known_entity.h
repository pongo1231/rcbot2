#pragma once

#include "bot_ehandle.h"

class CKnownEntity
{
  public:
	CKnownEntity(edict_t *pEntity)
	{
		m_who                 = MyEHandle(pEntity);
		m_vLastPosition       = CBotGlobals::entityOrigin(pEntity);
		m_fWhenLastSeen       = engine->Time();
		m_fWhenLastKnown      = engine->Time();
		m_fWhenBecameKnown    = engine->Time();
		m_bPositionConfirmed  = true;
	}

	inline edict_t *getEntity() { return m_who.get(); }

	inline bool isVisibleRecently(float fWindow = 3.0f) const
	{
		return (engine->Time() - m_fWhenLastSeen) < fWindow;
	}

	inline bool isObsolete() const
	{
		edict_t *pEnt = m_who.get();
		if (!pEnt || !CBotGlobals::entityIsValid(pEnt) || !CBotGlobals::entityIsAlive(pEnt))
			return true;
		return (engine->Time() - m_fWhenLastKnown) > 10.0f;
	}

	inline void updatePosition(Vector vPos)
	{
		m_vLastPosition      = vPos;
		m_fWhenLastSeen       = engine->Time();
		m_fWhenLastKnown      = engine->Time();
		m_bPositionConfirmed  = true;
	}

	inline void markLost()
	{
		m_fWhenLastKnown      = engine->Time();
		m_bPositionConfirmed  = true; // was confirmed before losing sight
	}

	MyEHandle m_who;
	Vector    m_vLastPosition;
	float     m_fWhenLastSeen;
	float     m_fWhenLastKnown;
	float     m_fWhenBecameKnown;
	bool      m_bPositionConfirmed;
};
