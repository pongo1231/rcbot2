#include "build_building_task.h"

#include "bot_globals.h"
#include "bot_mods.h"

CBotTFEngiBuildTask ::CBotTFEngiBuildTask(eEngiBuild iObject, CWaypoint *pWaypoint)
{
	m_iObject          = iObject;
	m_vOrigin          = pWaypoint->getOrigin() + pWaypoint->applyRadius();
	m_iState           = 0;
	m_fTime            = 0;
	m_iTries           = 0;
	m_fNextUpdateAngle = 0.0f;
	QAngle ang         = QAngle(0, pWaypoint->getAimYaw(), 0);
	Vector vForward;
	AngleVectors(ang, &vForward);
	m_vAimingVector = m_vOrigin + (vForward * 100.0f);
	m_iArea         = pWaypoint->getArea();
	m_vBaseOrigin   = m_vOrigin;
	m_fRadius       = pWaypoint->getRadius();
}

void CBotTFEngiBuildTask ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	CBotFortress *tfBot;

	bool bAimingOk = true;

	pBot->wantToInvestigateSound(false);

	// if ( !pBot->isTF() ) // shouldn't happen ever
	//	fail();

	if (!CTeamFortress2Mod::m_ObjectiveResource.isWaypointAreaValid(m_iArea))
		fail();

	pBot->wantToShoot(false);        // don't shoot enemies , want to build the damn thing
	pBot->wantToChangeWeapon(false); // if enemy just strike them with wrench
	pBot->wantToListen(
	    false); // sometimes bots dont place sentries because they are looking the wrong way due to a noise

	tfBot = (CBotFortress *)pBot;

	if (tfBot->getClass() != TF_CLASS_ENGINEER)
	{
		fail();
		return;
	}
	/*else if ( pBot->hasEnemy() && pBot->hasSomeConditions(CONDITION_SEE_CUR_ENEMY) )
	{
	    fail();
	    return;
	}*/

	if (m_fTime == 0.0f)
	{
		pBot->resetLookAroundTime();

		if (CTeamFortress2Mod::buildingNearby(pBot->getTeam(), m_vOrigin))
		{
			if (m_fRadius > 0.0f)
				m_vOrigin =
				    m_vBaseOrigin + Vector(randomFloat(-m_fRadius, m_fRadius), randomFloat(-m_fRadius, m_fRadius), 0);
			else
			{
				// can't build here
				fail();
				return;
			}
		}

		m_fTime            = engine->Time() + randomFloat(4.0f, 8.0f);

		m_fNextUpdateAngle = engine->Time() + 0.5f;
	}
	else if (m_fTime < engine->Time())
		fail();

	if (m_iObject == ENGI_DISP)
	{
		edict_t *pSentry = tfBot->getSentry();

		if (pSentry && CBotGlobals::entityIsValid(pSentry))
		{
			Vector vSentry  = CBotGlobals::entityOrigin(pSentry);
			Vector vOrigin  = pBot->getOrigin();
			Vector vLookAt  = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			// pBot->setLookVector(vLookAt);
			// pBot->setLookAtTask(LOOK_VECTOR);

			// LOOK_VECTOR,11);
			// bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f; // 15 degrees // <
			// CBotGlobals::yawAngleFromEdict(pBot->getEdict(),pBot->getLookVector()) < 15;
		}
		else
		{
			Vector vSentry  = pBot->getAiming();
			Vector vOrigin  = pBot->getOrigin();
			Vector vLookAt  = vOrigin - (vSentry - vOrigin);

			m_vAimingVector = vLookAt;

			/*pBot->setLookVector(pBot->getAiming());
			pBot->setLookAtTask((LOOK_VECTOR));*/
			// bAimingOk = pBot->DotProductFromOrigin(vLookAt) > 0.965925f;
		}
	}

	if ((m_iTries > 1) && (m_fNextUpdateAngle < engine->Time()))
	{
		QAngle angles;
		Vector vforward = m_vAimingVector - pBot->getOrigin();
		vforward        = vforward / vforward.Length(); // normalize

		VectorAngles(vforward, angles);

		angles.y += randomFloat(-60.0f, 60.0f); // yaw
		CBotGlobals::fixFloatAngle(&angles.y);

		AngleVectors(angles, &vforward);

		vforward           = vforward / vforward.Length();

		m_vAimingVector    = pBot->getOrigin() + vforward * 100.0f;

		m_fNextUpdateAngle = engine->Time() + 1.5f;
	}

	pBot->setLookAtTask(LOOK_VECTOR, 0.2f);
	pBot->setLookVector(m_vAimingVector);

	bAimingOk = pBot->isFacing(m_vAimingVector); // 15 degrees

	if (pBot->distanceFrom(m_vOrigin) > 70.0f)
	{
		if (!CBotGlobals::isVisible(pBot->getEdict(), pBot->getEyePosition(), m_vOrigin))
			fail();
		else
			pBot->setMoveTo(m_vOrigin);
	}
	else if (bAimingOk || (m_iTries > 1))
	{
		int state = tfBot->engiBuildObject(&m_iState, m_iObject, &m_fTime, &m_iTries);

		if (state == 1)
			complete();
		else if (state == 0)
			fail();
		else if (state == 3) // failed , try again
		{
			if (m_fRadius > 0.0f)
				m_vOrigin =
				    m_vBaseOrigin + Vector(randomFloat(-m_fRadius, m_fRadius), randomFloat(-m_fRadius, m_fRadius), 0);
			else
			{
				// can't build here
				fail();
				return;
			}
		}
	}
}

void CBotTFEngiBuildTask ::debugString(char *string)
{
	sprintf(string, "CBotTFEngiBuildTask (%d,%0.4f,%0.4f,%0.4f)", m_iObject, m_vOrigin.x, m_vOrigin.y, m_vOrigin.z);
}