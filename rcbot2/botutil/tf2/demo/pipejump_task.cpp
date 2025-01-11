#include "pipejump_task.h"

#include "bot_cvars.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_weapons.h"

#include <in_buttons.h>

CBotTF2DemomanPipeJump ::CBotTF2DemomanPipeJump(CBot *pBot, Vector vWaypointGround, Vector vWaypointNext,
                                                CBotWeapon *pWeapon)
{
	m_iStartingAmmo = pWeapon->getClip1(pBot);
	m_vStart        = vWaypointGround - Vector(0, 0, 48.0);
	m_vEnd          = vWaypointNext;
	m_pPipeBomb     = nullptr;
	m_fTime         = 0;
	m_iState        = 0;
	m_pWeapon       = pWeapon;
	m_bFired        = false;
}

void CBotTF2DemomanPipeJump ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	if (m_fTime == 0)
	{
		// init
		m_fTime = engine->Time() + randomFloat(5.0f, 10.0f);
	}

	if (m_fTime < engine->Time())
		fail(); // time out

	if (pBot->getCurrentWeapon() != m_pWeapon)
	{
		pBot->selectBotWeapon(m_pWeapon);
		return;
	}

	if (m_pPipeBomb)
	{
		if (!CBotGlobals::entityIsValid(m_pPipeBomb))
		{
			fail();
			return;
		}
	}

	switch (m_iState)
	{
	case 0:
		if (m_pWeapon->getClip1(pBot) == 0)
		{
			if (m_pWeapon->getAmmo(pBot) == 0)
				fail();
			else if (randomInt(0, 1))
				pBot->tapButton(IN_RELOAD);
		}
		else
		{
			if (!m_bFired && !m_iStartingAmmo)
				m_iStartingAmmo = m_pWeapon->getClip1(pBot);
			else if (m_bFired && (m_iStartingAmmo > m_pWeapon->getClip1(pBot)))
			{
				// find pipe bomb
				m_pPipeBomb = CClassInterface::FindEntityByClassnameNearest(
				    pBot->getOrigin(), "tf_projectile_pipe_remote", 150.0f, nullptr);

				if (m_pPipeBomb)
				{
					// set this up incase of fail, the bot knows he has a sticky there
					((CBotTF2 *)pBot)->setStickyTrapType(m_vStart, TF_TRAP_TYPE_ENEMY);
					m_iState++;
				}
				else
					fail();
			}
			else
			{
				pBot->setLookVector(m_vStart);
				pBot->setLookAtTask(LOOK_VECTOR);

				if (pBot->distanceFrom(m_vStart) < 150)
				{
					if (pBot->DotProductFromOrigin(m_vStart) > 0.99)
					{
						if (randomInt(0, 1))
						{
							pBot->primaryAttack();
							m_bFired = true;
						}
					}
				}
				else
				{
					pBot->setMoveTo(m_vStart);
				}
			}
		}
		break;
	case 1:
	{
		Vector v_comp;
		Vector v_startrunup;
		Vector v_pipe;
		Vector vel;

		if (CClassInterface::getVelocity(m_pPipeBomb, &vel))
		{
			if (vel.Length() > 1.0)
				break; // wait until the pipe bomb has rested
		}

		v_comp         = m_vEnd - m_vStart;
		v_comp         = v_comp / v_comp.Length();

		v_pipe         = CBotGlobals::entityOrigin(m_pPipeBomb);
		v_startrunup   = v_pipe - (v_comp * rcbot_demo_runup_dist.GetFloat());
		v_startrunup.z = v_pipe.z;

		pBot->lookAtEdict(m_pPipeBomb);
		pBot->setLookAtTask(LOOK_EDICT);

		// m_pPipeBomb != nullptr

		// run up and jump time

		if (pBot->distanceFrom(v_startrunup) < 52.0f)
			m_iState++;

		pBot->setMoveTo(v_startrunup);
	}
	break;
	case 2:
	{
		Vector v_comp;
		Vector v_endrunup;
		Vector v_pipe;

		v_comp       = m_vEnd - m_vStart;
		v_comp       = v_comp / v_comp.Length();
		v_pipe       = CBotGlobals::entityOrigin(m_pPipeBomb);

		v_endrunup   = v_pipe + (v_comp * rcbot_demo_runup_dist.GetFloat());
		v_endrunup.z = v_pipe.z;

		pBot->setLookVector(m_vEnd);
		pBot->setLookAtTask(LOOK_VECTOR);

		// m_pPipeBomb != nullptr

		// run up and jump time

		if (pBot->distanceFrom(v_endrunup) < 48.0f)
		{
			m_iState++;
		}

		pBot->setMoveTo(v_endrunup);
	}
	break;
	case 3:
		pBot->jump();
		m_iState++;
		break;
	case 4:
	{
		Vector vel;

		if (CClassInterface::getVelocity(pBot->getEdict(), &vel))
		{
			if (vel.z > 10)
			{
				((CBotTF2 *)pBot)->detonateStickies(true);
				complete();
			}
		}
		else
		{
			((CBotTF2 *)pBot)->detonateStickies(true);
			complete();
		}
	}
	break;
	default:
		fail();
	}
}