// bot_navmesh.cpp - Navmesh navigation for RCBot2
// Copyright Paul "pongo" C. 2026

#include "bot_navmesh.h"
#include "bot_globals.h"
#include "bot_kv.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <dlfcn.h>
#include <link.h>
#include <map>
#include <queue>
#include <set>
#include <sys/mman.h>
#include <vector>

#include "bot_cvars.h"
#include "engine_wrappers.h"
#include "in_buttons.h"
#include "bot_fortress.h"
#include "bot_getprop.h"

// For teleporter keyvalue matching
#include "iserverunknown.h"
#include "itoolentity.h"

// For escape mode ground detection
#include "bot_navmesh_util.h"

// ---------- CNavArea geometry helpers (portal-based walk targets) ----------

static inline Vector areaGetCenter(const void *area)
{
	float *c = (float *)((unsigned char *)area + NAV_M_CENTER);
	return Vector(c[0], c[1], c[2]);
}

static inline Vector areaNWCorner(const void *area)
{
	float *p = (float *)((unsigned char *)area + NAV_M_NWCORNER);
	return Vector(p[0], p[1], p[2]);
}

static inline Vector areaSECorner(const void *area)
{
	float *p = (float *)((unsigned char *)area + NAV_M_SECORNER);
	return Vector(p[0], p[1], p[2]);
}

// Bilinear interpolation of floor height at (x,y) within a CNavArea.
// Matches CNavArea::GetZ in the SDK.
static float areaGetZ(const void *area, float x, float y)
{
	unsigned char *a = (unsigned char *)area;
	float invDx = *(float *)(a + NAV_M_INVDX);
	float invDy = *(float *)(a + NAV_M_INVDY);
	if (invDx == 0.0f || invDy == 0.0f)
		return *(float *)(a + NAV_M_NEZ);

	float *nw = (float *)(a + NAV_M_NWCORNER);
	float u = (x - nw[0]) * invDx;
	float v = (y - nw[1]) * invDy;
	if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
	if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;

	float neZ = *(float *)(a + NAV_M_NEZ);
	float swZ = *(float *)(a + NAV_M_SWZ);
	return nw[2] + (neZ - nw[2]) * u + (swZ - nw[2]) * v;
}

// Compute the geometric portal (shared-edge midpoint + half-width)
// between two adjacent CNavAreas.
static void computePortal(const void *from, const void *to, int dir,
                          Vector *center, float *halfWidth)
{
	Vector nwFrom = areaNWCorner(from);
	Vector seFrom = areaSECorner(from);
	Vector nwTo   = areaNWCorner(to);
	Vector seTo   = areaSECorner(to);

	if (dir == 0 || dir == 2) // NORTH or SOUTH
	{
		if (dir == 0) // NORTH: shared edge at from->m_nwCorner.y
			center->y = nwFrom.y;
		else          // SOUTH: shared edge at from->m_seCorner.y
			center->y = seFrom.y;

		float left  = fmaxf(nwFrom.x, nwTo.x);
		float right = fminf(seFrom.x, seTo.x);
		if (left < nwFrom.x)  left  = nwFrom.x;
		if (right > seFrom.x) right = seFrom.x;

		center->x = (left + right) * 0.5f;
		*halfWidth = (right - left) * 0.5f;
	}
	else // EAST or WEST
	{
		if (dir == 3) // WEST: shared edge at from->m_nwCorner.x
			center->x = nwFrom.x;
		else          // EAST: shared edge at from->m_seCorner.x
			center->x = seFrom.x;

		float top    = fmaxf(nwFrom.y, nwTo.y);
		float bottom = fminf(seFrom.y, seTo.y);
		if (top < nwFrom.y)    top    = nwFrom.y;
		if (bottom > seFrom.y) bottom = seFrom.y;

		center->y = (top + bottom) * 0.5f;
		*halfWidth = (bottom - top) * 0.5f;
	}

	center->z = areaGetZ(from, center->x, center->y);
}

void CNavMeshAccessor::init(CRCBotKeyValueList &kv, void *pBase)
{
	findFunc(kv, "navmesh_push_sig", pBase,
	         "\xFF\x35\x2A\x2A\x2A\x2A\xF3\x0F\x11\x45\x9C"
	         "\xE8\x2A\x2A\x2A\x2A\x83\xC4\x10\x85\xC0\x0F\x95\xC0\x74");

	Dl_info info;
	if (dladdr(pBase, &info))
		m_pBase = info.dli_fbase;
}


// Dump all connections for a specific nav area pointer to stderr.
void CNavMeshAccessor::dumpAreaConnections(void *area, int connOff)
{
	if (!area || connOff < 0) return;
	float *c = (float *)((unsigned char *)area + NAV_M_CENTER);
	int attr = *(int *)((unsigned char *)area + NAV_M_ATTR);
	fprintf(stderr, "[RCDiag] areaConn area=%p pos=(%.0f,%.0f,%.0f) attr=0x%x\n",
	    area, c[0], c[1], c[2], attr);
	for (int dir = 0; dir < 4; dir++) {
		char *pData = *(char **)((char *)area + connOff + dir * 4);
		if (!pData || !ptrInArena(pData)) continue;
		int cnt = *(int *)pData;
		if (cnt <= 0) continue;
		fprintf(stderr, "[RCDiag]   dir=%d(N=%d E=%d S=%d W=%d) count=%d:",
		    dir, dir==0?cnt:0, dir==1?cnt:0, dir==2?cnt:0, dir==3?cnt:0, cnt);
		for (int i = 0; i < cnt && i < 8; i++) {
			void *nb = *(void **)(pData + 4 + i * 8);
			if (nb && areaKnown(nb)) {
				float *nc = (float *)((unsigned char *)nb + NAV_M_CENTER);
				fprintf(stderr, " %p(%.0f,%.0f,%.0f)", nb, nc[0], nc[1], nc[2]);
			}
		}
		fprintf(stderr, "\n");
	}
}

// Dump an ASCII grid of nav areas around (cx, cy) with given radius.
// Writes to stderr for capture in the server log.
void CNavMeshAccessor::dumpAreaGrid(float cx, float cy, float radius, int connOff)
{
	if (!m_bReady || m_Areas.empty()) {
		fprintf(stderr, "[RCDiag] navmeshDump: navmesh not ready\n");
		return;
	}
	// Collect areas within radius
	struct DumpArea {
		void *area;
		float x, y, z;
		int nConn;
	};
	std::vector<DumpArea> local;
	for (size_t i = 0; i < m_Areas.size(); i++) {
		unsigned char *a = m_Areas[i];
		float *c = (float *)(a + NAV_M_CENTER);
		float dx = c[0] - cx;
		float dy = c[1] - cy;
		if (dx*dx + dy*dy > radius*radius) continue;
		int nConn = 0;
		if (connOff >= 0) {
			for (int d = 0; d < 4; d++) {
				char *pData = *(char **)(a + connOff + d * 4);
				if (pData && ptrInArena(pData) && *(int*)pData > 0)
					nConn += *(int*)pData;
			}
		}
		DumpArea da;
		da.area = a;
		da.x = c[0]; da.y = c[1]; da.z = c[2];
		da.nConn = nConn;
		local.push_back(da);
	}
	fprintf(stderr, "[RCDiag] navmeshDump: %d areas near (%.0f,%.0f) radius=%.0f\n",
	    (int)local.size(), cx, cy, radius);
	if (local.empty()) return;
	// Find bounding box
	float minX = cx - radius, maxX = cx + radius;
	float minY = cy - radius, maxY = cy + radius;
	float cellSize = 50.0f;  // 50u per char cell
	int gridW = (int)((maxX - minX) / cellSize) + 1;
	int gridH = (int)((maxY - minY) / cellSize) + 1;
	if (gridW > 120) gridW = 120;
	if (gridH > 60) gridH = 60;
	// Build character grid
	std::vector<std::string> grid(gridH, std::string(gridW, '.'));
	for (auto &da : local) {
		int gx = (int)((da.x - minX) / cellSize);
		int gy = (int)((da.y - minY) / cellSize);
		if (gx < 0 || gx >= gridW || gy < 0 || gy >= gridH) continue;
		char ch = '#';
		if (da.nConn > 4) ch = '@';
		else if (da.nConn > 0) ch = 'O';
		else ch = 'x';
		grid[gridH - 1 - gy][gx] = ch;
	}
	// Print grid with coordinate labels
	for (int y = 0; y < gridH; y++) {
		float worldY = maxY - y * cellSize;
		if ((int)(y) % 5 == 0)
			fprintf(stderr, "[RCDiag] %6.0f | %s\n", worldY, grid[y].c_str());
		else
			fprintf(stderr, "[RCDiag]         | %s\n", grid[y].c_str());
	}
	// Print X axis labels
	fprintf(stderr, "[RCDiag]         ");
	for (int x = 0; x < gridW; x += 5)
		fprintf(stderr, "%.0f    ", minX + x * cellSize);
	fprintf(stderr, "\n");
	fprintf(stderr, "[RCDiag] Legend: @=hub(>4conn) O=area(1-4conn) x=isolated\n");
}

void CNavMeshAccessor::scanGrid()
{
	if (!m_func) return;

	uint32_t gotAddr = *(uint32_t *)((uint8_t *)m_func + 2);
	if (!gotAddr || gotAddr < 0x08000000 || gotAddr > 0xFF000000) return;
	m_pNavMesh       = *(void **)(uintptr_t)gotAddr;
	if (!m_pNavMesh) return;

	float cellSize = *(float *)((char *)m_pNavMesh + 0x1c);
	if (cellSize < 20.0f || cellSize > 1000.0f) return;

	int gridWidth   = *(int *)((char *)m_pNavMesh + 0x20);
	int gridHeight  = *(int *)((char *)m_pNavMesh + 0x24);
	char *pGrid     = *(char **)((char *)m_pNavMesh + 0x08);
	int gridCount   = *(int *)((char *)m_pNavMesh + 0x14);

	if (!pGrid || gridWidth <= 0 || gridHeight <= 0) return;

	m_Areas.clear();
	m_nAreaCount = 0;

	for (int cellIdx = 0; cellIdx < gridWidth * gridHeight; cellIdx++)
	{
		char *cell           = pGrid + cellIdx * 0x14;
		unsigned int *pAreas = *(unsigned int **)cell;
		int areaCount         = *(int *)(cell + 12);
		for (int a = 0; a < areaCount; a++)
		{
			unsigned int ptr = pAreas[a];
			if (!ptr || ptr < 0x08000000 || ptr > 0xF0000000) continue;
			if (ptr & 3) continue;

			float *center = (float *)(ptr + NAV_M_CENTER);
			if (center[0] < -16000.0f || center[0] > 16000.0f) continue;
			if (center[1] < -16000.0f || center[1] > 16000.0f) continue;
			if (center[2] < -10000.0f || center[2] > 100000.0f) continue;

			m_Areas.push_back((unsigned char *)ptr);
		}
	}
	std::sort(m_Areas.begin(), m_Areas.end());
	m_Areas.erase(std::unique(m_Areas.begin(), m_Areas.end()), m_Areas.end());
	m_nAreaCount = (int)m_Areas.size();
	m_bReady     = true;

	if (!m_Areas.empty())
	{
		m_areaMin = (uintptr_t)m_Areas.front();
		m_areaMax = (uintptr_t)m_Areas.back();
	}

	detectConnectOffset();

	m_teleportLinks.clear();

	if (gpGlobals && gpGlobals->maxEntities > 0)
	{
		extern IServerTools *servertools;

		struct DestInfo {
			Vector pos;
			char   targetname[256];
		};
		std::vector<DestInfo> destinations;
		for (int ie = 1; ie < gpGlobals->maxEntities; ie++)
		{
			edict_t *pEnt = engine->PEntityOfEntIndex(ie);
			if (!pEnt || pEnt->IsFree()) continue;
			const char *pCls = pEnt->GetClassName();
			if (!pCls || strcmp(pCls, "info_teleport_destination") != 0) continue;

			DestInfo di;
			di.pos = CBotGlobals::entityOrigin(pEnt);
			di.targetname[0] = '\0';

			IServerUnknown *pUnk = pEnt->GetUnknown();
			if (pUnk && servertools)
			{
				CBaseEntity *pBase = pUnk->GetBaseEntity();
				if (pBase)
					servertools->GetKeyValue(pBase, "targetname",
					                         di.targetname,
					                         sizeof(di.targetname));
			}
			destinations.push_back(di);
		}

		for (int ie = 1; ie < gpGlobals->maxEntities; ie++)
		{
			edict_t *pEnt = engine->PEntityOfEntIndex(ie);
			if (!pEnt || pEnt->IsFree()) continue;
			const char *pCls = pEnt->GetClassName();
			if (!pCls || strcmp(pCls, "trigger_teleport") != 0) continue;

			Vector vTele = CBotGlobals::entityOrigin(pEnt);
			unsigned char *srcArea = getNearestArea(vTele);
			if (!srcArea) continue;

			char szTarget[256] = "";
			IServerUnknown *pUnk = pEnt->GetUnknown();
			if (pUnk && servertools)
			{
				CBaseEntity *pBase = pUnk->GetBaseEntity();
				if (pBase)
					servertools->GetKeyValue(pBase, "target",
					                         szTarget,
					                         sizeof(szTarget));
			}

			bool bMatched = false;
			if (szTarget[0])
			{
				for (size_t j = 0; j < destinations.size(); j++)
				{
					if (destinations[j].targetname[0]
					    && strcmp(szTarget, destinations[j].targetname) == 0)
					{
						unsigned char *dstArea =
						    getNearestArea(destinations[j].pos);
						if (dstArea && dstArea != srcArea)
						{
							TeleportLink link;
							link.srcArea = srcArea;
							link.srcX = vTele.x; link.srcY = vTele.y;
							link.srcZ = vTele.z;
							link.dstArea = dstArea;
							link.dstX = destinations[j].pos.x;
							link.dstY = destinations[j].pos.y;
							link.dstZ = destinations[j].pos.z;
							m_teleportLinks.push_back(link);
						}
						bMatched = true;
						break;
					}
				}
			}

			if (!bMatched && !destinations.empty())
			{
				float bestDist = 9999999.0f;
				int   bestIdx  = -1;
				for (size_t j = 0; j < destinations.size(); j++)
				{
					float dx = destinations[j].pos.x - vTele.x;
					float dy = destinations[j].pos.y - vTele.y;
					float d  = dx * dx + dy * dy;
					if (d < bestDist) { bestDist = d; bestIdx = (int)j; }
				}
				if (bestIdx >= 0)
				{
					const DestInfo &di = destinations[bestIdx];
					unsigned char *dstArea =
					    getNearestArea(di.pos);
					if (dstArea && dstArea != srcArea)
					{
						TeleportLink link;
						link.srcArea = srcArea;
						link.srcX = vTele.x; link.srcY = vTele.y;
						link.srcZ = vTele.z;
						link.dstArea = dstArea;
						link.dstX = di.pos.x;
						link.dstY = di.pos.y;
						link.dstZ = di.pos.z;
						m_teleportLinks.push_back(link);
					}
				}
			}
		}
	}

	if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
	{
		fprintf(stderr, "[RCDiag] scanGrid: %d unique nav areas cached,"
		        " connectOffset=0x%x, tele=%d\n",
		        m_nAreaCount, m_connectOffset, (int)m_teleportLinks.size());
		for (size_t i = 0; i < m_teleportLinks.size(); i++)
		{
			const TeleportLink &tl = m_teleportLinks[i];
			fprintf(stderr, "[RCDiag] teleLink[%d] src=(%.0f,%.0f,%.0f)"
			        " srcArea=%p dst=(%.0f,%.0f,%.0f) dstArea=%p\n",
			        (int)i,
			        tl.srcX, tl.srcY, tl.srcZ, tl.srcArea,
			        tl.dstX, tl.dstY, tl.dstZ, tl.dstArea);
		}
	}
}

bool CNavMeshAccessor::areaKnown(const void *p) const
{
	if (!p) return false;
	return std::binary_search(m_Areas.begin(), m_Areas.end(), (unsigned char *)p);
}

bool CNavMeshAccessor::isGoalFailed(const void *area) const
{
	auto it = m_failedGoals.find(area);
	if (it == m_failedGoals.end()) return false;
	return it->second > engine->Time();
}

void CNavMeshAccessor::markGoalFailed(const void *area)
{
	if (!area) return;
	m_failedGoals[(unsigned char *)area] = engine->Time() + randomFloat(30.0f, 90.0f);
}

// Probe candidate offsets for the m_connect[4] CUtlVectorUltraConservative
// array. Each element is a single pointer to a block { int count; NavConnect
// elems[] } where NavConnect = { CNavArea* area; float length } (8 bytes).
bool CNavMeshAccessor::detectConnectOffset()
{
	m_connectOffset = -1;
	if (m_Areas.size() < 2) return false;

	const int   kMinOff = 0x30, kMaxOff = 0xC0;
	const int   kSample = (int)m_Areas.size() < 64 ? (int)m_Areas.size() : 64;
	int         bestOff = -1, bestScore = 0;

	for (int off = kMinOff; off <= kMaxOff; off += 4)
	{
		int score = 0, bad = 0;
		for (int s = 0; s < kSample; s++)
		{
			unsigned char *area = m_Areas[(size_t)s * m_Areas.size() / kSample];
			int areaGood = 0, areaResolved = 0;
			for (int dir = 0; dir < 4; dir++)
			{
				unsigned char *pData = *(unsigned char **)(area + off + dir * 4);
				if (!pData || !ptrInArena(pData)) { bad++; continue; }
				// Reject self-pointers (connection data is heap-allocated separately)
				if (pData >= area && pData < area + 0x200) { bad++; continue; }
				// Reject pointers outside the observed area address range
				// to avoid dereferencing stale/unmapped memory from wrong offsets
				if ((uintptr_t)pData < m_areaMin || (uintptr_t)pData > m_areaMax) { bad++; continue; }
				int cnt = *(int *)pData;
				if (cnt < 0 || cnt > 16) { bad++; continue; }
				for (int i = 0; i < cnt; i++)
				{
					void *nb = *(void **)(pData + 4 + i * 8);
					if (areaKnown(nb)) areaResolved++;
					else { bad++; }
				}
				if (cnt > 0) areaGood++;
			}
			if (areaResolved > 0) score += areaResolved;
		}
		score -= bad;
		if (score > bestScore) { bestScore = score; bestOff = off; }
	}

	if (bestOff >= 0 && bestScore > kSample)
	{
		m_connectOffset = bestOff;
		return true;
	}
	return false;
}

unsigned char *CNavMeshAccessor::getAreaByIndex(int i) const
{
	if (i < 0 || i >= (int)m_Areas.size()) return nullptr;
	return m_Areas[i];
}

unsigned char *CNavMeshAccessor::getNearestArea(float x, float y, float z)
{
	if (!m_bReady && m_func) scanGrid();
	if (!m_bReady || m_Areas.empty()) return nullptr;

	unsigned char *best = nullptr;
	float bestD2        = 99999999.0f;
	for (size_t i = 0; i < m_Areas.size(); i++)
	{
		unsigned char *a = m_Areas[i];
		if (!a) continue;
		float *c = (float *)(a + NAV_M_CENTER);
		float dx = c[0] - x, dy = c[1] - y, dz = c[2] - z;
		float d2 = dx * dx + dy * dy + dz * dz * 32.0f;
		if (d2 < bestD2) { bestD2 = d2; best = a; }
	}
	return best;
}

// -- Per-navigator bad-connection cache ----------------------------------

void CNavMeshNavigator::expireBadConnections()
{
	float fNow = engine->Time();
	size_t j = 0;
	for (size_t i = 0; i < m_badConnections.size(); i++)
	{
		if (m_badConnections[i].expireTime > fNow)
		{
			if (j != i)
				m_badConnections[j] = m_badConnections[i];
			j++;
		}
	}
	m_badConnections.resize(j);
}

bool CNavMeshNavigator::isConnectionBad(const void *src, const void *dst) const
{
	if (!src || !dst) return false;
	uintptr_t s = (uintptr_t)src;
	uintptr_t d = (uintptr_t)dst;
	float fNow = engine->Time();
	for (size_t i = 0; i < m_badConnections.size(); i++)
	{
		// dst == 0 is a wildcard: blocks ALL destinations from this src
		if (m_badConnections[i].src == s &&
		    (m_badConnections[i].dst == d || m_badConnections[i].dst == 0))
			return m_badConnections[i].expireTime > fNow;
	}
	return false;
}

void CNavMeshNavigator::markConnectionBad(const void *src, const void *dst)
{
	if (!src || !dst) return;
	uintptr_t s = (uintptr_t)src;
	uintptr_t d = (uintptr_t)dst;
	float fNow = engine->Time();
	float fExpire = fNow + randomFloat(60.0f, 180.0f);
	for (size_t i = 0; i < m_badConnections.size(); i++)
	{
		if (m_badConnections[i].src == s && m_badConnections[i].dst == d)
		{
			m_badConnections[i].expireTime = fExpire;
			return;
		}
	}
	BadConnection bc;
	bc.src = s;
	bc.dst = d;
	bc.expireTime = fExpire;
	m_badConnections.push_back(bc);
}

// -- CNavMeshNavigator -------------------------------------------------------

void CNavMeshNavigator::setBot(CBot *pBot) { m_pBot = pBot; }

void CNavMeshNavigator::init()
{
	extern struct CNavMeshAccessor *g_pNavMeshAccessor;
	m_pAccessor = g_pNavMeshAccessor;

	m_vGoal         = Vector(0, 0, 0);
	m_vPreviousPoint = Vector(0, 0, 0);
	m_vDangerPoint  = Vector(0, 0, 0);
	m_bDangerPoint  = false;
	m_fGoalDistance = 0.0f;
}

// Forward declaration: used in A* ladder connection loop below
static inline bool navAreaPtrLooksValid(const void *a);

bool CNavMeshNavigator::workRoute(Vector vFrom, Vector vTo, bool *bFail,
                                   bool bRestart, bool bNoInterruptions,
                                   int iGoalWptID, int iConditions,
                                   int iDangerPoint)
{
	m_bWorkingRoute = false;
	m_route.clear();
	expireBadConnections();
	m_fStuckBestDist    = 0; m_fStuckBestTime = 0; m_iStuckSkips = 0;
	m_fJumpRelease      = 0;
	m_fScoutDJRelease   = 0;
	m_iStuckRecovery    = 0;
	m_fSteerExpiry      = 0;
	m_iConsecutiveHits  = 0;
	m_fLastTraceLog     = 0;
	m_fLastStuckLog     = 0;
	m_fLastIdleLog      = 0;
	m_vIdleCheckPos     = Vector(0,0,0);
	m_fIdleCheckTime    = 0;

	float fNow = engine->Time();
	if (m_fFailBackoffTime > fNow)
	{
		if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
			fprintf(stderr, "[RCDiag] workRouteFailBackoff name=%s bot=%d"
			    " remaining=%.1fs\n",
			    m_pBot ? m_pBot->getLogName() : "?",
			    m_pBot ? ENTINDEX(m_pBot->getEdict()) : 0,
			    m_fFailBackoffTime - fNow);
		if (bFail) *bFail = true; return true;
	}
	if (!m_route.empty() && (fNow - m_fLastRepathTime) < m_fMinRepathInterval)
		return true;

	if (!m_pAccessor || !m_pAccessor->ready())
	{
		if (bFail) *bFail = true;
		return true;
	}

	unsigned char *start = m_pAccessor->getNearestArea(vFrom);
	unsigned char *goal  = m_pAccessor->getNearestArea(vTo);
	if (!start || !goal) { if (bFail) *bFail = true; return true; }
	if (start == goal) {
		NavPathSegment seg = {};
		seg.area = goal;
		seg.pos  = areaGetCenter(goal);
		seg.type = NAV_SEG_ON_GROUND;
		m_route.push_back(seg);
		m_pGoalArea   = goal;
		m_pCurrentArea = start;
		m_bRouteFound = true; m_bWorkingRoute = true;
		m_fLastRepathTime = fNow;
		return true;
	}

	const int connOff = m_pAccessor->getConnectOffset();
	if (connOff < 0)
	{
		if (bFail) *bFail = true;
		if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
			fprintf(stderr, "[RCDiag] workRoute: connectOffset undetected, aborting route\n");
		return true;
	}

	Vector vGoalCenter = areaGetCenter(goal);
	Vector vStartCenter = areaGetCenter(start);

	auto heuristic = [&vGoalCenter](void *area) -> float {
		if (!area) return 0;
		float *c = (float *)((unsigned char *)area + NAV_M_CENTER);
		float dx = c[0] - vGoalCenter.x, dy = c[1] - vGoalCenter.y, dz = c[2] - vGoalCenter.z;
		return dx * dx + dy * dy + dz * dz;
	};

	std::map<void *, float> gScore;
	std::map<void *, void *> parent;
	std::map<void *, int>   parentHow;
	std::set<void *> closed;

	std::vector<std::pair<float, void *>> openList;

	gScore[start] = 0;
	openList.push_back({heuristic(start), start});

	auto heapCmp = [](const std::pair<float, void *> &a, const std::pair<float, void *> &b) {
		return a.first > b.first;
	};

	int maxIter = 6000;
	bool bFound = false;
	int nConnSkipped = 0;

	while (!openList.empty() && maxIter-- > 0)
	{
		std::pop_heap(openList.begin(), openList.end(), heapCmp);
		void *current = openList.back().second;
		openList.pop_back();

		if (current == goal) { bFound = true; break; }
		if (closed.count(current)) continue;
		closed.insert(current);

		Vector vCurCenter = areaGetCenter(current);

		for (int dir = 0; dir < 4; dir++)
		{
			char **ppData = (char **)((char *)current + connOff + dir * 4);
			char  *pData  = *ppData;
			if (!pData || !m_pAccessor->ptrInArena(pData)) continue;
			int connCount = *(int *)pData;
			if (connCount <= 0 || connCount > 16) continue;

			for (int c = 0; c < connCount; c++)
			{
				void *neighbor = *(void **)(pData + 4 + c * 8);
				if (!neighbor || !m_pAccessor->areaKnown(neighbor)) continue;
				if (isConnectionBad(current, neighbor)) {
					nConnSkipped++;
					if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool()
					    && current == start && (nConnSkipped & 31) == 0)
						fprintf(stderr, "[RCDiag] aStarBlockedStart name=%s bot=%d"
						    " cur=%p nxt=%p cz=%.0f nz=%.0f\n",
						    m_pBot ? m_pBot->getLogName() : "?",
						    m_pBot ? ENTINDEX(m_pBot->getEdict()) : 0,
						    current, neighbor, areaGetCenter(current).z,
						    areaGetCenter(neighbor).z);
					continue;
				}
				if (closed.count(neighbor)) continue;

				float *nCenter = (float *)((unsigned char *)neighbor + NAV_M_CENTER);
				float dx = nCenter[0] - vCurCenter.x;
				float dy = nCenter[1] - vCurCenter.y;
				float dz = nCenter[2] - vCurCenter.z;
				float stepCost = dx * dx + dy * dy + dz * dz;
				if (nCenter[2] < vCurCenter.z - 128.0f)
					stepCost += (vCurCenter.z - nCenter[2]) * (vCurCenter.z - nCenter[2]) * 3.0f;
				float tentativeG = gScore[current] + stepCost;

				auto gIt = gScore.find(neighbor);
				if (gIt != gScore.end() && tentativeG >= gIt->second) continue;

				parent[neighbor] = current;
				parentHow[neighbor] = dir;
				gScore[neighbor] = tentativeG;
				openList.push_back({tentativeG + heuristic(neighbor), neighbor});
				std::push_heap(openList.begin(), openList.end(), heapCmp);
			}
		}

		// --- Ladder connections (UP/DOWN) ---
		for (int ladDir = 0; ladDir < 2; ladDir++)
		{
			char **ppLad = (char **)((char *)current + NAV_M_LADDER + ladDir * 4);
			char  *pLad  = *ppLad;
			if (!pLad || !m_pAccessor->ptrInArena(pLad)) continue;
			int ladCount = *(int *)pLad;
			if (ladCount < 0 || ladCount > 4) continue;

			for (int l = 0; l < ladCount; l++)
			{
				void *pLadder = *(void **)(pLad + 4 + l * 4);
				if (!pLadder || !navAreaPtrLooksValid(pLadder)) continue;

				void *destArea = nullptr;
				if (ladDir == 0)
					destArea = *(void **)((char *)pLadder + 0x20);
				else
					destArea = *(void **)((char *)pLadder + 0x30);

				if (!destArea || !m_pAccessor->areaKnown(destArea)) continue;
				if (closed.count(destArea)) continue;

				Vector vN = areaGetCenter(destArea);
				float dx = vN.x - vCurCenter.x, dy = vN.y - vCurCenter.y, dz = vN.z - vCurCenter.z;
				float stepCost = dx*dx + dy*dy + dz*dz;
				stepCost *= 2.0f;

				float tentativeG = gScore[current] + stepCost;
				auto gIt = gScore.find(destArea);
				if (gIt != gScore.end() && tentativeG >= gIt->second) continue;

				parent[destArea] = current;
				parentHow[destArea] = -1;
				gScore[destArea] = tentativeG;
				openList.push_back({tentativeG + heuristic(destArea), destArea});
				std::push_heap(openList.begin(), openList.end(), heapCmp);
			}
		}

		// --- Elevator connections ---
		{
			char **ppElev = (char **)((char *)current + NAV_M_ELEV);
			char  *pElev  = *ppElev;
			if (pElev && m_pAccessor->ptrInArena(pElev))
			{
				int elevCount = *(int *)pElev;
				if (elevCount > 0 && elevCount <= 16)
				{
					for (int e = 0; e < elevCount; e++)
					{
						void *destArea = *(void **)(pElev + 4 + e * 8);
						if (!destArea || !m_pAccessor->areaKnown(destArea)) continue;
						if (closed.count(destArea)) continue;

						Vector vN = areaGetCenter(destArea);
						float dx = vN.x - vCurCenter.x, dy = vN.y - vCurCenter.y, dz = vN.z - vCurCenter.z;
						float stepCost = dx*dx + dy*dy + dz*dz;
						stepCost *= 4.0f;

						float tentativeG = gScore[current] + stepCost;
						auto gIt = gScore.find(destArea);
						if (gIt != gScore.end() && tentativeG >= gIt->second) continue;

						parent[destArea] = current;
						parentHow[destArea] = -1;
						gScore[destArea] = tentativeG;
						openList.push_back({tentativeG + heuristic(destArea), destArea});
						std::push_heap(openList.begin(), openList.end(), heapCmp);
					}
				}
			}
		}

		// --- Teleporter connections ---
		for (auto &tl : m_pAccessor->getTeleportLinks())
		{
			if (tl.srcArea != current || !tl.dstArea) continue;
			if (!m_pAccessor->areaKnown(tl.dstArea)) continue;
			if (isConnectionBad(tl.srcArea, tl.dstArea)) continue;
			if (closed.count(tl.dstArea)) continue;

			Vector vN = areaGetCenter(tl.dstArea);
			float dx = vN.x - vCurCenter.x;
			float dy = vN.y - vCurCenter.y;
			float dz = vN.z - vCurCenter.z;
			float stepCost = dx*dx + dy*dy + dz*dz;
			stepCost *= 0.1f;

			float tentativeG = gScore[current] + stepCost;
			auto gIt = gScore.find(tl.dstArea);
			if (gIt != gScore.end() && tentativeG >= gIt->second) continue;

			parent[tl.dstArea] = current;
			parentHow[tl.dstArea] = -1;
			gScore[tl.dstArea] = tentativeG;
			openList.push_back({tentativeG + heuristic(tl.dstArea), tl.dstArea});
			std::push_heap(openList.begin(), openList.end(), heapCmp);
		}
	}

	if (bFound)
	{
		std::vector<void *> path;
		int safety = 0;
		for (void *area = goal; area != nullptr; area = parent[area])
		{
			path.push_back(area);
			if (area == start || ++safety > 300) break;
		}
		for (int i = 0; i < (int)path.size() - 1; i++)
		{
			NavPathSegment seg = {};
			seg.area = path[i];
			seg.pos  = areaGetCenter(path[i]);
			seg.type = NAV_SEG_ON_GROUND;
			if (i + 1 < (int)path.size())
			{
				auto hit = parentHow.find(path[i]);
				seg.how = (hit != parentHow.end()) ? hit->second : -1;
			}
			else
				seg.how = -1;
			m_route.push_back(seg);
		}

		postProcessPath();

		optimizePath();

		// Populate the per-bot bad-connection cache from validateRoute()
		// so future A* runs avoid known-false edges.  But keep the route
		// — the bot still walks it and the runtime mechanisms (stuck-pop,
		// 60-hit drain, fell-off detection) handle individual false edges.
		// Rejecting the route entirely is too aggressive when the nav mesh
		// has pervasive false connections.
		validateRoute();
	}
	else {
		if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
			fprintf(stderr, "[RCDiag] aStarFailed name=%s bot=%d"
			    " start=%p startZ=%.0f goal=%p goalZ=%.0f"
			    " openSz=%d iter=%d maxIter=%d connSkip=%d\n",
			    m_pBot ? m_pBot->getLogName() : "?",
			    m_pBot ? ENTINDEX(m_pBot->getEdict()) : 0,
			    start, areaGetCenter(start).z,
			    goal, areaGetCenter(goal).z,
			    (int)openList.size(), 6000 - maxIter, 6000, nConnSkipped);
		m_pAccessor->markGoalFailed(goal); if (bFail) *bFail = true;
		m_fFailBackoffTime = fNow + fminf(0.005f * (vTo - vFrom).Length(), 2.0f);
		m_bWorkingRoute = false; return true; }

	m_pGoalArea   = goal;
	m_pCurrentArea = start;
	m_vGoalFloat  = vTo;
	m_bRouteFound = (m_route.size() > 0);
	m_bWorkingRoute = true;
	m_fLastRepathTime = fNow;

	if (!m_route.empty())
	{
		const NavPathSegment &first = m_route.back();
		m_vCurrentTarget = (first.portalHalfWidth > 0.0f)
		    ? first.portalCenter : first.pos;
	}

	if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
		fprintf(stderr, "[RCDiag] workRoute name=%s bot=%d found=%d routeSz=%d"
		    " (iter used=%d)\n",
		    m_pBot ? m_pBot->getLogName() : "?", m_pBot ? ENTINDEX(m_pBot->getEdict()) : 0,
		    (int)bFound, (int)m_route.size(), 6000 - maxIter);

	return true;
}

static inline bool navAreaPtrLooksValid(const void *a)
{
	uintptr_t p = (uintptr_t)a;
	return p >= 0x08000000 && p < 0xF0000000 && (p & 3) == 0;
}

// ---------- Path post-processing (portal centers, forward, curvature) ----------

void CNavMeshNavigator::postProcessPath()
{
	size_t sz = m_route.size();
	if (sz < 1) return;

	if (sz == 1)
	{
		m_route[0].forward = Vector(0, 0, 0);
		m_route[0].length = 0;
		m_route[0].distanceFromStart = 0;
		m_route[0].curvature = 0;
		return;
	}

	float distanceSoFar = 0.0f;

	for (int i = (int)sz - 1; i >= 1; i--)
	{
		NavPathSegment &from = m_route[i];
		NavPathSegment &to   = m_route[i - 1];

		int dir = to.how;
		if (dir >= 0 && dir < 4 && from.area && to.area)
		{
			computePortal(from.area, to.area, dir,
			              &to.portalCenter, &to.portalHalfWidth);
			to.pos = to.portalCenter;
		}

		from.forward = to.pos - from.pos;
		from.length = from.forward.NormalizeInPlace();
		from.distanceFromStart = distanceSoFar;
		distanceSoFar += from.length;
	}

	NavPathSegment &first = m_route[sz - 1];
	first.forward = Vector(0, 0, 0);
	first.length = 0;
	first.distanceFromStart = 0;
	first.curvature = 0;

	NavPathSegment &last = m_route[0];
	if (sz >= 2)
	{
		last.forward = m_route[1].forward;
		last.length = 0;
		last.distanceFromStart = distanceSoFar;
		last.curvature = 0;
	}

	for (size_t i = 0; i < sz; i++)
	{
		if (m_route[i].type != NAV_SEG_ON_GROUND || i >= sz - 1)
		{
			m_route[i].curvature = 0;
			continue;
		}
		if (i == 0) { m_route[i].curvature = 0; continue; }

		Vector2D incoming(m_route[i + 1].forward.x, m_route[i + 1].forward.y);
		Vector2D outgoing(m_route[i].forward.x, m_route[i].forward.y);
		float lenIn = incoming.NormalizeInPlace();
		float lenOut = outgoing.NormalizeInPlace();
		if (lenIn < 0.001f || lenOut < 0.001f)
			{ m_route[i].curvature = 0; continue; }

		float dot = incoming.Dot(outgoing);
		m_route[i].curvature = 0.5f * (1.0f - dot);

		Vector2D right(-incoming.y, incoming.x);
		if (outgoing.Dot(right) < 0.0f)
			m_route[i].curvature = -m_route[i].curvature;
	}
}

// Trace center-to-center for every consecutive area pair in the
// built route using AREA CENTERS, not portal positions.  Portal Z
// is clamped to the source area's Z, which masks cross-floor false
// connections (e.g. ledge z=192 connected to ground z=0).
// Returns true if the route is validated (all hops clear).
// When a false connection is detected, populates the per-bot
// bad-connection cache so future A* runs skip this edge.
bool CNavMeshNavigator::validateRoute()
{
	if (m_route.size() < 2) return true;
	CTraceFilterWorldAndPropsOnly filter;
	bool bAllClear = true;
	for (size_t i = 0; i < m_route.size() - 1; i++)
	{
		Vector from = areaGetCenter(m_route[i].area);
		Vector to   = areaGetCenter(m_route[i + 1].area);
		from.z += 24.0f; to.z += 24.0f;
		CBotGlobals::traceLine(from, to, MASK_PLAYERSOLID, &filter);
		trace_t *tr = CBotGlobals::getTraceResult();
		if (tr && tr->fraction < 1.0f && tr->plane.normal.z < 0.5f)
		{
			if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
				fprintf(stderr, "[RCDiag] invalidHop area=%p->%p"
				    " fraction=%.2f normalZ=%.2f\n",
				    m_route[i].area, m_route[i+1].area,
				    tr->fraction, tr->plane.normal.z);
			markConnectionBad(m_route[i+1].area, m_route[i].area);
			markConnectionBad(m_route[i].area, m_route[i+1].area);
			bAllClear = false;
		}
	}
	return bAllClear;
}

// ---------- Path collinear-node optimization ----------

void CNavMeshNavigator::optimizePath()
{
	size_t sz = m_route.size();
	if (sz < 3)
		return;

	// Iterative backward pass: scan triples from goal toward start.
	// If A→B→C is collinear (angle < 5°) and a direct A→C trace is clear,
	// remove B. Repeat until no more removals.
	const float kCosThreshold = cosf(5.0f * (M_PI / 180.0f)); // cos(5°)
	bool removed = true;
	while (removed && m_route.size() >= 3)
	{
		removed = false;
		sz = m_route.size();
		for (size_t i = sz - 1; i >= 2; i--)
		{
			if (i >= m_route.size()) continue; // index shifted by earlier removal

			NavPathSegment &segA = m_route[i];
			NavPathSegment &segB = m_route[i - 1];
			NavPathSegment &segC = m_route[i - 2];

			// Skip non-ground segments (ladders, elevators, etc.)
			if (segA.type != NAV_SEG_ON_GROUND ||
			    segB.type != NAV_SEG_ON_GROUND ||
			    segC.type != NAV_SEG_ON_GROUND)
				continue;

			// Compute the angle at B (between BA and BC vectors)
			Vector2D ba(segA.pos.x - segB.pos.x, segA.pos.y - segB.pos.y);
			Vector2D bc(segC.pos.x - segB.pos.x, segC.pos.y - segB.pos.y);
			float lenBA = ba.NormalizeInPlace();
			float lenBC = bc.NormalizeInPlace();
			if (lenBA < 0.01f || lenBC < 0.01f)
				continue;

			float dot = ba.Dot(bc);
			// dot > kCosThreshold means angle < 5° (collinear)
			if (dot <= kCosThreshold)
				continue;

			// Verify the A→C shortcut is walkable
			Vector posA = segA.pos;
			Vector posC = segC.pos;
			if (hasPotentialGap(posA, posC))
				continue;
			if (!isPotentiallyTraversable(posA, posC))
				continue;

			// Remove node B at index i-1
			m_route.erase(m_route.begin() + (i - 1));
			removed = true;
			// Don't continue walking indices backward — they shifted.
			// The outer while loop will re-scan from the end.
			break;
		}
	}

	// Recompute portal centers, forward vectors, curvature for modified path
	postProcessPath();
}

// -- Traversability checks ----------

bool CNavMeshNavigator::isPotentiallyTraversable(Vector from, Vector to)
{
	if (!m_pBot) return false;

	if ((to.z - from.z) > 72.0f)
	{
		Vector along = to - from;
		along.NormalizeInPlace();
		if (along.z > 0.7f) return false;
	}

	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;

	CBotGlobals::traceLine(from, to, MASK_PLAYERSOLID, &filter);
	tr = *CBotGlobals::getTraceResult();
	if (tr.fraction < 1.0f) return false;

	Vector fwd = to - from;
	float dist = fwd.NormalizeInPlace();
	Vector right(-fwd.y, fwd.x, 0);
	float halfWidth = 16.0f; // half of player standing hull width (32u)

	CBotGlobals::traceLine(from + right * halfWidth, to + right * halfWidth,
	                       MASK_PLAYERSOLID, &filter);
	tr = *CBotGlobals::getTraceResult();
	if (tr.fraction < 1.0f) return false;

	CBotGlobals::traceLine(from - right * halfWidth, to - right * halfWidth,
	                       MASK_PLAYERSOLID, &filter);
	tr = *CBotGlobals::getTraceResult();
	if (tr.fraction < 1.0f) return false;

	return true;
}

bool CNavMeshNavigator::hasPotentialGap(Vector from, Vector to, float *fraction)
{
	Vector fwd = to - from;
	float length = fwd.NormalizeInPlace();
	if (length < 1.0f) { if (fraction) *fraction = 1.0f; return false; }

	float step = 50.0f;
	Vector pos = from;
	CTraceFilterWorldAndPropsOnly filter;

	for (float t = 0; t < length; t += step)
	{
		Vector dropStart = pos + Vector(0, 0, 18.0f);
		Vector dropEnd   = pos + Vector(0, 0, -200.0f);

		CBotGlobals::traceLine(dropStart, dropEnd, MASK_PLAYERSOLID, &filter);
		trace_t *tr = CBotGlobals::getTraceResult();
		if (tr && tr->fraction >= 1.0f)
		{
			if (fraction) *fraction = t / (length + step);
			return true;
		}

		pos += fwd * step;
	}

	if (fraction) *fraction = 1.0f;
	return false;
}

bool CNavMeshNavigator::tryDirectPath(Vector vFrom, Vector vTo)
{
	float dist = (vTo - vFrom).Length();
	if (dist > 2000.0f || dist < 1.0f) return false;

	if (!isPotentiallyTraversable(vFrom, vTo)) return false;
	if (hasPotentialGap(vFrom, vTo)) return false;

	m_route.clear();

	void *startArea = m_pAccessor ? m_pAccessor->getNearestArea(vFrom) : nullptr;
	void *nearest    = m_pAccessor ? m_pAccessor->getNearestArea(vTo) : nullptr;

	NavPathSegment seg = {};
	seg.area = nearest;
	seg.pos  = vTo;
	seg.type = NAV_SEG_ON_GROUND;
	seg.portalHalfWidth = 0;
	seg.how = -1;
	seg.forward = vTo - vFrom;
	seg.length = seg.forward.NormalizeInPlace();
	seg.distanceFromStart = 0;
	seg.curvature = 0;
	m_route.push_back(seg);

	m_bRouteFound = true;
	m_bWorkingRoute = true;
	m_vCurrentTarget = vTo;
	m_pGoalArea = nearest;
	m_pCurrentArea = startArea;
	m_fLastRepathTime = engine->Time();

	if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
		fprintf(stderr, "[RCDiag] directPath name=%s bot=%d dist=%.0fu\n",
		    m_pBot ? m_pBot->getLogName() : "?",
		    m_pBot ? ENTINDEX(m_pBot->getEdict()) : 0, dist);

	return true;
}

// ---------- Proactive side-feeler obstacle avoidance ----------

Vector CNavMeshNavigator::avoidObstacles(const Vector &goalPos)
{
	if (!m_pBot) return goalPos;

	Vector vOrigin = m_pBot->getOrigin();
	Vector fwd = goalPos - vOrigin;
	fwd.z = 0;
	float distToGoal = fwd.NormalizeInPlace();

	if (distToGoal < 30.0f) return goalPos;

	Vector left(-fwd.y, fwd.x, 0);
	float range = 80.0f;
	float offset = 20.0f;

	CTraceFilterWorldAndPropsOnly filter;

	Vector leftFrom = vOrigin + left * offset;
	leftFrom.z += 18.0f;
	Vector leftTo = leftFrom + fwd * range;
	CBotGlobals::traceLine(leftFrom, leftTo, MASK_PLAYERSOLID, &filter);
	trace_t *trL = CBotGlobals::getTraceResult();
	float leftClear = (trL && trL->fraction < 1.0f) ? trL->fraction : 1.0f;

	Vector rightFrom = vOrigin - left * offset;
	rightFrom.z += 18.0f;
	Vector rightTo = rightFrom + fwd * range;
	CBotGlobals::traceLine(rightFrom, rightTo, MASK_PLAYERSOLID, &filter);
	trace_t *trR = CBotGlobals::getTraceResult();
	float rightClear = (trR && trR->fraction < 1.0f) ? trR->fraction : 1.0f;

	if (leftClear >= 1.0f && rightClear >= 1.0f) return goalPos;
	if (leftClear < 1.0f && rightClear < 1.0f)
	{
		// Both sides blocked: bias toward the clearer side instead of no-op
		float avoidAmount = (leftClear > rightClear) ? (1.0f - leftClear) : -(1.0f - rightClear);
		if (leftClear == rightClear)
			avoidAmount = (m_sStrafeDir > 0) ? (1.0f - leftClear) : -(1.0f - rightClear);
		Vector avoidDir = fwd + left * avoidAmount;
		avoidDir.NormalizeInPlace();
		return vOrigin + avoidDir * 100.0f;
	}

	float avoidAmount = (leftClear < 1.0f) ? (1.0f - leftClear) : -(1.0f - rightClear);
	Vector avoidDir = fwd + left * avoidAmount;
	avoidDir.NormalizeInPlace();

	return vOrigin + avoidDir * 100.0f;
}

Vector CNavMeshNavigator::getNextPoint()
{
	if (m_route.empty()) return Vector(0,0,0);
	void *a = m_route.back().area;
	if (!navAreaPtrLooksValid(a)) return Vector(0,0,0);
	return areaGetCenter(a);
}

bool CNavMeshNavigator::hasNextPoint() { return !m_route.empty(); }

void CNavMeshNavigator::updatePosition()
{
	if (!m_pBot) return;
	if (!m_pAccessor || !m_pAccessor->ready()) { freeMapMemory(); return; }

	// Phase 1: Escape mode (handles its own early-return if active)
	doEscapeMode(Vector(0,0,0)); // escape mode reads vBotOrigin internally via m_pBot->getOrigin()

	// Escape mode may have cleared its state but left us with no route
	if (m_route.empty()) return;

	Vector vBotOrigin = m_pBot->getOrigin();

	// Phase 2: Look-ahead goal skipping
	doLookAheadSkip(vBotOrigin);

	if (m_route.empty()) return;

	// Phase 3: Pop reached segments
	popReachedSegments(vBotOrigin);

	// Clear stale area pointer if route is now empty
	if (m_route.empty())
		m_pCurrentArea = nullptr;

	// Phase 4: Target selection + fall-off check + obstacle avoidance + trace-hit
	if (!m_route.empty())
	{
		const NavPathSegment &seg = m_route.back();
		Vector vRouteTarget = (seg.portalHalfWidth > 0.0f)
		    ? seg.portalCenter : seg.pos;
		m_vCurrentTarget = vRouteTarget;

		// --- Fall-off detection ---
		if (m_route.back().type == NAV_SEG_ON_GROUND)
		{
			float tooHigh = 72.0f;
			float tooLow  = 128.0f;
			float targetAreaZ = areaGetCenter(m_route.back().area).z;
			float fUpDelta   = m_vCurrentTarget.z - vBotOrigin.z;
			float fDownDelta = m_vCurrentTarget.z - vBotOrigin.z;
			if (fabsf(fDownDelta) < 16.0f)
				fDownDelta = targetAreaZ - vBotOrigin.z;
			if (fUpDelta > tooHigh)
			{
				Vector2D to(m_vCurrentTarget.x - vBotOrigin.x,
				             m_vCurrentTarget.y - vBotOrigin.y);
				if (to.Length() < 100.0f ||
				    (m_fStuckBestTime > 0 && engine->Time() - m_fStuckBestTime > 3.0f))
				{
					m_pAccessor->markGoalFailed(m_route.back().area);
					freeMapMemory();
					if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
						fprintf(stderr, "[RCDiag] fellOff name=%s bot=%d"
						    " goalZ=%.0f botZ=%.0f delta=%.0f"
						    " curArea=%p nxtArea=%p\n",
						    m_pBot->getLogName(),
						    ENTINDEX(m_pBot->getEdict()),
						    m_vCurrentTarget.z, vBotOrigin.z, fUpDelta,
						    m_pCurrentArea,
						    m_route.empty() ? nullptr : m_route.back().area);
					return;
				}
			}
			else if (fDownDelta < -tooLow)
			{
				if (m_fStuckBestTime > 0 && engine->Time() - m_fStuckBestTime > 3.0f)
				{
					m_pAccessor->markGoalFailed(m_route.back().area);
					freeMapMemory();
					if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
						fprintf(stderr, "[RCDiag] fellOff name=%s bot=%d"
						    " goalZ=%.0f botZ=%.0f delta=%.0f"
						    " curArea=%p nxtArea=%p\n",
						    m_pBot->getLogName(),
						    ENTINDEX(m_pBot->getEdict()),
						    m_vCurrentTarget.z, vBotOrigin.z, fDownDelta,
						    m_pCurrentArea,
						    m_route.empty() ? nullptr : m_route.back().area);
					return;
				}
			}
		}

		// --- Proactive obstacle avoidance (side feelers) ---
		// Capture both-feelers-blocked state for later corner-pivot decision
		bool bBothFeelersBlocked = false;
		{
			Vector fwdAdj = vRouteTarget - vBotOrigin;
			float distAdj = fwdAdj.NormalizeInPlace();
			if (distAdj > 30.0f)
			{
				Vector leftAdj(-fwdAdj.y, fwdAdj.x, 0);
				Vector lFrom = vBotOrigin + leftAdj * 20.0f;
				lFrom.z += 18.0f;
				Vector lTo = lFrom + fwdAdj * 80.0f;
				CTraceFilterWorldAndPropsOnly filtAdj;
				CBotGlobals::traceLine(lFrom, lTo, MASK_PLAYERSOLID, &filtAdj);
				trace_t *trAdjL = CBotGlobals::getTraceResult();
				bool leftBlocked = (trAdjL && trAdjL->fraction < 1.0f);

				Vector rFrom = vBotOrigin - leftAdj * 20.0f;
				rFrom.z += 18.0f;
				Vector rTo = rFrom + fwdAdj * 80.0f;
				CBotGlobals::traceLine(rFrom, rTo, MASK_PLAYERSOLID, &filtAdj);
				trace_t *trAdjR = CBotGlobals::getTraceResult();
				bool rightBlocked = (trAdjR && trAdjR->fraction < 1.0f);

				bBothFeelersBlocked = leftBlocked && rightBlocked;
			}
		}
		Vector vAdjusted = avoidObstacles(vRouteTarget);

		m_pBot->setMoveTo(vAdjusted);
		Vector vStuckRef = m_vCurrentTarget;

		// --- Preemptive obstacle detection ---
		{
			extern ConVar *rcbot_navmesh_jump_obstacle_range;
			float fJumpRange = rcbot_navmesh_jump_obstacle_range ? rcbot_navmesh_jump_obstacle_range->GetFloat() : 160.0f;

			float dxTrace = vRouteTarget.x - vBotOrigin.x;
			float dyTrace = vRouteTarget.y - vBotOrigin.y;
			float fDistTrace2D = sqrtf(dxTrace*dxTrace + dyTrace*dyTrace);
			if (fDistTrace2D > 20.0f)  // don't bother when target is right at our feet
			{
				float inv = 1.0f / fDistTrace2D;
				Vector vFwd(dxTrace * inv, dyTrace * inv, 0);

				// Trace from waist height (z+18) — above step height
				CTraceFilterWorldAndPropsOnly trFilter;
				Vector vTraceStart = vBotOrigin + Vector(0,0,18);
				Vector vTraceEnd   = vBotOrigin + vFwd * 200.0f;
				vTraceEnd.z = vBotOrigin.z + 24.0f;
				CBotGlobals::traceLine(vTraceStart, vTraceEnd,
				    MASK_PLAYERSOLID, &trFilter);
				trace_t *tr = CBotGlobals::getTraceResult();
				if (tr && tr->fraction < 1.0f)
				{
					float fHitDist = tr->fraction * 200.0f;
					if (fHitDist < fJumpRange)
					{
						// Try unified preemptive obstacle jump
						if (doObstacleJump(vBotOrigin, vFwd, true))
						{
							// Jump performed successfully
						}
						else
						{
							if (tr->plane.normal.z > 0.5f)
								m_iConsecutiveHits = 0;
							else
							{
								m_iConsecutiveHits++;
								if (m_iConsecutiveHits >= 60)
								{
									void *stuckDst = (!m_route.empty()) ? m_route.back().area : nullptr;
									float botZ = vBotOrigin.z;
									float dstZ = stuckDst ? areaGetCenter(stuckDst).z : botZ;
									if (stuckDst) {
										if (m_pCurrentArea)
											markConnectionBad(m_pCurrentArea, stuckDst);
									}
									if (stuckDst) {
										for (auto &tl : m_pAccessor->getTeleportLinks()) {
											if (tl.dstArea == stuckDst && tl.srcArea)
												markConnectionBad(tl.srcArea, tl.dstArea);
										}
									}
									m_route.clear();
									m_iConsecutiveHits = 0;
									m_fFailBackoffTime = 0;
									if (m_pGoalArea)
										m_pAccessor->markGoalFailed(m_pGoalArea);
									m_iDrainStreak++;
									if (m_iDrainStreak >= 3)
									{
										m_iDrainStreak = 0;
										void *curArea = m_pAccessor->getNearestArea(vBotOrigin);
										if (curArea)
											m_pAccessor->markGoalFailed(curArea);
										if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
											fprintf(stderr, "[RCDiag] drainStreak name=%s bot=%d"
											    " forcing escape curArea=%p\n",
											    m_pBot ? m_pBot->getLogName() : "?",
											    ENTINDEX(m_pBot->getEdict()), curArea);
										m_fFailBackoffTime = engine->Time() + 60.0f;
									}
									return;
								}
								if (m_route.empty()) return;

								// Check vertical clearance: low -> crouch
								Vector vHead = tr->endpos + Vector(0,0,48);
								CBotGlobals::traceLine(tr->endpos, vHead,
								    MASK_PLAYERSOLID, &trFilter);
								trace_t *tr2 = CBotGlobals::getTraceResult();
								if (tr2 && tr2->fraction < 1.0f
								    && tr2->fraction * 48.0f < 32.0f)
								{
									m_pBot->duck(true);
								}
								else
								{
									// On STAIRS / NO_JUMP: don't jump, just walk.
									int traceAttr = *(int *)((unsigned char *)m_route.back().area + NAV_M_ATTR);
									bool bNoJump = (traceAttr & (NAV_ATTR_STAIRS | NAV_ATTR_NO_JUMP)) != 0;
									if (!bNoJump && m_pCurrentArea)
									{
										int curAttr = *(int *)((unsigned char *)m_pCurrentArea + NAV_M_ATTR);
										bNoJump = (curAttr & (NAV_ATTR_STAIRS | NAV_ATTR_NO_JUMP)) != 0;
									}
									if (!bNoJump)
									{
										// 5-angle steering sweep
										float fBestClear = 0.0f;
										Vector vBestSteer(0,0,0);
										for (int s = 0; s < 5; s++)
										{
											float angSteer = (s - 2) * 0.52359878f;
											Vector vSteer(vFwd.x * cosf(angSteer) - vFwd.y * sinf(angSteer),
											              vFwd.x * sinf(angSteer) + vFwd.y * cosf(angSteer), 0);
											Vector vSEnd = vBotOrigin + vSteer * 150.0f;
											vSEnd.z = vBotOrigin.z + 24.0f;
											CBotGlobals::traceLine(vBotOrigin, vSEnd,
											    MASK_PLAYERSOLID, &trFilter);
											trace_t *trS = CBotGlobals::getTraceResult();
											float fClear = (trS && trS->fraction < 1.0f)
											    ? trS->fraction : 1.0f;
											if (fClear > fBestClear)
											{
												fBestClear = fClear;
												vBestSteer = vSteer;
											}
										}

										if (fBestClear < 0.33f)
										{
											for (int s = 0; s < 7; s++)
											{
												float angS2 = (s - 3) * 0.872664f;
												Vector vSt2(vFwd.x*cosf(angS2)-vFwd.y*sinf(angS2),
												           vFwd.x*sinf(angS2)+vFwd.y*cosf(angS2),0);
												Vector vS2End = vBotOrigin + vSt2 * 200.0f;
												vS2End.z = vBotOrigin.z + 24.0f;
												CBotGlobals::traceLine(vBotOrigin, vS2End,
												    MASK_PLAYERSOLID, &trFilter);
												trace_t *trS2 = CBotGlobals::getTraceResult();
												float fCl2 = (trS2 && trS2->fraction < 1.0f)
												    ? trS2->fraction : 1.0f;
												if (fCl2 > fBestClear)
												{
													fBestClear = fCl2;
													vBestSteer = vSt2;
												}
											}
										}

										if (fBestClear > 0.25f)
										{
											m_pBot->setMoveTo(vBotOrigin
											    + vBestSteer * (fBestClear * 150.0f));
											m_pBot->setSideMove(
											    (vBestSteer.y >= 0 ? 250.0f : -250.0f),
											    1.0f);
											m_fSteerExpiry = engine->Time() + 2.0f;
										}
								else
								{
									// All angles blocked.
									// Before jump/pop, try backward trace and corner pivot.

									// --- Backward trace: back out of tight corners ---
									{
										Vector vBackEnd = vBotOrigin - vFwd * 150.0f;
										vBackEnd.z = vBotOrigin.z + 24.0f;
										CBotGlobals::traceLine(vBotOrigin, vBackEnd, MASK_PLAYERSOLID, &trFilter);
										trace_t *trBack = CBotGlobals::getTraceResult();
										if (trBack && trBack->fraction >= 1.0f)
										{
											m_pBot->setMoveTo(vBotOrigin - vFwd * 150.0f);
											if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
												fprintf(stderr, "[RCDiag] cornerBack name=%s bot=%d pos=(%.0f,%.0f,%.0f)\n",
												    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
												    vBotOrigin.x, vBotOrigin.y, vBotOrigin.z);
											return;
										}
									}

									// --- Corner pivot: one-shot 16-direction scan ---
									if (bBothFeelersBlocked && m_fSteerExpiry <= engine->Time())
									{
										int bestDir = -1;
										float bestClear = 0.0f;
										for (int i = 0; i < 16; i++)
										{
											float ang = i * 0.392699f;
											Vector dir(cosf(ang), sinf(ang), 0);
											Vector right(-dir.y, dir.x, 0);
											Vector end = vBotOrigin + dir * 400.0f;
											end.z = vBotOrigin.z + 24.0f;
											Vector startPos = vBotOrigin + Vector(0,0,24);
											const float hullHalf = 16.0f;

											CBotGlobals::traceLine(startPos, end, MASK_PLAYERSOLID, &trFilter);
											trace_t *trF = CBotGlobals::getTraceResult();
											float cl = (trF && trF->fraction < 1.0f) ? trF->fraction : 1.0f;

											CBotGlobals::traceLine(startPos + right * hullHalf,
											    end + right * hullHalf, MASK_PLAYERSOLID, &trFilter);
											trF = CBotGlobals::getTraceResult();
											float cl2 = (trF && trF->fraction < 1.0f) ? trF->fraction : 1.0f;
											if (cl2 < cl) cl = cl2;

											CBotGlobals::traceLine(startPos - right * hullHalf,
											    end - right * hullHalf, MASK_PLAYERSOLID, &trFilter);
											trF = CBotGlobals::getTraceResult();
											cl2 = (trF && trF->fraction < 1.0f) ? trF->fraction : 1.0f;
											if (cl2 < cl) cl = cl2;

											if (cl > bestClear) { bestClear = cl; bestDir = i; }
										}

										if (bestDir >= 0 && bestClear > 0.5f)
										{
											float pivotAng = bestDir * 0.392699f;
											Vector pivotDir(cosf(pivotAng), sinf(pivotAng), 0);
											m_pBot->setMoveTo(vBotOrigin + pivotDir * 400.0f);
											m_pBot->setSideMove((pivotDir.y >= 0 ? 250.0f : -250.0f), 1.0f);
											m_fSteerExpiry = engine->Time() + 2.0f;
											if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
												fprintf(stderr, "[RCDiag] cornerPivot name=%s bot=%d dir=%d clear=%.2f\n",
												    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
												    bestDir, bestClear);
											return;
										}
									}

									// Fall through to existing jump/pop logic
									// Check stair/no-jump flags on current area before attempting jump.
										bool bNoJump = false;
										int traceAttr = *(int *)((unsigned char *)m_route.back().area + NAV_M_ATTR);
										bNoJump = (traceAttr & (NAV_ATTR_STAIRS | NAV_ATTR_NO_JUMP)) != 0;
										if (!bNoJump && m_pCurrentArea)
										{
											int curAttr = *(int *)((unsigned char *)m_pCurrentArea + NAV_M_ATTR);
											bNoJump = (curAttr & (NAV_ATTR_STAIRS | NAV_ATTR_NO_JUMP)) != 0;
										}
										if (!bNoJump)
										{
											Vector vJumpCheck = vBotOrigin + Vector(0, 0, 72);
											CBotGlobals::traceLine(vBotOrigin, vJumpCheck,
											    MASK_PLAYERSOLID, &trFilter);
											trace_t *trJ = CBotGlobals::getTraceResult();
											if (trJ && trJ->fraction >= 1.0f)
											{
												m_pBot->tapButton(IN_JUMP);
												m_fJumpRelease = engine->Time() + 0.25f;
											}
											else
											{
												// False connection: pop + blacklist.
												Vector vCurCtr = m_route.back().pos;
												vCurCtr.z += 24.0f;
												Vector vNxtCtr;
												if (m_route.size() >= 2)
												{
													vNxtCtr = m_route[m_route.size() - 2].pos;
													vNxtCtr.z += 24.0f;
												}
												else
												{
													vNxtCtr = m_vCurrentTarget;
													vNxtCtr.z += 24.0f;
												}
												CBotGlobals::traceLine(vCurCtr, vNxtCtr,
												    MASK_PLAYERSOLID, &trFilter);
												trace_t *trC2C = CBotGlobals::getTraceResult();
												if (trC2C && trC2C->fraction < 1.0f)
												{
													void *popped = m_route.back().area;
													if (popped && m_pCurrentArea)
														markConnectionBad(m_pCurrentArea, popped);
													m_route.pop_back();
													if (popped)
														m_pAccessor->markGoalFailed(popped);
												}
												if (m_route.empty() && m_pGoalArea)
													m_pAccessor->markGoalFailed(m_pGoalArea);
											}
										}
										else
										{
											// Stairs/no-jump area: pop + blacklist, don't jump.
											void *popped = m_route.back().area;
											if (popped && m_pCurrentArea)
												markConnectionBad(m_pCurrentArea, popped);
											m_route.pop_back();
											if (popped)
												m_pAccessor->markGoalFailed(popped);
											if (m_route.empty() && m_pGoalArea)
												m_pAccessor->markGoalFailed(m_pGoalArea);
										}
									}
									}
								}
							}
						}
					}
				}
			}
		}

		// Phase 5: Nav-area attribute handling
		applyNavAttributes(vBotOrigin);

		// Phase 6: Stuck detection
		detectStuck(vBotOrigin, vStuckRef);

		// --- Occasional LOOK_AROUND glances ---
		if (m_fLookAroundTime <= engine->Time())
		{
			m_pBot->setLookAtTask(LOOK_AROUND, 0.6f);
			m_fLookAroundTime = engine->Time() + randomFloat(3.0f, 5.0f);
		}

		// Phase 7: Idle-position heartbeat
		heartbeatIdleLog(vBotOrigin);
	}
	else
		m_pCurrentArea = nullptr;

	// Debug update log
	if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2) {
		static int nUpd = 0;
		if ((++nUpd & 31) == 0) {
			Vector vCenter(0, 0, 0);
			if (!m_route.empty())
				vCenter = m_route.back().pos;
			fprintf(stderr,
			        "[RCDiag] updatePos name=%s bot=%d routeSz=%d center=(%.0f,%.0f,%.0f)"
			        " target=(%.0f,%.0f,%.0f) dist=%.0f\n",
			        m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
			        (int)m_route.size(), vCenter.x, vCenter.y, vCenter.z,
			        m_vCurrentTarget.x, m_vCurrentTarget.y, m_vCurrentTarget.z,
			        m_pBot->distanceFrom(vCenter));
		}
	}
}

// ---------- Preemptive obstacle-jump helper ----------

bool CNavMeshNavigator::doObstacleJump(const Vector &vBotOrigin, const Vector &vFwdDirection, bool bAllowEnhancedJumps)
{
	extern ConVar *rcbot_navmesh_jump_obstacle_min;
	extern ConVar *rcbot_navmesh_jump_obstacle_max;

	if (!m_pBot) return false;
	if (m_fJumpRelease > engine->Time()) return false;

	// Trace forward from waist height (z+18) — above step height
	CTraceFilterWorldAndPropsOnly trFilter;
	Vector vTraceStart = vBotOrigin + Vector(0, 0, 18.0f);
	Vector vTraceEnd   = vBotOrigin + vFwdDirection * 200.0f;
	vTraceEnd.z = vBotOrigin.z + 24.0f;

	CBotGlobals::traceLine(vTraceStart, vTraceEnd, MASK_PLAYERSOLID, &trFilter);
	trace_t *tr = CBotGlobals::getTraceResult();
	if (!tr || tr->fraction >= 1.0f) return false;

	// Snapshot the hit BEFORE any further traces (traceLine overwrites the static result)
	Vector vHitPos  = tr->endpos;
	float  fHitFrac = tr->fraction;
	float  fHitNormZ = tr->plane.normal.z;

	// Only react to vertical faces, not slopes
	if (fHitNormZ >= 0.5f) return false;

	// Down-trace from hit point to measure obstacle height
	Vector vDown = vHitPos;
	vDown.z -= 72.0f;
	CBotGlobals::traceLine(vHitPos, vDown, MASK_PLAYERSOLID, &trFilter);
	trace_t *trG = CBotGlobals::getTraceResult();
	if (!trG || trG->fraction >= 1.0f) return false;

	float obstacleHeight = vHitPos.z - trG->endpos.z;
	float fMin = rcbot_navmesh_jump_obstacle_min ? rcbot_navmesh_jump_obstacle_min->GetFloat() : 18.0f;
	float fMax = rcbot_navmesh_jump_obstacle_max ? rcbot_navmesh_jump_obstacle_max->GetFloat() : 72.0f;
	if (obstacleHeight < fMin || obstacleHeight > fMax) return false;

	// Top-surface check: trace forward from just above the obstacle top.
	// If there's walkable ground behind the obstacle, it's a jumpable ledge/crate.
	// If not (e.g. a full-height wall), skip.
	{
		Vector vTopStart = vHitPos;
		vTopStart.z = trG->endpos.z + obstacleHeight + 4.0f; // just above the obstacle
		Vector vTopEnd = vTopStart + vFwdDirection * 40.0f;  // reach a bit past it
		vTopEnd.z -= 40.0f; // angle down to find the top surface
		CBotGlobals::traceLine(vTopStart, vTopEnd, MASK_PLAYERSOLID, &trFilter);
		trace_t *trTop = CBotGlobals::getTraceResult();
		if (!trTop || trTop->fraction >= 1.0f || trTop->plane.normal.z < 0.7f)
			return false; // no walkable top surface behind the obstacle
	}

	// Wide-wall check (±48u lateral traces)
	Vector rightSide(-vFwdDirection.y, vFwdDirection.x, 0);
	Vector fwd5 = vHitPos + vFwdDirection * 5.0f;
	CBotGlobals::traceLine(vHitPos + rightSide * 48.0f,
	    fwd5 + rightSide * 48.0f, MASK_PLAYERSOLID, &trFilter);
	bool leftBlocked = (CBotGlobals::getTraceResult()
	    && CBotGlobals::getTraceResult()->fraction < 1.0f);
	CBotGlobals::traceLine(vHitPos - rightSide * 48.0f,
	    fwd5 - rightSide * 48.0f, MASK_PLAYERSOLID, &trFilter);
	bool rightBlocked = (CBotGlobals::getTraceResult()
	    && CBotGlobals::getTraceResult()->fraction < 1.0f);
	if (leftBlocked && rightBlocked) return false;

	// Vertical clearance check
	Vector vJumpCheck = vBotOrigin + Vector(0, 0, 72);
	CBotGlobals::traceLine(vBotOrigin, vJumpCheck, MASK_PLAYERSOLID, &trFilter);
	trace_t *trJ = CBotGlobals::getTraceResult();
	if (!trJ || trJ->fraction < 1.0f) return false;

	// Perform the jump with class-specific logic
	if (m_pBot->isTF2())
	{
		CBotTF2 *pTF2 = (CBotTF2 *)m_pBot;
		int tfClass = pTF2->getClass();

		if (tfClass == TF_CLASS_SCOUT)
		{
			pTF2->tapButton(IN_JUMP);
			m_fJumpRelease = engine->Time() + 0.8f;
			if (m_fScoutDJRelease <= engine->Time())
				m_fScoutDJRelease = engine->Time() + 0.28f;
		}
		else if (bAllowEnhancedJumps && obstacleHeight > 48.0f)
		{
			if (tfClass == TF_CLASS_SOLDIER && pTF2->getHealthPercent() > 0.5f)
			{
				pTF2->jump();
				m_fJumpRelease = engine->Time() + 1.2f;
			}
			else if (tfClass == TF_CLASS_DEMOMAN && rcbot_demo_jump && rcbot_demo_jump->GetInt())
			{
				pTF2->jump();
				m_fJumpRelease = engine->Time() + 1.2f;
			}
			else
			{
				pTF2->tapButton(IN_JUMP);
				m_fJumpRelease = engine->Time() + 1.0f;
			}
		}
		else
		{
			pTF2->tapButton(IN_JUMP);
			m_fJumpRelease = engine->Time() + 1.0f;
		}
	}
	else
	{
		m_pBot->tapButton(IN_JUMP);
		m_fJumpRelease = engine->Time() + 1.0f;
	}

	if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetInt() >= 2)
	{
		fprintf(stderr, "[RCDiag] obstJump name=%s bot=%d"
		    " pos=(%.0f,%.0f,%.0f) h=%.0f resp=jump\n",
		    m_pBot->getLogName(),
		    ENTINDEX(m_pBot->getEdict()),
		    vBotOrigin.x, vBotOrigin.y, vBotOrigin.z,
		    obstacleHeight);
	}

	return true;
}

// ---------- Extracted updatePosition phases ----------

// Returns true if the caller (updatePosition) should return immediately.
void CNavMeshNavigator::doEscapeMode(const Vector &vBotOrigin)
{
	(void)vBotOrigin; // unused parameter — escape mode reads vBotOrigin from m_pBot->getOrigin() internally

	// Escape mode: when the A* can't find any valid path (route empty
	// + failBackoff active), use trace-based exploration to navigate
	// without nav mesh coverage.
	if (!m_route.empty() || m_fFailBackoffTime <= engine->Time())
	{
		if (m_iEscapeMode > 0)
			m_iEscapeMode = 0;
		return;
	}

	Vector vBotNow = m_pBot->getOrigin();

	if (m_iEscapeMode == 0)
	{
		m_iEscapeMode = 1;
		m_fEscapeStartTime = engine->Time();
		m_tnLastScanTime = 0;
		m_tnStuckCount = 0;
		m_vTnExploreStart = vBotNow;
		m_fTnExploreRadius = 0;
		if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
			fprintf(stderr, "[RCDiag] escapeStart name=%s bot=%d"
			    " pos=(%.0f,%.0f,%.0f)\n",
			    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
			    vBotNow.x, vBotNow.y, vBotNow.z);
	}

	if (m_iEscapeMode >= 1)
	{
		// --- Falling check ---
		if (m_iEscapeMode == 2)
		{
			m_pBot->setMoveTo(m_vEscapeTarget);
			if (NavMeshUtil::IsOnWalkableGround(vBotNow))
			{
				m_iEscapeMode = 0;
				m_fFailBackoffTime = 0;
			}
			if (engine->Time() - m_fEscapeStartTime > 10.0f)
				m_iEscapeMode = 0;
			return;
		}

		float fNow = engine->Time();

		// --- Jump detection (also runs in escape mode) ---
		// Check for jumppable obstacle in current direction
		Vector vFwdDir = m_vEscapeTarget - vBotNow;
		vFwdDir.z = 0;
		float fDistToTarget = vFwdDir.NormalizeInPlace();
		if (fDistToTarget > 80.0f && fDistToTarget < 400.0f) {
			doObstacleJump(vBotNow, vFwdDir, false);
		}

		// --- Nav mesh recovery check (every 2s) ---
		if (fNow - m_tnLastScanTime >= 2.0f)
		{
			void *nearArea = m_pAccessor->getNearestArea(vBotNow);
			float nearDist = 99999.0f;
			if (nearArea) {
				float *nc = (float *)((unsigned char *)nearArea + NAV_M_CENTER);
				nearDist = (Vector(nc[0],nc[1],nc[2]) - vBotNow).Length();
			}
			if (nearDist < 800.0f) {
				m_iEscapeMode = 0;
				m_fFailBackoffTime = 0;
				m_pBot->setMoveTo(m_vEscapeTarget);
				return;
			}

			// --- 16-direction fan scan with 3-trace hull check ---
			int bestDir = -1;
			float bestScore = 0.0f;
			CTraceFilterWorldAndPropsOnly tnFilter;
			for (int i = 0; i < 16; i++)
			{
				float ang = i * 0.392699f;
				Vector dir(cosf(ang), sinf(ang), 0);
				Vector right(-dir.y, dir.x, 0);
				Vector end = vBotNow + dir * 400.0f;
				end.z = vBotNow.z + 24.0f;
				Vector startPos = vBotNow + Vector(0,0,24);
				const float hullHalf = 16.0f;

				CBotGlobals::traceLine(startPos, end, MASK_PLAYERSOLID, &tnFilter);
				trace_t *tr = CBotGlobals::getTraceResult();
				float clear = (tr && tr->fraction < 1.0f) ? tr->fraction : 1.0f;

				CBotGlobals::traceLine(startPos + right * hullHalf,
				    end + right * hullHalf, MASK_PLAYERSOLID, &tnFilter);
				tr = CBotGlobals::getTraceResult();
				float cl = (tr && tr->fraction < 1.0f) ? tr->fraction : 1.0f;
				if (cl < clear) clear = cl;

				CBotGlobals::traceLine(startPos - right * hullHalf,
				    end - right * hullHalf, MASK_PLAYERSOLID, &tnFilter);
				tr = CBotGlobals::getTraceResult();
				cl = (tr && tr->fraction < 1.0f) ? tr->fraction : 1.0f;
				if (cl < clear) clear = cl;

				// Floor check at end
				Vector floorCheck = vBotNow + dir * clear * 400.0f;
				floorCheck.z += 18.0f;
				Vector floorDown = floorCheck - Vector(0,0,200.0f);
				CBotGlobals::traceLine(floorCheck, floorDown,
				    MASK_PLAYERSOLID, &tnFilter);
				tr = CBotGlobals::getTraceResult();
				bool hasFloor = (tr && tr->fraction < 1.0f
				    && tr->plane.normal.z > 0.5f);

				float score = clear;
				if (hasFloor) score += 2.0f;
				if (score > bestScore || (score == bestScore && randomInt(0, 1) == 0)) { bestScore = score; bestDir = i; }
				m_tnClearance[i] = clear;
				m_tnHasFloor[i] = hasFloor;
			}

			if (bestDir >= 0)
			{
				// Wall-follow: if blocked forward, try 90deg left/right
				if (m_tnClearance[bestDir] < 0.2f) {
					int leftDir = (bestDir + 4) & 15;
					int rightDir = (bestDir - 4 + 16) & 15;
					if (m_tnClearance[leftDir] > 0.5f)
						bestDir = leftDir;
					else if (m_tnClearance[rightDir] > 0.5f)
						bestDir = rightDir;
				}

				float targetAng = bestDir * 0.392699f;
				m_vEscapeTarget = vBotNow
				    + Vector(cosf(targetAng), sinf(targetAng), 0) * 800.0f;
			}
			m_tnLastScanTime = fNow;

			if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
				fprintf(stderr, "[RCDiag] escapeTrace name=%s bot=%d"
				    " bestDir=%d score=%.2f target=(%.0f,%.0f)\n",
				    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
				    bestDir, bestScore,
				    m_vEscapeTarget.x, m_vEscapeTarget.y);
		}

		// --- Progress check ---
		float fPhaseTime = fNow - m_fEscapeStartTime;
		float distFromPhaseStart = (vBotNow - m_vTnExploreStart).Length();
		if (distFromPhaseStart > m_fTnExploreRadius)
			m_fTnExploreRadius = distFromPhaseStart;
		if (fPhaseTime > 8.0f && distFromPhaseStart < 100.0f)
		{
			m_tnStuckCount++;
			m_tnLastScanTime = 0; // force new scan
			if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
				fprintf(stderr, "[RCDiag] escapeStuck name=%s bot=%d"
				    " stuck=%d\n",
				    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
				    m_tnStuckCount);
		}

		m_pBot->setMoveTo(m_vEscapeTarget);

		if (fNow - m_fEscapeStartTime > 60.0f)
		{
			m_iEscapeMode = 0;
			if (rcbot_debug_navmesh && rcbot_debug_navmesh->GetBool())
				fprintf(stderr, "[RCDiag] escapeTimeout name=%s bot=%d"
				    " dur=%.0fs\n",
				    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
				    fNow - m_fEscapeStartTime);
		}
	}
}

void CNavMeshNavigator::doLookAheadSkip(const Vector &vBotOrigin)
{
	if (m_fMinLookAheadRange <= 0 || m_route.size() < 2)
		return;

	NavPathSegment &goal = m_route.back();
	NavPathSegment &next = m_route[m_route.size() - 2];

	if (goal.type == NAV_SEG_ON_GROUND &&
	    next.type == NAV_SEG_ON_GROUND &&
	    fabsf(next.curvature) < 0.5f)
	{
		float distToGoal = (goal.pos - vBotOrigin).Length();
		if (distToGoal < m_fMinLookAheadRange)
		{
			if (isPotentiallyTraversable(vBotOrigin, next.pos) &&
			    !hasPotentialGap(vBotOrigin, next.pos) &&
			    (next.pos.z - vBotOrigin.z) < 18.0f)
			{
				m_route.pop_back();
				m_vCurrentTarget = m_route.back().portalHalfWidth > 0.0f
				    ? m_route.back().portalCenter : m_route.back().pos;
				m_pCurrentArea   = m_route.back().area;
				m_fStuckBestDist = (m_route.back().pos - vBotOrigin).Length2D();
				m_fStuckBestTime = engine->Time();
				m_iStuckRecovery = 0;
			}
		}
	}
}

void CNavMeshNavigator::popReachedSegments(const Vector &vBotOrigin)
{
	int safety = 0;
	while (!m_route.empty() && safety < 50)
	{
		NavPathSegment &segBack = m_route.back();
		Vector cTarget = segBack.pos;

		if (m_route.size() >= 2)
		{
			Vector ctrBack = areaGetCenter(segBack.area);
			Vector ctrNext = areaGetCenter(m_route[m_route.size() - 2].area);
			float  fDistAB = (ctrBack - ctrNext).Length2D();
			float  fPopRad = fDistAB > 100.0f ? fDistAB * 0.5f : 100.0f;
			float  fDist2D = (cTarget - vBotOrigin).Length2D();
			if (fDist2D >= fPopRad)
				break;
		}

		m_vPreviousPoint = m_vCurrentTarget;
		m_pCurrentArea   = m_route.back().area;
		m_route.pop_back();

		m_vLookJitter = Vector(randomFloat(-128, 128), randomFloat(-128, 128), 0);
		m_fLookJitterTime = engine->Time() + 0.5f;

		m_fStuckBestDist  = 0;
		m_fStuckBestTime  = engine->Time();
		m_fJumpRelease    = 0;
		m_fScoutDJRelease = 0;
		m_iConsecutiveHits = 0;
		m_fSteerExpiry     = 0;
		safety++;
	}
}

void CNavMeshNavigator::applyNavAttributes(const Vector &vBotOrigin)
{
	int attr = *(int *)((unsigned char *)m_route.back().area + NAV_M_ATTR);

	if (attr & NAV_ATTR_CROUCH)
		m_pBot->duck(true);

	if (attr & NAV_ATTR_RUN)
		m_pBot->updateCondition(CONDITION_RUN);

	if (attr & NAV_ATTR_NO_JUMP)
	{
		m_fJumpRelease = 0;
		m_fScoutDJRelease = 0;
	}

	if (attr & NAV_ATTR_STAIRS)
	{
		m_fJumpRelease = 0;
		m_fScoutDJRelease = 0;
	}

	if (attr & NAV_ATTR_JUMP && m_fJumpRelease <= engine->Time())
	{
		float dxJ = m_vCurrentTarget.x - vBotOrigin.x;
		float dyJ = m_vCurrentTarget.y - vBotOrigin.y;
		float fDistJ = sqrtf(dxJ*dxJ + dyJ*dyJ);
		if (fDistJ < 200.0f)
		{
			float zDelta = m_vCurrentTarget.z - m_pBot->getOrigin().z;

			if (m_pBot->isTF2())
			{
				CBotTF2 *pTF2 = (CBotTF2 *)m_pBot;
				int tfClass = pTF2->getClass();

				if (tfClass == TF_CLASS_SOLDIER && zDelta > 48.0f
				    && pTF2->getHealthPercent() > 0.5f)
				{
					pTF2->jump();
					m_fJumpRelease = engine->Time() + 1.2f;
				}
				else if (tfClass == TF_CLASS_DEMOMAN && zDelta > 48.0f
				         && rcbot_demo_jump && rcbot_demo_jump->GetInt())
				{
					pTF2->jump();
					m_fJumpRelease = engine->Time() + 1.2f;
				}
				else if (tfClass == TF_CLASS_SCOUT)
				{
					pTF2->jump();
					m_fJumpRelease = engine->Time() + 0.35f;
					if (m_fScoutDJRelease <= engine->Time())
						m_fScoutDJRelease = engine->Time() + 0.28f;
				}
				else
				{
					pTF2->tapButton(IN_JUMP);
					m_fJumpRelease = engine->Time() + 0.25f;
				}
			}
			else
			{
				m_pBot->tapButton(IN_JUMP);
				m_fJumpRelease = engine->Time() + 0.25f;
			}
		}
	}

	if (m_fScoutDJRelease && m_fScoutDJRelease <= engine->Time()
	    && m_fScoutDJRelease + 0.5f > engine->Time())
	{
		m_pBot->jump();
		m_fScoutDJRelease = engine->Time() + 3600.0f;
	}
}

void CNavMeshNavigator::detectStuck(const Vector &vBotOrigin, const Vector &vStuckRef)
{
	float dx2 = vStuckRef.x - vBotOrigin.x;
	float dy2 = vStuckRef.y - vBotOrigin.y;
	float fDist2D = sqrtf(dx2 * dx2 + dy2 * dy2);
	float fNow     = engine->Time();

	if (m_fStuckBestDist == 0 || fDist2D < m_fStuckBestDist - 25.0f)
	{
		m_fStuckBestDist = fDist2D;
		m_fStuckBestTime = fNow;
	}
	else if (fNow - m_fStuckBestTime > 3.0f)
	{
		m_fStuckBestDist = 0;
		m_fStuckBestTime = fNow;

		if (!m_route.empty())
		{
			void *popped = m_route.back().area;
			void *prevArea = (m_pCurrentArea ? m_pCurrentArea : popped);
			m_route.pop_back();
			if (popped) {
				markConnectionBad(prevArea, popped);
				for (auto &tl : m_pAccessor->getTeleportLinks()) {
					if (tl.dstArea == popped && tl.srcArea)
						markConnectionBad(tl.srcArea, tl.dstArea);
				}
				m_pAccessor->markGoalFailed(popped);
			}
			if (m_route.empty() && m_pGoalArea)
				m_pAccessor->markGoalFailed(m_pGoalArea);
		}
	}
}

void CNavMeshNavigator::heartbeatIdleLog(const Vector &vBotOrigin)
{
	if (!rcbot_debug_navmesh || rcbot_debug_navmesh->GetInt() < 2)
		return;
	if (m_fStuckBestTime <= 0 || m_fLastIdleLog > engine->Time())
		return;

	float fNow = engine->Time();

	if (m_fIdleCheckTime == 0)
	{
		m_vIdleCheckPos  = vBotOrigin;
		m_fIdleCheckTime = fNow;
	}
	else if (fNow - m_fIdleCheckTime > 4.0f)
	{
		float dxI = vBotOrigin.x - m_vIdleCheckPos.x;
		float dyI = vBotOrigin.y - m_vIdleCheckPos.y;
		if (sqrtf(dxI*dxI + dyI*dyI) < 80.0f)
		{
			m_fLastIdleLog = engine->Time() + 4.0f;
			fprintf(stderr, "[RCDiag] idle name=%s bot=%d pos=(%.0f,%.0f,%.0f)"
			    " target=(%.0f,%.0f,%.0f) dist=%.0fu sz=%d\n",
			    m_pBot->getLogName(), ENTINDEX(m_pBot->getEdict()),
			    vBotOrigin.x,vBotOrigin.y,vBotOrigin.z,
			    m_vCurrentTarget.x,m_vCurrentTarget.y,m_vCurrentTarget.z,
			    sqrtf(dxI*dxI + dyI*dyI), (int)m_route.size());
		}
		m_vIdleCheckPos  = vBotOrigin;
		m_fIdleCheckTime = fNow;
	}
}

void CNavMeshNavigator::checkFallOff(const Vector &vBotOrigin) {}
void CNavMeshNavigator::handleObstacleHit(const Vector &vBotOrigin, const Vector &vFwd, const Vector &vRouteTarget)
{
	(void)vBotOrigin; (void)vFwd; (void)vRouteTarget;
}

// ---------- End extracted updatePosition phases ----------


Vector CNavMeshNavigator::getRandomAreaCenter(Vector vFrom, float minDist, float maxDist, bool bTrace)
{
	if (!m_pAccessor || !m_pAccessor->ready()) return Vector(0,0,0);
	int n = m_pAccessor->getAreaCount();
	if (n <= 0) return Vector(0,0,0);

	Vector vFallback(0, 0, 0);

	for (int t = 0; t < 200; t++)
	{
		int idx = randomInt(0, n - 1);
		unsigned char *a = m_pAccessor->getAreaByIndex(idx);
		if (!a) continue;
		if (m_pAccessor->isGoalFailed(a)) continue;
		float *c = (float *)(a + NAV_M_CENTER);
		Vector v(c[0], c[1], c[2]);
		float fDist = (v - vFrom).Length();

		if (fDist < minDist || fDist > maxDist) continue;

		if (bTrace)
		{
			CTraceFilterWorldAndPropsOnly filter;
			CBotGlobals::traceLine(vFrom, v, MASK_PLAYERSOLID, &filter);
			trace_t *tr = CBotGlobals::getTraceResult();
			if (tr && tr->fraction < 1.0f) { vFallback = v; continue; }
		}

		return v;
	}

	if (vFallback.Length() < 0.1f)
	{
		int idx = randomInt(0, n - 1);
		unsigned char *a = m_pAccessor->getAreaByIndex(idx);
		if (a)
		{
			float *c = (float *)(a + NAV_M_CENTER);
			vFallback = Vector(c[0], c[1], c[2]);
		}
	}
	return vFallback;
}

void CNavMeshNavigator::freeMapMemory()
{
	m_route.clear();
	m_vCurrentTarget = Vector(0, 0, 0);
	m_pGoalArea = nullptr; m_pCurrentArea = nullptr;
	m_bRouteFound = false; m_bWorkingRoute = false;
	m_fStuckBestDist = 0; m_fStuckBestTime = 0; m_iStuckSkips = 0;
	m_fJumpRelease     = 0;
	m_fScoutDJRelease  = 0;
	m_iStuckRecovery   = 0;
	m_fSteerExpiry      = 0;
	m_iConsecutiveHits  = 0;
	m_iDrainStreak     = 0;
	m_fLastTraceLog    = 0;
	m_fLastStuckLog    = 0;
	m_fLastIdleLog     = 0;
	m_vIdleCheckPos    = Vector(0,0,0);
	m_fIdleCheckTime   = 0;
	m_fFailBackoffTime  = 0;
	m_fLastRepathTime   = 0;
	m_badConnections.clear();
	m_iEscapeMode = 0;
}

void CNavMeshNavigator::freeAllMemory()
{
	freeMapMemory();
	m_pAccessor = nullptr;
}

bool CNavMeshNavigator::routeFound()       { return m_bRouteFound; }
bool CNavMeshNavigator::canGetTo(Vector v) { return m_pAccessor && m_pAccessor->ready() && m_pAccessor->getNearestArea(v) != nullptr; }

bool CNavMeshNavigator::computeBuildSpot(int iBuildType, int iObjArea, int iTeam,
                                          Vector &vSpot, float &fYaw, int &iOutArea,
                                          float fMaxDist, Vector vRefPos)
{
	if (!m_pAccessor || !m_pAccessor->ready() || !m_pBot) return false;
	int n = m_pAccessor->getAreaCount();
	if (n <= 0) return false;
	Vector vBotOrigin = m_pBot->getOrigin();
	float  bestScore  = -9999.0f;
	Vector bestCenter(0,0,0);
	int    bestIdx    = -1;
	for (int i = 0; i < n; i++)
	{
		unsigned char *a = m_pAccessor->getAreaByIndex(i);
		if (!a || m_pAccessor->isGoalFailed(a)) continue;
		float *c = (float *)(a + NAV_M_CENTER);
		Vector vC(c[0], c[1], c[2]);
		if (fMaxDist > 0 && (vC - vBotOrigin).Length2D() > fMaxDist) continue;
		int attr = *(int *)(a + NAV_M_ATTR);
		if (attr & (NAV_ATTR_JUMP | NAV_ATTR_CROUCH)) continue;
		int walls = 0;
		CTraceFilterWorldAndPropsOnly trF;
		for (int d = 0; d < 4; d++)
		{
			float ang = d * 1.57079633f;
			Vector vDir(cosf(ang), sinf(ang), 0);
			Vector vEnd = vC + vDir * 300.0f;
			vEnd.z = vC.z + 24.0f;
			CBotGlobals::traceLine(vC + Vector(0,0,24), vEnd, MASK_PLAYERSOLID, &trF);
			trace_t *tr = CBotGlobals::getTraceResult();
			if (tr && tr->fraction < 1.0f) walls++;
		}
		int connCnt = 0;
		int connOff = m_pAccessor->getConnectOffset();
		for (int d = 0; d < 4; d++)
		{
			char *pData = *(char **)(a + connOff + d * 4);
			if (pData && m_pAccessor->ptrInArena(pData) && *(int*)pData > 0) connCnt++;
		}
		float score = 0.0f;
		if (iBuildType == 2)       { score = walls * 0.3f + connCnt * 0.05f; score += 1.0f / (1.0f + (vC - vBotOrigin).Length2D() / 1000.0f); }
		else if (iBuildType == 0)  {
			score = walls * 0.4f - connCnt * 0.1f;
			if (vRefPos.Length() > 0.1f) {
				float d = (vC - vRefPos).Length2D();
				if (d > 500.0f) score -= (d - 500.0f) / 500.0f;
			}
		}
		else if (iBuildType == 5)  { score = connCnt * 0.3f - walls * 0.1f; }
		else if (iBuildType == 4)  { score = walls * 0.35f - connCnt * 0.15f + (vC - vBotOrigin).Length2D() / 10000.0f; }
		else                       { score = walls * 0.3f; }
		if (score > bestScore) { bestScore = score; bestIdx = i; bestCenter = vC; }
	}
	if (bestIdx < 0) return false;
	float dxF = bestCenter.x - vBotOrigin.x, dyF = bestCenter.y - vBotOrigin.y;
	fYaw = atan2(dyF, dxF) * (180.0f / M_PI);
	vSpot = bestCenter; iOutArea = bestIdx;
	return true;
}

bool CNavMeshNavigator::getCoverPosition(Vector, Vector *) { return false; }
bool CNavMeshNavigator::getHideSpotPosition(Vector, Vector *) { return false; }
bool CNavMeshNavigator::getNextRoutePoint(Vector *p)
{
	if (m_route.empty() || !p) return false;

	if (m_fLookJitterTime <= engine->Time())
	{
		m_vLookJitter = Vector(randomFloat(-64, 64), randomFloat(-64, 64), 0);
		m_fLookJitterTime = engine->Time() + randomFloat(1.5f, 2.0f);
	}

	Vector vLook;
	int lookAhead = 3;
	if ((int)m_route.size() >= lookAhead)
		vLook = m_route[m_route.size() - lookAhead].pos;
	else if (m_route.size() >= 2)
		vLook = m_route[m_route.size() - 2].pos;
	else
		vLook = m_vCurrentTarget;

	*p = vLook + m_vLookJitter;
	return true;
}
int  CNavMeshNavigator::getCurrentWaypointID() { return -1; }
int  CNavMeshNavigator::getCurrentGoalID()     { return -1; }
float CNavMeshNavigator::getNextYaw()
{
	if (!m_pBot || m_vCurrentTarget.Length() < 0.1f) return 0.0f;
	Vector dir = m_vCurrentTarget - m_pBot->getOrigin();
	float baseYaw = atan2(dir.y, dir.x) * (180.0f / M_PI);
	baseYaw += sinf(engine->Time() * 0.7f) * 6.0f;
	return baseYaw;
}
// Navmesh navigator relies on stuck-detection in updatePosition() rather than external rollback; failMove is intentionally a no-op.
void  CNavMeshNavigator::failMove() {}
void  CNavMeshNavigator::clear() {}
float CNavMeshNavigator::distanceTo(Vector)      { return 0.0f; }
const std::deque<NavPathSegment> &CNavMeshNavigator::getRoute() const { return m_route; }
