#pragma once

struct edict_t;
class Vector;

namespace NavMeshUtil
{
	bool IsOnWalkableGround(const Vector &pos);
	bool IsOnWalkableGround(edict_t *pEntity);
};
