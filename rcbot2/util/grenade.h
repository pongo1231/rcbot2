#pragma once

#include "bot_cvars.h"
#include "bot_globals.h"

#include <basetypes.h>
#include <edict.h>
#include <mathlib.h>

namespace Util
{
	// desx and desy must be normalized
	// desx = distance (should be 2d)
	// desy = height offset
	inline void GetGrenadeAngle(float v, float g, float desx, float desy, float *fa1, float *fa2)
	{
		// normalize
		float fmax = MAX(v, MAX(g, MAX(desx, desy)));

		v /= fmax;
		g /= fmax;
		desx /= fmax;
		desy /= fmax;
		double vsquared    = v * v;
		double vfourth     = vsquared * vsquared;
		double x2          = desx * desx;
		double gx2         = g * x2;
		double twoyv2      = 2 * desy * vsquared;
		double fourabplusa = vfourth - g * (gx2 + twoyv2);
		double topplus     = vsquared + sqrt(fourabplusa);
		double topminus    = vsquared - sqrt(fourabplusa);
		double bottom      = g * desx;

		*fa1               = (float)atan(topplus / bottom);
		*fa2               = (float)atan(topminus / bottom);

		*fa1               = RAD2DEG(*fa1);
		*fa2               = RAD2DEG(*fa2);

		return;
	}

	inline float GetGrenadeZ(edict_t *pShooter, edict_t *pTarget, Vector vOrigin, Vector vTarget, float fInitialSpeed)
	{
		float fAngle1, fAngle2;

		Vector vForward;
		QAngle angles;

		Vector v_comp = (vTarget - vOrigin);

		float fDist2D = v_comp.Length2D();
		float fDist3D = v_comp.Length();

		GetGrenadeAngle(fInitialSpeed, sv_gravity.GetFloat(), fDist2D, vTarget.z - vOrigin.z, &fAngle1, &fAngle2);

		angles = QAngle(0, 0, 0);
		// do a quick traceline to check which angle to choose (minimum angle = straight)

		if (CBotGlobals::isShotVisible(pShooter, vOrigin, vTarget, pTarget))
			angles.x = -MIN(fAngle1, fAngle2);
		else
			angles.x = -MAX(fAngle1, fAngle2);

		AngleVectors(angles, &vForward);

		// add gravity height
		return (vOrigin + (vForward * fDist3D)).z;
	}
}