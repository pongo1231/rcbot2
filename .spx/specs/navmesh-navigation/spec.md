## ADDED Requirements

### Requirement: Collinear Node Removal
The navmesh navigator SHALL remove redundant collinear nodes from the computed path deque to produce smoother routes with fewer unnecessary direction changes. A node is redundant when the angle formed by three consecutive waypoints is below 5 degrees and a direct trace between the outer two waypoints is unobstructed by world geometry.

#### Scenario: Three collinear nodes on a straight corridor
- **WHEN** the path deque contains nodes A→B→C where the angle ABC is less than 5 degrees AND a hull trace from A to C is clear
- **THEN** node B is removed from the path and the route goes directly from A to C

#### Scenario: Near-collinear nodes with a turn
- **WHEN** the path deque contains nodes A→B→C where the angle ABC is greater than 15 degrees
- **THEN** node B is retained in the path

#### Scenario: Path with fewer than 3 nodes
- **WHEN** the path deque has 2 or fewer nodes
- **THEN** `optimizePath()` returns without modifying the path

### Requirement: Walkability Validation for Optimized Shortcuts
When `optimizePath()` considers removing an intermediate node B to create a direct hop from A to C, it SHALL verify that the direct A→C edge is walkable by checking:
1. That the 2D line from A to C does not cross a gap (drop-off) using `hasPotentialGap()`
2. That the direct trace is unobstructed by world geometry using `isPotentiallyTraversable()`

#### Scenario: Shortcut crosses an unjumpable gap
- **WHEN** the direct A→C line has a potential drop-off detected by `hasPotentialGap()`
- **THEN** node B is NOT removed even if the angle is collinear

#### Scenario: Shortcut blocked by world geometry
- **WHEN** the direct A→C trace hits world geometry in `isPotentiallyTraversable()`
- **THEN** node B is NOT removed even if the angle is collinear

### Requirement: Navigator Area Tracking
The navmesh navigator SHALL maintain an accurate `m_pCurrentArea` pointer reflecting the bot's current nav area. When the route deque is emptied (all segments popped), `m_pCurrentArea` SHALL be set to `nullptr` to prevent stale pointer usage in bad-connection marking and stuck-recovery logic.

#### Scenario: Route fully drained by pop loop
- **WHEN** `updatePosition()` pops all remaining segments from the route deque
- **THEN** `m_pCurrentArea` is set to `nullptr` after the pop loop exits

#### Scenario: Route cleared via freeMapMemory
- **WHEN** `freeMapMemory()` is called
- **THEN** `m_pCurrentArea` is set to `nullptr`

#### Scenario: Bot still has route segments
- **WHEN** the route deque still has at least one segment after popping
- **THEN** `m_pCurrentArea` points to the area of the last-popped segment

### Requirement: Traversability Hull Check
The `isPotentiallyTraversable()` method SHALL perform hull-trace validation using a half-width of 16 units (matching the standard Source engine player standing hull width of 32 units) instead of 12 units. This ensures the check accurately represents whether a player-sized entity can walk the path.

#### Scenario: Path narrow but passable for a player
- **WHEN** a corridor is exactly 34 units wide
- **THEN** `isPotentiallyTraversable()` returns true (16+16=32 < 34, player fits)

#### Scenario: Path too narrow for a player
- **WHEN** a corridor is exactly 28 units wide
- **THEN** `isPotentiallyTraversable()` returns false (16+16=32 > 28)

#### Scenario: Old check would have passed a too-narrow corridor
- **WHEN** a corridor is 30 units wide
- **THEN** the old ±12u check would have passed (12+12=24 < 30) but the new ±16u check correctly returns false (16+16=32 > 30)

### Requirement: Stair-Aware Jump Behavior in Steering Fallback
When the steering sweep determines that all forward and angled directions are blocked by obstacles, the navigator SHALL check the `NAV_ATTR_STAIRS` and `NAV_ATTR_NO_JUMP` flags on the current and next nav areas before attempting a jump. If either flag is set, the jump SHALL be suppressed and the navigator SHALL instead pop the blocked edge from the route.

#### Scenario: All steer angles blocked on stairs
- **WHEN** the bot is on a nav area with `NAV_ATTR_STAIRS` AND all 12 steering angles are blocked
- **THEN** the navigator does NOT jump; instead it pops the current segment and blacklists the false connection

#### Scenario: All steer angles blocked on a no-jump area
- **WHEN** the bot is on a nav area with `NAV_ATTR_NO_JUMP` AND all steer angles are blocked
- **THEN** the navigator does NOT jump; instead it pops the current segment and blacklists the false connection

#### Scenario: All steer angles blocked on a jump-enabled area
- **WHEN** the bot is on a nav area without STAIRS or NO_JUMP flags AND all steer angles are blocked AND vertical clearance exists above the bot
- **THEN** the navigator performs a jump attempt (existing behavior preserved)

### Requirement: Extended Steering Sweep to ±150°
The steering sweep in `updatePosition()` SHALL test angles up to ±150° from the bot's forward direction (relative to the route target), allowing the sweep to discover 90° corner exits. The primary 5-angle sweep (±60°) is unchanged. The fallback 7-angle sweep SHALL be extended from its current ±75° range to a 7-angle sweep covering ±150° by doubling the angular step from 25° to 50°.

#### Scenario: 90° corner exit is discovered by extended sweep
- **WHEN** the bot faces a wall with an open corridor at 90° to its left AND the 5-angle sweep (±60°) finds all angles blocked
- **THEN** the 7-angle fallback sweep tests angles including 90° left and discovers the clear corridor

#### Scenario: 90° corner exit on the right
- **WHEN** the bot faces a wall with an open corridor at 90° to its right
- **THEN** the extended sweep discovers the +90° direction and the bot side-steps right

#### Scenario: All angles still blocked at ±150°
- **WHEN** the bot is in a dead-end with walls at all angles within ±150°
- **THEN** the existing all-angles-blocked fallback (jump/pop) executes — no regression

### Requirement: Backward Trace in All-Angles-Blocked Fallback
When the steering sweep finds all forward and angled directions blocked (fBestClear ≤ 0.25), the navigator SHALL trace 180° behind the bot (backward direction) before resorting to a jump or route pop. If the backward trace has clearance ≥ 1.0f (unobstructed), the bot SHALL walk backward 150 units with the existing `setMoveTo` mechanism, allowing it to back out of a tight corner.

#### Scenario: Bot wedged in a corner can back out
- **WHEN** all ±150° steering angles are blocked AND the backward trace is clear
- **THEN** the bot walks backward 150 units instead of jumping or popping the route

#### Scenario: Bot in a dead-end with no backward escape
- **WHEN** all ±150° steering angles are blocked AND the backward trace is also blocked (frac < 1.0)
- **THEN** the existing jump/pop fallback executes — no regression

### Requirement: Both-Blocked Corner Avoidance Fallback
The `avoidObstacles()` function SHALL no longer return the goal position unchanged when both left and right side-feelers are blocked. Instead, it SHALL bias toward the side with the greater clearance fraction, steering away from the more-obstructed side. If both feelers have identical clearance, the function SHALL bias toward the side matching the current strafe direction (`m_sStrafeDir`).

#### Scenario: Left wall is closer than right wall in a corner
- **WHEN** the left feeler has fraction 0.4 and the right feeler has fraction 0.7
- **THEN** the function returns a point biased rightward (away from the closer left wall)

#### Scenario: Both feelers equally blocked in a symmetrical corner
- **WHEN** both feelers have fraction 0.3
- **THEN** the function picks a side based on `m_sStrafeDir` rather than returning goalPos unchanged

#### Scenario: One side fully clear, one side blocked (existing behavior preserved)
- **WHEN** the left feeler has fraction 1.0 and the right feeler has fraction 0.5
- **THEN** the function returns the existing left-avoidance steer (unchanged from current behavior)

### Requirement: Increased Side-Feeler Range
The `avoidObstacles()` side-feeler traces SHALL extend from 50 units to 80 units forward range, giving the bot earlier awareness of approaching walls and corners.

#### Scenario: Wall detected at 70 units with extended range
- **WHEN** a wall is at 70 units ahead, offset ±20u from the bot's forward path
- **THEN** the 80-unit feeler detects it (previously the 50-unit feeler would have missed until the wall was at 50 units)

### Requirement: Corner Pivot Scan
When the steering sweep finds all angles blocked at ±150° AND both `avoidObstacles()` feelers are blocked, the navigator SHALL run a 16-direction fan scan (same pattern as escape mode's scanner: 3-trace hull checks at 16 directions × 22.5° each for 400 units) to find the clearest perpendicular corridor. If a direction with >50% clearance is found, the bot SHALL pivot toward it with a side-move and set `m_fSteerExpiry` to persist the redirect for 2 seconds. This provides a one-shot wall-follow pivot that avoids immediate escalation to route drain.

#### Scenario: 90° corner with clear corridor to the left
- **WHEN** the steering sweep finds all blocked AND both feelers hit walls AND a 16-direction scan finds 50%+ clearance 90° left
- **THEN** the bot pivots leftward with a 2-second steer redirect

#### Scenario: Corner with no clear perpendicular corridor
- **WHEN** the 16-direction scan finds no direction with >50% clearance
- **THEN** the existing all-angles-blocked fallback (backward trace → jump/pop) executes — no regression

#### Scenario: Corner pivot does not re-trigger while active
- **WHEN** a corner pivot is active (m_fSteerExpiry is in the future)
- **THEN** the corner pivot scan is skipped to avoid conflicting redirects

### Requirement: Waist-Height Obstacle Trace
The preemptive obstacle-detection trace SHALL originate from the bot's waist height (`vBotOrigin + Vector(0,0,18)`) rather than foot level, so that the forward trace strikes the upper face of small obstacles rather than their base. This ensures the subsequent down-trace correctly measures the obstacle's height above ground.

#### Scenario: 24-unit crate detected and measured correctly
- **WHEN** a 24-unit crate sits on the ground 100 units ahead of the bot
- **THEN** the trace from waist height (z+18) hits the top portion of the crate, the down-trace from the hit point finds ground 24 units below, and `obstacleHeight` is measured as approximately 24 units

#### Scenario: Trace over sloped ground does not trigger
- **WHEN** the ground ahead rises at a 20-degree slope
- **THEN** the trace hits the slope surface whose `plane.normal.z > 0.5f`, the ground/slope branch runs, and no jump is attempted

#### Scenario: Trace hits a full-height wall
- **WHEN** a 96-unit wall spans the trace path
- **THEN** the down-trace from the hit point finds ground near the hit point (obstacleHeight ~0) and no jump is attempted

### Requirement: Extended Detection Range
The forward obstacle-detection trace SHALL scan 200 units ahead, and obstacles within a ConVar-tunable range (default 160 units) SHALL be eligible for a jump response. This allows the bot enough travel time to become airborne before arriving at the obstacle.

#### Scenario: Obstacle at 120 units triggers jump
- **WHEN** an obstacle is detected at 120 units (with `rcbot_navmesh_jump_obstacle_range` at default 160)
- **THEN** the jump-eligible code path runs

#### Scenario: Obstacle at 180 units is ignored
- **WHEN** an obstacle is detected at 180 units (with `rcbot_navmesh_jump_obstacle_range` at default 160)
- **THEN** the jump-eligible code path does NOT run; the bot continues walking normally

#### Scenario: Range adjusted at runtime
- **WHEN** `rcbot_navmesh_jump_obstacle_range` is changed to 100 at runtime
- **THEN** obstacles detected beyond 100 units no longer trigger jump responses

### Requirement: Tunable Obstacle Height Range
The minimum and maximum obstacle heights that trigger a jump response SHALL be controlled by ConVars `rcbot_navmesh_jump_obstacle_min` (default 18) and `rcbot_navmesh_jump_obstacle_max` (default 72). The upper bound SHALL accommodate crouch-jump clearance (~72 units).

#### Scenario: 60-unit obstacle triggers jump
- **WHEN** an obstacle is measured at 60 units height (with default max of 72)
- **THEN** the bot jumps over the obstacle

#### Scenario: 80-unit obstacle does not trigger jump
- **WHEN** an obstacle is measured at 80 units height (with default max of 72)
- **THEN** the bot does NOT attempt to jump; the obstacle is too tall

#### Scenario: 12-unit curb does not trigger jump
- **WHEN** an obstacle is measured at 12 units height (with default min of 18)
- **THEN** the bot does NOT jump; the lip is a step, not a jump obstacle

### Requirement: Scout Double-Jump on Obstacles
When the bot is a TF2 Scout and a preemptive obstacle jump is triggered, the navigator SHALL schedule a second jump via `m_fScoutDJRelease` set to `engine->Time() + 0.28f`, consistent with the existing Scout double-jump pattern in `applyNavAttributes()`. This allows Scouts to clear taller obstacles than other classes.

#### Scenario: Scout encounters a 70-unit obstacle
- **WHEN** a Scout bot triggers a preemptive jump for a 70-unit obstacle AND the initial `tapButton(IN_JUMP)` fires
- **THEN** `m_fScoutDJRelease` is set to `engine->Time() + 0.28f`, causing a second jump ~0.28 seconds later

#### Scenario: Non-Scout does not schedule double-jump
- **WHEN** a Soldier or Heavy bot triggers a preemptive obstacle jump
- **THEN** `m_fScoutDJRelease` is NOT set; only a single jump is performed

### Requirement: Class-Specific Enhanced Jumps on Obstacles
When the bot is a TF2 Soldier (health > 50%) or Demoman (with `rcbot_demo_jump` enabled) and a preemptive obstacle jump is triggered for an obstacle taller than 48 units, the navigator SHALL use `CBotTF2::jump()` (rocket/sticky jump) instead of `tapButton(IN_JUMP)`, consistent with the existing class-jump logic in `applyNavAttributes()`.

#### Scenario: Soldier rocket-jumps a tall obstacle
- **WHEN** a Soldier with >50% health triggers a preemptive jump for a 50-unit obstacle
- **THEN** `CBotTF2::jump()` is called and `m_fJumpRelease` is set to `engine->Time() + 1.2f`

#### Scenario: Demoman pipe-jumps a tall obstacle when enabled
- **WHEN** a Demoman triggers a preemptive jump for a 50-unit obstacle AND `rcbot_demo_jump` is 1
- **THEN** `CBotTF2::jump()` is called with a 1.2-second cooldown

#### Scenario: Non-TF2 game falls back to standard jump
- **WHEN** the bot is NOT a TF2 class (e.g. HL2DM) and a preemptive obstacle jump triggers
- **THEN** `tapButton(IN_JUMP)` is called with a 1.0-second cooldown

### Requirement: Trace Result Snapshot
Because `CBotGlobals::getTraceResult()` returns a pointer to a single static `trace_t` that every `traceLine()` call overwrites, the helper SHALL snapshot the forward-trace hit position (`vHitPos`), fraction, and surface normal immediately after the forward trace, before performing any subsequent traces (down-trace, wide-wall, top-surface, vertical clearance). All later code SHALL use the snapshot, not the live `tr` pointer.

#### Scenario: Down-trace does not corrupt obstacle height measurement
- **WHEN** the forward trace hits an obstacle face at z=160 and the down-trace from that point finds ground at z=140
- **THEN** `obstacleHeight` is computed as `vHitPos.z - trG->endpos.z = 160 - 140 = 20`, not as `tr->endpos.z - trG->endpos.z` (which would be 140 - 140 = 0 after the static was clobbered)

### Requirement: Top-Surface Walkability Validation
Before triggering a jump, the helper SHALL verify that the obstacle has a walkable top surface behind its face. It SHALL trace forward from just above the obstacle top (ground height + obstacle height + 4 units) at a downward angle, and SHALL require the trace to hit a surface whose `plane.normal.z >= 0.7f` (a flat, walkable surface). If no such surface is found, the obstacle is a wall with no top to land on, and the jump SHALL be suppressed.

#### Scenario: Crate with a walkable top surface
- **WHEN** a 48-unit crate has a flat top surface
- **THEN** the top-surface trace hits the crate top at a near-flat angle and the jump proceeds

#### Scenario: Wall with no top surface accessible
- **WHEN** a 96-unit wall has no walkable surface behind its face within the trace range
- **THEN** the top-surface trace either misses entirely or hits a steep surface, and the jump is suppressed

### Requirement: Jump Cooldown Anti-Spam
The helper SHALL enforce a minimum cooldown between obstacle-triggered jumps to prevent bunnyhopping. For basic `tapButton(IN_JUMP)` calls the cooldown SHALL be 1.0 seconds. For Scouts the initial jump cooldown SHALL be 0.8 seconds (with the double-jump at +0.28s per existing pattern). The existing 1.2-second cooldown for rocket/sticky jumps is unchanged.

#### Scenario: Bot approaches consecutive obstacles
- **WHEN** a bot has just performed an obstacle jump and approaches another obstacle 0.4 seconds later
- **THEN** `m_fJumpRelease` is still in the future (`engine->Time() + 0.6s remaining`), so the jump is suppressed

#### Scenario: Bot approaches obstacle after cooldown expires
- **WHEN** a bot approaches an obstacle 1.1 seconds after its last obstacle jump
- **THEN** `m_fJumpRelease <= engine->Time()` and the jump proceeds

### Requirement: Unified Jump-Detection Helper
The preemptive obstacle-detection logic in `updatePosition()` and the escape-mode jump logic in `doEscapeMode()` SHALL use a shared private helper method `doObstacleJump(const Vector &vBotOrigin, const Vector &vFwdDirection, bool bAllowEnhancedJumps)` that encapsulates the trace setup, wide-wall check, height validation, vertical-clearance check, top-surface validation, trace-result snapshotting, and jump dispatch. Both call sites SHALL invoke this helper with their respective forward-direction vectors and an enhanced-jumps flag.

#### Scenario: Preemptive block calls unified helper
- **WHEN** the preemptive trace-ahead block detects a wall-hit within the jump range
- **THEN** it calls `doObstacleJump(vBotOrigin, vFwd, true)` to evaluate and potentially execute a jump

#### Scenario: Escape-mode jump calls unified helper
- **WHEN** the escape-mode trace-ahead detects a jumppable obstacle in the escape target direction
- **THEN** it calls `doObstacleJump(vBotNow, vFwdDir, false)` to evaluate and potentially execute a jump

### Requirement: Preemptive Obstacle Detection Outer Gate
The outer gate in the preemptive obstacle-detection block SHALL use a minimal distance threshold (`> 20.0f`) to determine whether the bot is close enough to its route target to bother tracing for obstacles. Previously the outer gate used the same distance as the obstacle range, which caused the entire detection block to be skipped whenever the bot was within range of its route target (which is nearly always).
