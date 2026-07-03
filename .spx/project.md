# Project Conventions

## Tech Stack
- **Language**: C++17, compiled with GCC/clang via AMBuild
- **Platform**: Linux (TF2 dedicated server), Source Engine (SDK 2013)
- **Build system**: AMBuild (Valve's meta-build system)
- **Plugin model**: SourceMod extension (.so), loaded at server start via meta-mod

## Architecture
- `rcbot2/` — main bot plugin code
- `rcbot/` — legacy bot base (being migrated away from)
- `botutil/` — schedule tasks and utilities (shared across bot types)
- `sm_ext/` — SourceMod extension glue
- Navigation: dual-backend — waypoint navigator (`CWaypointNavigator`) and navmesh navigator (`CNavMeshNavigator`), both implementing `IBotNavigator`. The active navigator is chosen per-frame via `rcbot_use_navmesh` ConVar.

## Naming Conventions
- Classes: `C` prefix (e.g., `CBot`, `CNavMeshNavigator`)
- Members: `m_` prefix (e.g., `m_pBot`, `m_route`)
- Pointers: `p` prefix (e.g., `pBot`, `pAcc`)
- Globals: `g_` prefix (e.g., `g_pNavMeshAccessor`)
- ConVars: `rcbot_` prefix, snake_case (e.g., `rcbot_debug_navmesh`)
- Booleans: `b` prefix (e.g., `bFound`, `m_bReady`)

## Patterns
- Raw pointer-based graph (no smart pointers in navmesh code)
- `CBot::setMoveTo()` is the sole output for locomotion — the navigator writes to it
- `engine->Time()` for all time comparisons (not system time)
- `fprintf(stderr, ...)` for debug logging, gated by debug CVars
- `ifdef` guards for game-specific code (`#if SOURCE_ENGINE == SE_TF2`)

## Map Lifecycle
- Plugin load → `CNavMeshAccessor::init()` (sigscan once)
- Map start → lazy `scanGrid()` on first `getNearestArea()`
- Map end → `invalidate()` clears all cached areas
- No persistence between maps
