#include "snipe_task.h"

#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_waypoint.h"
#include "bot_weapons.h"

CBotHL2DMSnipe ::CBotHL2DMSnipe(CBotWeapon *pWeaponToUse, Vector vOrigin, float fYaw, bool bUseZ, float z,
                                int iWaypointType)
{
	QAngle angle;
	m_fEnemyTime = 0.0f;
	m_fTime      = 0.0f;
	angle        = QAngle(0, fYaw, 0);
	AngleVectors(angle, &m_vAim);
	m_vAim          = vOrigin + (m_vAim * 1024);
	m_vOrigin       = vOrigin;
	m_pWeaponToUse  = pWeaponToUse;
	m_fScopeTime    = 0;
	m_bUseZ         = bUseZ;
	m_z             = z; // z = ground level
	m_iWaypointType = iWaypointType;
}

void CBotHL2DMSnipe ::debugString(char *string)
{
	sprintf(string, "CBotHL2DMSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f", m_fTime,
	        m_pWeaponToUse->getWeaponInfo()->getWeaponName(), m_bUseZ ? "true" : "false", m_z);
}

void CBotHL2DMSnipe ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	static CBotWeapon *pCurrentWeapon;
	static CWeapon *pWeapon;

	static bool bDeployedOrZoomed;
	static float fDist;

	bDeployedOrZoomed = false;

	pBot->wantToShoot(false);
	pBot->wantToListen(false);

	if (m_fTime == 0.0f)
	{
		m_fEnemyTime = engine->Time();
		m_fTime      = m_fEnemyTime + randomFloat(20.0f, 40.0f);
		pBot->resetLookAroundTime();
	}

	pCurrentWeapon = pBot->getCurrentWeapon();

	if (!pCurrentWeapon)
	{
		fail();
		return;
	}

	pWeapon = pCurrentWeapon->getWeaponInfo();

	if (pWeapon == nullptr)
	{
		fail();
		return;
	}

	// refrain from proning
	pBot->updateCondition(CONDITION_RUN);

	if (pCurrentWeapon != m_pWeaponToUse)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())))
		{
			fail();
		}

		return;
	}

	if (pCurrentWeapon->getAmmo(pBot) < 1)
	{
		complete();
	}
	else if (pBot->distanceFrom(m_vOrigin) > 200) // too far from sniper point
	{
		// too far away
		fail();
	}

	if (m_bUseZ)
	{
		Vector vAim = Vector(m_vAim.x, m_vAim.y, m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_SNIPE);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if (fDist > 16)
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict()) / 8);

		// if ( ( fDist < 48 ) && ((CDODBot*)pBot)->withinTeammate() )
		//	fail();
	}
	else
	{
		pBot->stopMoving();

		if (m_iWaypointType & CWaypointTypes::W_FL_CROUCH)
			pBot->duck();

		// no enemy for a while
		if ((m_fEnemyTime + m_fTime) < engine->Time())
		{
			// if ( bDeployedOrZoomed )
			//	pBot->secondaryAttack();

			complete();
		}
	}

	if (pBot->hasEnemy())
	{
		pBot->setMoveLookPriority(MOVELOOK_ATTACK);
		pBot->setLookAtTask(LOOK_ENEMY);
		pBot->handleAttack(pCurrentWeapon, pBot->getEnemy());
		pBot->setMoveLookPriority(MOVELOOK_TASK);

		// havin' fun
		m_fEnemyTime = engine->Time();
	}
}