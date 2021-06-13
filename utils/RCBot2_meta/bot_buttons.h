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
#ifndef __BOT_BUTTONS_H__
#define __BOT_BUTTONS_H__

#include <vector>
#include <memory>

class CBotButton
{
public:
	CBotButton ( int iId )
	{
		m_iButtonId = iId;
		m_bTapped = false;		
	}

	inline void tap () { m_bTapped = true; }

	inline bool held ( float fTime )
	{
		return m_bTapped || ((fTime >= m_fTimeStart) && (fTime <= m_fTimeEnd));// && (!m_fLetGoTime||(fTime > m_fLetGoTime));
	}

	inline bool canPress (float fTime)
	{
		return !m_bTapped || (m_fLetGoTime < fTime);
	}

	inline int getID ()
	{
		return m_iButtonId;
	}

	void letGo ()
	{
		m_fTimeStart = 0.0f;
		m_fTimeEnd = 0.0f;
		m_fLetGoTime = 0.04f; // bit of latency
		m_bTapped = false;
	}

	inline void unTap () { m_bTapped = false; }

	void hold ( float fFrom = 0.0, float fFor = 1.0f, float m_fLetGoTime = 0.0f );
private:
	int m_iButtonId = 0;
	float m_fTimeStart = 0.0f;
	float m_fTimeEnd = 0.0f;
	float m_fLetGoTime = 0.0f;

	bool m_bTapped = false;
};

class CBotButtons
{
public:
	CBotButtons();
	~CBotButtons() = default;

	void letGo (int iButtonId);
	void holdButton ( int iButtonId, float fFrom = 0.0, float fFor = 1.0f, float m_fLetGoTime = 0.0f );

	inline void add ( int iInputType );

	bool holdingButton ( int iButtonId );
	bool canPressButton ( int iButtonId );

	void tap ( int iButtonId );

	void letGoAllButtons ( bool bVal ) { m_bLetGoAll = bVal; }

	int getBitMask ();

	////////////////////////////

	void attack (float fFor = 1.0, float fFrom = 0);
	void jump (float fFor = 1.0, float fFrom = 0);
	void duck (float fFor = 1.0, float fFrom = 0);

private:
	std::vector<std::unique_ptr<CBotButton>> m_theButtons;
	bool m_bLetGoAll = false;
};
#endif