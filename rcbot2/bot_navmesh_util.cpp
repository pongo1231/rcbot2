#include "bot_navmesh_util.h"
#include "bot_globals.h"

bool NavMeshUtil::IsOnWalkableGround(const Vector &pos)
{
	static const float MAX_FALL_DISTANCE = 512.0f;
	static const float MAX_FLOOR_ANGLE   = 0.7f; // cos(45°) ~ walkable slope

	Vector vDown = pos;
	vDown.z -= MAX_FALL_DISTANCE;

	CBotGlobals::traceLine(pos, vDown, MASK_PLAYERSOLID, nullptr);

	trace_t *pTrace = CBotGlobals::getTraceResult();
	if (pTrace->fraction >= 1.0f)
		return false; // nothing hit — in open air

	if (pTrace->startsolid)
		return false; // inside geometry

	Vector vNormal = pTrace->plane.normal;
	if (vNormal.z < MAX_FLOOR_ANGLE)
		return false; // surface too steep — wall, not floor

	return true;
}

bool NavMeshUtil::IsOnWalkableGround(edict_t *pEntity)
{
	if (!pEntity || !CBotGlobals::entityIsValid(pEntity))
		return false;

	return IsOnWalkableGround(CBotGlobals::entityOrigin(pEntity));
}
