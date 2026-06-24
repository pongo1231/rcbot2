// bot_navmesh.h — raw navmesh access + navigator

#pragma once

#include <vector>
#include <deque>
#include <cstdint>
#include <map>
#include "bot_sigscan.h"
#include "bot_navigator.h"

#define NAV_M_CENTER     44    // m_center offset (vtable at 0 shifts by 4)
#define NAV_M_ATTR       0x54  // m_attributeFlags offset (80 + 4 vtable = 84 = 0x54)
#define NAV_M_LADDER     0x68  // m_ladder[0] (LADDER_UP) runtime offset
#define NAV_M_ELEV       0x70  // m_elevatorAreas runtime offset

// CNavArea corner layout (offsets include +4 vtable shift)
#define NAV_M_NWCORNER   0x04  // Vector m_nwCorner (2D mins, x/y/z)
#define NAV_M_SECORNER   0x10  // Vector m_seCorner (2D maxs, x/y/z)
#define NAV_M_INVDX      0x1C  // float m_invDxCorners
#define NAV_M_INVDY      0x20  // float m_invDyCorners
#define NAV_M_NEZ        0x24  // float m_neZ (height at NE implicit corner)
#define NAV_M_SWZ        0x28  // float m_swZ (height at SW implicit corner)

// NavAttributeType bit masks (see nav.h)
#define NAV_ATTR_CROUCH  0x00000001 // NAV_MESH_CROUCH
#define NAV_ATTR_JUMP    0x00000002 // NAV_MESH_JUMP
#define NAV_ATTR_PRECISE 0x00000004 // NAV_MESH_PRECISE
#define NAV_ATTR_NO_JUMP 0x00000008 // NAV_MESH_NO_JUMP
#define NAV_ATTR_STOP    0x00000010 // NAV_MESH_STOP
#define NAV_ATTR_RUN     0x00000020 // NAV_MESH_RUN
#define NAV_ATTR_WALK    0x00000040 // NAV_MESH_WALK
#define NAV_ATTR_STAIRS  0x00001000 // NAV_MESH_STAIRS

// Teleporter link: src nav area → destination nav area.
// Populated on scanGrid() by scanning trigger_teleport entities.
struct TeleportLink
{
	void *srcArea;
	void *dstArea;
	float srcX, srcY, srcZ; // source trigger position
	float dstX, dstY, dstZ; // destination position
};

// Path segment types matching NextBot's Path::SegmentType
enum NavSegmentType
{
	NAV_SEG_ON_GROUND,
	NAV_SEG_DROP_DOWN,
	NAV_SEG_CLIMB_UP,
	NAV_SEG_JUMP_OVER_GAP,
	NAV_SEG_LADDER_UP,
	NAV_SEG_LADDER_DOWN
};

// Typed path segment replacing raw void* in the route deque.
// Carries the area pointer plus computed geometry for portal-based
// walk targets, look-ahead skipping, and curvature-based speed.
struct NavPathSegment
{
	void    *area;              // CNavArea pointer
	Vector   pos;               // walk target (portal midpoint or area center)
	Vector   portalCenter;      // center of shared-edge portal from previous area
	float    portalHalfWidth;   // half-width of portal (0 = not computed)
	Vector   forward;           // unit vector along segment
	float    length;            // segment length (units)
	float    distanceFromStart; // cumulative distance from path start
	float    curvature;         // 0 = straight, 1 = 180° double-back
	NavSegmentType type;        // traversal type
	int      how;               // direction index (TF2 NavDirType: 0=N,1=E,2=S,3=W) from parent area
};

class CNavMeshAccessor : public CSignatureFunction
{
  public:
	CNavMeshAccessor() : m_pNavMesh(nullptr), m_nAreaCount(0), m_bReady(false), m_pBase(nullptr),
		m_connectOffset(-1), m_areaMin(0), m_areaMax(0) {}
	~CNavMeshAccessor() {}

	void init(class CRCBotKeyValueList &kv, void *pBase);
	void scanGrid();
	bool ready() const { return m_bReady && m_connectOffset >= 0; }
	bool found() const { return m_func != nullptr; }
	inline void *getFunc() { return m_func; }
	inline void *getBase() { return m_pBase; }
	int  getAreaCount() const { return m_nAreaCount; }
	void invalidate() { m_Areas.clear(); m_nAreaCount = 0; m_bReady = false; m_connectOffset = -1;
		m_areaMin = 0; m_areaMax = 0; m_failedGoals.clear(); m_teleportLinks.clear(); }
	unsigned char *getNearestArea(float x, float y, float z);
	unsigned char *getNearestArea(Vector v) { return getNearestArea(v.x, v.y, v.z); }
	unsigned char *getAreaByIndex(int i) const;

	// Offset of the m_connect[4] CUtlVectorUltraConservative array within a
	// CNavArea, detected at runtime (the layout differs between game builds).
	// Returns -1 if detection failed (navmesh should then stay inert).
	int  getConnectOffset() const { return m_connectOffset; }
	// True if p is one of the cached nav area pointers (O(log n)).
	bool areaKnown(const void *p) const;

	// Failed-goal tracking: temporarily exclude an area pointer from
	// selection when recent A* routing to it has failed.  Prevents
	// bots from repeatedly targeting unreachable destinations.
	bool isGoalFailed(const void *area) const;
	void markGoalFailed(const void *area);
	void clearFailedGoals() { m_failedGoals.clear(); }
	bool hasFailedGoals() const { return !m_failedGoals.empty(); }
	const std::vector<TeleportLink> &getTeleportLinks() const { return m_teleportLinks; }

	// True if p is aligned and lies within the heap arena that holds the nav
	// areas (their connection Data_t blocks are allocated nearby). Used to
	// reject float/garbage values before dereferencing a candidate pointer.
	bool ptrInArena(const void *p) const
	{
		uintptr_t v = (uintptr_t)p;
		if (!v || (v & 3) || !m_areaMin || !m_areaMax) return false;
		const uintptr_t kSlack = 0x4000000; // 64 MB
		return v >= (m_areaMin - (m_areaMin > kSlack ? kSlack : m_areaMin))
		    && v <= (m_areaMax + kSlack);
	}

  private:
	bool detectConnectOffset();

	void *m_pNavMesh;
	void *m_pBase;
	std::vector<unsigned char *> m_Areas; // sorted+unique after scanGrid()
	int  m_nAreaCount;
	bool m_bReady;
	int  m_connectOffset;
	uintptr_t m_areaMin; // min/max cached area pointer = heap arena span
	uintptr_t m_areaMax;
	std::map<const void *, float> m_failedGoals; // goal area → clear time (180–600 s)
	std::vector<TeleportLink> m_teleportLinks;   // trigger_teleport connections
};

// forward decl
class CBot;
class CBotSchedule;

class CNavMeshNavigator : public IBotNavigator
{
  public:
	CNavMeshNavigator() : m_pAccessor(nullptr), m_pBot(nullptr), m_bWorkingRoute(false),
		m_pGoalArea(nullptr), m_pCurrentArea(nullptr), m_bRouteFound(false),
		m_vCurrentTarget(0,0,0), m_fStuckBestDist(0), m_fStuckBestTime(0),
		m_iStuckSkips(0), m_fJumpRelease(0), m_fScoutDJRelease(0),
		m_iStuckRecovery(0), m_vLookJitter(0,0,0), m_fLookJitterTime(0),
		m_fLookAroundTime(0), m_sStrafeDir(1),
		m_fLastTraceLog(0), m_fLastStuckLog(0), m_fLastIdleLog(0),
		m_vIdleCheckPos(0,0,0), m_fIdleCheckTime(0),
		m_fSteerExpiry(0), m_iConsecutiveHits(0), m_fLastHitTime(0),
		m_fMinLookAheadRange(150.0f), m_fLastRepathTime(0),
		m_fMinRepathInterval(0.5f), m_fFailBackoffTime(0),
		m_iEscapeMode(0), m_vEscapeTarget(0,0,0), m_fEscapeStartTime(0) {}
	void init();

	void setBot(CBot *pBot);
	bool isReady() const { return m_pAccessor && m_pAccessor->ready(); }
	CNavMeshAccessor *getAccessor() { return m_pAccessor; }
	const std::deque<NavPathSegment> &getRoute() const;
	bool isInEscapeMode() const { return m_iEscapeMode > 0; }

	bool workRoute(Vector vFrom, Vector vTo, bool *bFail, bool bRestart,
	               bool bNoInterruptions, int iGoalWptID,
	               int iConditions, int iDangerPoint);
	Vector getNextPoint();
	bool   hasNextPoint();
	void   updatePosition();
	bool   routeFound();
	void   freeMapMemory();
	void   freeAllMemory();
	bool   canGetTo(Vector vOrigin);
	float  belief(int iWpt);
	bool   getCoverPosition(Vector, Vector *);
	bool   getHideSpotPosition(Vector, Vector *);
	bool   getNextRoutePoint(Vector *);
	int    getCurrentWaypointID();
	int    getCurrentGoalID();
	float  getNextYaw();
	void   failMove();
	void   clear();
	float  distanceTo(Vector);
	void   rollBackPosition() {}

	void   belief(Vector, Vector, float, float, BotBelief) {}
	// Must write a valid pointer: the caller (CBotTF2::getTasks) passes the
	// result straight to resetFailedWaypoints() without a null guard against
	// an uninitialised pointer. Return an always-empty list.
	void   getFailedGoals(WaypointList **g) { if (g) *g = &m_failedGoals; }
	float  distanceTo(CWaypoint *) { return 0.0f; }

	Vector getRandomAreaCenter(Vector vFrom, float minDist = 300.0f,
	                           float maxDist = 800.0f, bool bTrace = false);

	// Auto-compute a build spot (SENTRY, DISPENSER, TELE_ENTRANCE,
	// TELE_EXIT) from nav areas when no waypoint-flagged spots exist.
	// Returns true and fills vSpot + fYaw + iArea if a good spot was found.
	bool computeBuildSpot(int iBuildType, int iArea, int iTeam,
	                      Vector &vSpot, float &fYaw, int &iOutArea,
	                      float fMaxDist = -1.0f,
	                      Vector vRefPos = Vector(0,0,0));

	// Portal-based path post-processing: populate portalCenter,
	// portalHalfWidth, forward, length, curvature for every segment.
	void postProcessPath();

	// Trace every consecutive area pair in the route.  If any hop
	// is blocked by world geometry between their centers, the nav
	// mesh has a false connection and the route is invalid.
	bool validateRoute();

	// Remove redundant collinear nodes (gated by rcbot_navmesh_optimize).
	void optimizePath();

	// Proactive side-feeler obstacle avoidance. Returns an adjusted
	// walk target that steers around nearby obstacles.
	Vector avoidObstacles(const Vector &goalPos);

	// Hull-trace check: can the bot walk directly from 'from' to 'to'?
	bool isPotentiallyTraversable(Vector from, Vector to);

	// Scan along the line for drop-offs too high to jump back from.
	bool hasPotentialGap(Vector from, Vector to, float *fraction = nullptr);

	// Lazy pathfinding: if the target is directly reachable, skip A*.
	bool tryDirectPath(Vector vFrom, Vector vTo);

  private:
	CNavMeshAccessor *m_pAccessor;
	CBot *m_pBot;
	bool m_bWorkingRoute;
	std::deque<NavPathSegment> m_route;
	void *m_pGoalArea;
	void *m_pCurrentArea;
	Vector m_vGoalFloat;
	bool m_bRouteFound;
	Vector m_vCurrentTarget; // current walk target (portal midpoint or goal centre)
	WaypointList m_failedGoals; // always empty; returned by getFailedGoals()

	// Per-connection blocked-pair cache.  When a bot gets stuck or
	// detects a false connection (center-to-center trace blocked),
	// it records which (src→dst) area pair was unreachable so future
	// A* runs skip that edge.  This is per-navigator (no global
	// cascade) and decays with time so the bot can try again later.
	struct BadConnection {
		uintptr_t src;
		uintptr_t dst;
		float     expireTime; // engine->Time() when this expires
	};
	std::vector<BadConnection> m_badConnections;

	bool isConnectionBad(const void *src, const void *dst) const;
	void markConnectionBad(const void *src, const void *dst);
	void expireBadConnections();

	float  m_fStuckBestDist; // best distance to m_vCurrentTarget in this hop
	float  m_fStuckBestTime; // engine->Time() when bestDist was recorded
	int    m_iStuckSkips;    // consecutive force-pops (cap at 3)
	float  m_fJumpRelease;   // engine->Time() at which to release IN_JUMP
	float  m_fScoutDJRelease; // engine->Time() for Scout second jump
	int    m_iStuckRecovery;  // anti-stuck cycle counter (jump+strafe before pop)
	Vector m_vLookJitter;     // random offset added to look target
	float  m_fLookJitterTime; // engine->Time() when jitter refreshes
	float  m_fLookAroundTime; // engine->Time() for next LOOK_AROUND glance
	short  m_sStrafeDir;      // alternating +1/-1 for sidestep direction
	float  m_fSteerExpiry;     // engine->Time() until steer redirect persists
	int    m_iConsecutiveHits; // consecutive trace-ahead wall-hit ticks
	float  m_fLastHitTime;     // engine->Time() of last trace-ahead wall hit
	float  m_fLastTraceLog;   // throttle for trace-hit log (Log A)
	float  m_fLastStuckLog;   // throttle for stuck-state log (Log B)
	float  m_fLastIdleLog;    // throttle for idle-position log (Log C)
	Vector m_vIdleCheckPos;   // position snapshot for idle detection
	float  m_fIdleCheckTime;  // engine->Time() when idle snapshot taken
	float  m_fMinLookAheadRange; // min range for look-ahead node skipping (0=off)
	float  m_fLastRepathTime;    // engine->Time() of last route computation
	float  m_fMinRepathInterval; // minimum interval between repaths
	float  m_fFailBackoffTime;   // backoff timer after path-find failure

	// Escape mode: when A* can't find any path from our current
	// position (e.g. stuck on a ledge with only false connections),
	// walk the clearest direction until we fall off and find solid
	// ground with valid nav connections.
	int    m_iEscapeMode;        // 0=off, 1=walking/to, 2=falling
	Vector m_vEscapeTarget;      // world position we're walking toward
	float  m_fEscapeStartTime;   // when escape mode began
};

// ---------- CNavMeshAccessor ----------
// Finds TheNavMesh via signature scan (GOT PUSH pattern), reads the grid
// of CNavArea objects, and caches valid area pointers for the navigator.
