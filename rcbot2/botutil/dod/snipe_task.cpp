#include "snipe_task.h"

#include "bot_dod_bot.h"
#include "bot_getprop.h"
#include "bot_globals.h"
#include "bot_weapons.h"

CBotDODSnipe ::CBotDODSnipe(CBotWeapon *pWeaponToUse, Vector vOrigin, float fYaw, bool bUseZ, float z,
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
	m_fTimeout      = 0.0f;
}

void CBotDODSnipe ::debugString(char *string)
{
	sprintf(string, "CBotDODSnipe\nm_fTime = %0.2f\npWeaponToUse = %s\nm_bUseZ = %s\nm_z = %0.2f", m_fTime,
	        m_pWeaponToUse->getWeaponInfo()->getWeaponName(), m_bUseZ ? "true" : "false", m_z);
}

void CBotDODSnipe ::execute(CBot *pBot, CBotSchedule *pSchedule)
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
		m_fEnemyTime = 0.0f;
		m_fTime      = engine->Time() + randomFloat(20.0f, 40.0f);
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

	if (pCurrentWeapon != m_pWeaponToUse)
	{
		if (!pBot->select_CWeapon(CWeapons::getWeapon(m_pWeaponToUse->getID())))
		{
			fail();
		}

		return;
	}
	else
	{
		if (pCurrentWeapon->isZoomable())
			bDeployedOrZoomed = CClassInterface::isSniperWeaponZoomed(pCurrentWeapon->getWeaponEntity());
		else if (pCurrentWeapon->isDeployable())
			bDeployedOrZoomed = CClassInterface::isMachineGunDeployed(pCurrentWeapon->getWeaponEntity());

		if (m_fScopeTime < engine->Time())
		{
			if (!bDeployedOrZoomed)
			{
				pBot->secondaryAttack();

				if (m_fTimeout == 0.0f)
					m_fTimeout = engine->Time();
				else if ((m_fTimeout + 3.0f) < engine->Time())
					fail();
			}
			else
				m_fTimeout = 0.0f;

			m_fScopeTime = engine->Time() + randomFloat(0.5f, 1.0f);
		}
	}

	if (pCurrentWeapon->getAmmo(pBot) < 1)
	{
		if (bDeployedOrZoomed)
			pBot->secondaryAttack();

		complete();
	}
	else if (pBot->distanceFrom(m_vOrigin) > 200) // too far from sniper point
	{
		if (bDeployedOrZoomed)
			pBot->secondaryAttack();
		// too far away
		fail();
	}

	if ((m_fEnemyTime + 5.0f) > engine->Time())
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(m_vLastEnemy);
	}
	else if (m_bUseZ)
	{
		Vector vAim = Vector(m_vAim.x, m_vAim.y, m_z);
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(vAim));
	}
	else
	{
		pBot->setLookAtTask(LOOK_VECTOR);
		pBot->setLookVector(pBot->snipe(m_vAim));
	}

	fDist = (m_vOrigin - pBot->getOrigin()).Length2D();

	if ((fDist > 16) || !bDeployedOrZoomed)
	{
		pBot->setMoveTo(m_vOrigin);
		pBot->setMoveSpeed(CClassInterface::getMaxSpeed(pBot->getEdict()) / 8);

		if ((fDist < 48) && ((CDODBot *)pBot)->withinTeammate())
			fail();
	}
	else
	{

		bool unprone = false;

		pBot->stopMoving();

		if (m_iWaypointType & CWaypointTypes::W_FL_PRONE)
		{
			// pBot->updateDanger(MAX_BELIEF);
			pBot->removeCondition(CONDITION_RUN);
			pBot->updateCondition(CONDITION_PRONE);
		}
		else
		{
			if (m_iWaypointType & CWaypointTypes::W_FL_CROUCH)
				pBot->duck();
			// refrain from proning
			pBot->updateCondition(CONDITION_RUN);
			pBot->removeCondition(CONDITION_PRONE);
			unprone = true;
		}

		if (unprone)
		{
			CClassInterface::getPlayerInfoDOD(pBot->getEdict(), &unprone, nullptr);
			if (unprone)
			{
				CDODBot *pDODBot = (CDODBot *)pBot;
				pDODBot->unProne();
			}
		}

		// no enemy for a while
		if ((m_fEnemyTime + m_fTime) < engine->Time())
		{
			if (bDeployedOrZoomed)
				pBot->secondaryAttack();

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

		m_vLastEnemy = CBotGlobals::entityOrigin(pBot->getEnemy());
	}
}