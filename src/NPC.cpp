#include "NPC.h"
#include <cmath>
#include <iostream>

#include "world.h"
#include "sound_manager.h"
#include "utilities.h"
#include "character.h"
#include "ui.h"

static bool PtrStillListed(Character* p, const std::vector<Character*>& enemyPtrs)
{
    if (!p) return false;
    for (Character* e : enemyPtrs)
    {
        if (e == p) return true;
    }
    return false;
}

static bool IsWithinAimConeXZ(const Vector3& hermitPos,
                             float hermitYawDeg,
                             const Vector3& enemyPos,
                             float maxDegrees)
{
    float yaw = hermitYawDeg * DEG2RAD;
    Vector3 forward = { sinf(yaw), 0.0f, cosf(yaw) };

    Vector3 toE = Vector3Subtract(enemyPos, hermitPos);
    toE.y = 0.0f;

    float lenSq = Vector3DotProduct(toE, toE);
    if (lenSq < 0.0001f) return true;

    toE = Vector3Scale(toE, 1.0f / sqrtf(lenSq));

    float cosThresh = cosf(maxDegrees * DEG2RAD);
    float d = Vector3DotProduct(forward, toE);
    return d >= cosThresh;
}

static bool ViewerIsInFrontOfNPC(const Vector3& npcPos, float npcYawDeg, const Vector3& viewerPos)
{
    float yaw = npcYawDeg * DEG2RAD;
    Vector3 forward = { sinf(yaw), 0.0f, cosf(yaw) };

    Vector3 toViewer = Vector3Subtract(viewerPos, npcPos);
    toViewer.y = 0.0f;

    float lenSq = Vector3DotProduct(toViewer, toViewer);
    if (lenSq < 0.0001f) return true;

    toViewer = Vector3Scale(toViewer, 1.0f / sqrtf(lenSq));
    float d = Vector3DotProduct(forward, toViewer);
    return (d > 0.0f);
}

void NPC::Init(Texture2D sheet, int frameW, int frameH, float sc, float /*rotationYParam*/)
{
    texture = sheet;
    frameWidth  = frameW;
    frameHeight = frameH;
    scale = sc;

    homePos = position;
    patrolLastPos = position;

    visionRadiusSq = visionRadius * visionRadius;
    // fireRadiusSq is already stored as sq in your code; keep as-is

    // start idle anim
    ResetAnim(0, 0, 1, 0.15f);
}

void NPC::Update(float dt, const std::vector<Character*>& enemyPtrs)
{
    if (!isActive) return;



    // Talk override behavior (same idea you had)
    if (state == NPCState::Talk && !CanInteract(player.position))
        state = NPCState::Idle;

    // If talking, let timed loop / oneshot drive animation and early out
    if (state == NPCState::Talk)
    {
        animIntent = AnimIntent::Talk;
        UpdateHermitAnimationFromIntent();
        UpdateAnim(dt);
        previousPosition = position;
        return;
    }

    if (type == NPCType::Hermit)
    {
        UpdateHermitBrain(dt, enemyPtrs);
        UpdateHermitAnimationFromIntent();
        UpdateAnim(dt);
    }
    else
    {
        // Placeholder: other NPC types later
        animIntent = AnimIntent::Idle;
        UpdateAnim(dt);
    }

    previousPosition = position;
}

// -------------------------------------------------------------
// High-level hermit brain selection (ONE place)
// -------------------------------------------------------------
void NPC::UpdateHermitBrain(float dt, const std::vector<Character*>& enemyPtrs)
{

    if (!gHermitIntroDone)
    {
        hermitBrain = HermitBrain::Patrol;      // or anything; doesn't matter
        SetPatrolState(HermitPatrolState::Idle);
        animIntent = AnimIntent::Idle;
        ClearNav();
        return;
    }
    // Keep your old "if no target for long -> patrol" vibe:
    // We only advance idleNoTargetTimer while NOT seeing enemies.
    if (!HasTargetInVision(enemyPtrs))
    {
        idleNoTargetTimer += dt;
        if (idleNoTargetTimer >= patrolIdleDelay)
        {
            idleNoTargetTimer = 0.0f;

            // If we aren't in combat, bias back toward patrol (matches your old behavior)
            if (hermitBrain != HermitBrain::Turret)
                hermitBrain = HermitBrain::Patrol;
        }
    }
    else
    {
        idleNoTargetTimer = 0.0f;
    }

    // Priority:
    // 1) If any enemy in vision => Turret
    // 2) Else if canFollow => Follow
    // 3) Else Patrol
    if (HasTargetInVision(enemyPtrs))
        hermitBrain = HermitBrain::Turret;
    else if (canFollow)
        hermitBrain = HermitBrain::Follow;
    else
        hermitBrain = HermitBrain::Patrol;

    switch (hermitBrain)
    {
        case HermitBrain::Turret: UpdateHermitTurretBrain(dt, enemyPtrs); break;
        case HermitBrain::Patrol: UpdateHermitPatrolBrain(dt); break;
        case HermitBrain::Follow: UpdateHermitFollowBrain(dt); break;
    }
}

// -------------------------------------------------------------
// Animation mapping (ONE place)
// -------------------------------------------------------------
void NPC::UpdateHermitAnimationFromIntent()
{
    // Your rows (unchanged)
    const int FRONT_ROW_IDLE = 0;
    const int FRONT_ROW_AIM  = 2;
    const int FRONT_ROW_FIRE = 2;
    const int FRONT_ROW_WALK = 5;

    const int REAR_ROW_IDLE  = 3;
    const int REAR_ROW_AIM   = 3;
    const int REAR_ROW_FIRE  = 4;
    const int REAR_ROW_WALK  = 6;

    Vector3 viewerPos = player.position;
    bool showFront = ViewerIsInFrontOfNPC(position, rotationY, viewerPos);

    int rowIdle = showFront ? FRONT_ROW_IDLE : REAR_ROW_IDLE;
    int rowAim  = showFront ? FRONT_ROW_AIM  : REAR_ROW_AIM;
    int rowFire = showFront ? FRONT_ROW_FIRE : REAR_ROW_FIRE;
    int rowWalk = showFront ? FRONT_ROW_WALK : REAR_ROW_WALK;

    switch (animIntent)
    {
        case AnimIntent::Idle: ResetAnim(rowIdle, 0, 1, 0.10f); break;
        case AnimIntent::Aim:  ResetAnim(rowAim,  1, 1, 0.10f); break;
        case AnimIntent::Fire: ResetAnim(rowFire, 2, 1, 0.10f); break;
        case AnimIntent::Walk: ResetAnim(rowWalk, 0, 2, 0.50f); break;
        case AnimIntent::Talk: ResetAnim(1,       0, 3, 0.50f); break;
    }
}

// -------------------------------------------------------------
// Senses
// -------------------------------------------------------------
bool NPC::HasTargetInVision(const std::vector<Character*>& enemyPtrs) const
{
    for (Character* e : enemyPtrs)
    {
        if (!e) continue;
        if (e->isDead) continue;
        if (e->currentHealth <= 0) continue;

        Vector3 d = Vector3Subtract(e->position, position);
        float distSq = Vector3DotProduct(d, d);
        if (distSq <= visionRadiusSq)
            return true;
    }
    return false;
}

// -------------------------------------------------------------
// Flip stabilization (kept, but fixed to do what it says)
// -------------------------------------------------------------
void NPC::SetFlipX()
{
    if (!target) return;

    // Don’t flip while firing (keeps your old “stable fire frame” rule)
    if (turretState == HermitTurretState::Fire) return;

    float yaw = rotationY * DEG2RAD;
    Vector3 forward = { sinf(yaw), 0.0f, cosf(yaw) };

    Vector3 toT = Vector3Subtract(target->position, position);
    toT.y = 0.0f;

    float side = forward.x * toT.z - forward.z * toT.x;

    bool wantFlip = flipX;
    if (side >  flipDeadZone) wantFlip = true;
    if (side < -flipDeadZone) wantFlip = false;

    if (wantFlip != flipX && flipCooldownLeft <= 0.0f)
    {
        flipX = wantFlip;
        flipCooldownLeft = flipCooldown;
    }
}

// -------------------------------------------------------------
// Turret brain
// -------------------------------------------------------------
void NPC::SetTurretState(HermitTurretState s)
{
    if (turretState == s) return;
    turretState = s;
    stateTimer = 0.0f;

    if (s == HermitTurretState::Aim)
    {
        aimTimer = 0.0f;
    }
    else if (s == HermitTurretState::Fire)
    {
        firedThisCycle = false;
    }
}

Character* NPC::AcquireClosestEnemy(const std::vector<Character*>& enemyPtrs)
{
    // Keep current target if valid and within vision
    if (target && PtrStillListed(target, enemyPtrs) &&
        !target->isDead && target->currentHealth > 0)
    {
        Vector3 d = Vector3Subtract(target->position, position);
        float distSq = Vector3DotProduct(d, d);
        if (distSq <= visionRadiusSq)
            return target;
    }

    Character* best = nullptr;
    float bestDistSq = visionRadiusSq;

    for (Character* e : enemyPtrs)
    {
        if (!e) continue;
        if (e->isDead) continue;
        if (e->currentHealth <= 0) continue;

        Vector3 d = Vector3Subtract(e->position, position);
        float distSq = Vector3DotProduct(d, d);

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            best = e;
        }
    }
    return best;
}

void NPC::MaintainOrAcquireTarget(float dt, const std::vector<Character*>& enemyPtrs)
{
    auto TargetIsValid = [&]() -> bool {
        if (!target) return false;
        if (!PtrStillListed(target, enemyPtrs)) return false;
        if (target->isDead) return false;
        if (target->currentHealth <= 0) return false;

        Vector3 d = Vector3Subtract(target->position, position);
        float distSq = Vector3DotProduct(d, d);
        return distSq <= visionRadiusSq;
    };

    // Still valid => reset grace timer
    if (TargetIsValid())
    {
        targetDropTimer = 0.0f;
        return;
    }

    // If we had a target, allow grace
    if (target)
    {
        targetDropTimer += dt;
        if (targetDropTimer < targetDropGrace)
            return;
    }

    // Drop and reacquire
    targetDropTimer = 0.0f;
    target = AcquireClosestEnemy(enemyPtrs);

    if (target && turretState == HermitTurretState::Idle)
        SetTurretState(HermitTurretState::Aim);
}

void NPC::UpdateHermitTurretBrain(float dt, const std::vector<Character*>& enemyPtrs)
{
    // Turret brain always runs with these timers
    stateTimer += dt;

    fireCooldownLeft = fmaxf(0.0f, fireCooldownLeft - dt);
    flipCooldownLeft = fmaxf(0.0f, flipCooldownLeft - dt);

    // If our stored target pointer got removed from list, drop immediately
    if (target && !PtrStillListed(target, enemyPtrs))
    {
        target = nullptr;
        targetDropTimer = 0.0f;
        aimTimer = 0.0f;
        firedThisCycle = false;
        SetTurretState(HermitTurretState::Idle);
    }

    MaintainOrAcquireTarget(dt, enemyPtrs);

    // Dispatch turret substate via table (no switch explosion)
    static TurretFn fns[(int)HermitTurretState::Count] = {
        &NPC::Hermit_Turret_Idle,
        &NPC::Hermit_Turret_Aim,
        &NPC::Hermit_Turret_Fire
    };

    (this->*fns[(int)turretState])(dt, enemyPtrs);
}

void NPC::Hermit_Turret_Idle(float /*dt*/, const std::vector<Character*>& /*enemyPtrs*/)
{
    animIntent = AnimIntent::Idle;

    if (target)
        SetTurretState(HermitTurretState::Aim);
}

void NPC::Hermit_Turret_Aim(float dt, const std::vector<Character*>& /*enemyPtrs*/)
{
    if (!target)
    {
        SetTurretState(HermitTurretState::Idle);
        return;
    }

    animIntent = AnimIntent::Aim;

    Vector3 d = Vector3Subtract(target->position, position);
    rotationY = atan2f(d.x, d.z) * RAD2DEG;

    float distSq = Vector3DotProduct(d, d);

    // Respect cooldown and engagement radius
    if (fireCooldownLeft > 0.0f) return;

    if (distSq > fireRadiusSq)
    {
        // In vision but not in engage range: drop target and let brain selection fall back to patrol/follow
        target = nullptr;
        SetTurretState(HermitTurretState::Idle);
        return;
    }

    SetFlipX();

    aimTimer += dt;
    if (aimTimer >= aimDuration)
    {
        SetTurretState(HermitTurretState::Fire);
    }
}

void NPC::Hermit_Turret_Fire(float /*dt*/, const std::vector<Character*>& enemyPtrs)
{
    animIntent = AnimIntent::Fire;

    // Fire once on entering Fire
    if (!firedThisCycle && target)
    {
        firedThisCycle = true;
        fireCooldownLeft = fireCooldown;

        Vector3 muzzle = Vector3Add(position, muzzleOffset);

        FireBullet(muzzle, target->position, 10000, 1.0, false, true);

        if (IsWithinAimConeXZ(muzzle, rotationY, target->position, 15.0f))
            target->TakeDamage(999);

        SoundManager::GetInstance().PlaySoundAtPosition(
            "musket", position, player.position, 0.0f, 3000.0f);
    }

    // Hold firing frame for duration
    if (stateTimer >= fireHoldDuration)
    {
        // reacquire like you did before
        Character* next = AcquireClosestEnemy(enemyPtrs);
        if (next)
        {
            target = next;
            SetTurretState(HermitTurretState::Aim);
        }
        else
        {
            target = nullptr;
            SetTurretState(HermitTurretState::Idle);
        }
    }
}

// -------------------------------------------------------------
// Patrol brain
// -------------------------------------------------------------
void NPC::SetPatrolState(HermitPatrolState s)
{
    if (patrolState == s) return;
    patrolState = s;
    stateTimer = 0.0f;
}

void NPC::ClearNav()
{
    navHasPath = false;
    navPath.clear();
    navPathIndex = -1;
}

void NPC::UpdateHermitPatrolBrain(float dt)
{
    // Only patrol on island nav
    if (!hasIslandNav)
    {
        animIntent = AnimIntent::Idle;
        SetPatrolState(HermitPatrolState::Idle);
        return;
    }

    stateTimer += dt;
    navRepathTimer += dt;

    static PatrolFn fns[(int)HermitPatrolState::Count] = {
        &NPC::Hermit_Patrol_Idle,
        &NPC::Hermit_Patrol_Move
    };

    (this->*fns[(int)patrolState])(dt);
}

void NPC::Hermit_Patrol_Idle(float dt)
{
    animIntent = AnimIntent::Idle;

    // Face player while idle (matches your prior behavior)
    Vector3 d = Vector3Subtract(player.position, position);
    rotationY = atan2f(d.x, d.z) * RAD2DEG;

    // wait then pick a goal
    idleNoTargetTimer += dt;
    if (idleNoTargetTimer < patrolIdleDelay) return;
    idleNoTargetTimer = 0.0f;

    float cellSize = terrainScale.x / (float)(heightmap.width - 1);

    patrolGoal = RandomPointOnHeightmapRingXZ(
        homePos,
        patrolMinR,
        patrolMaxR,
        heightmap.width,
        cellSize,
        200.0f
    );
    patrolGoal.y = GetFeetPosY();

    patrolHasGoal = true;

    if (navRepathTimer >= navRepathCooldown)
    {
        navRepathTimer = 0.0f;
        if (BuildPathTo(patrolGoal))
        {
            SetPatrolState(HermitPatrolState::Move);
        }
        else
        {
            patrolHasGoal = false;
            ClearNav();
        }
    }
}

void NPC::Hermit_Patrol_Move(float dt)
{
    animIntent = AnimIntent::Walk;

    // Ensure we have a goal
    if (!patrolHasGoal)
    {
        float cellSize = terrainScale.x / (float)(heightmap.width - 1);

        patrolGoal = RandomPointOnHeightmapRingXZ(
            homePos,
            patrolMinR,
            patrolMaxR,
            heightmap.width,
            cellSize,
            200.0f
        );
        patrolGoal.y = GetFeetPosY();
        patrolHasGoal = true;
    }

    // Ensure we have a path (respect cooldown)
    if (!navHasPath && navRepathTimer >= navRepathCooldown)
    {
        navRepathTimer = 0.0f;
        if (!BuildPathTo(patrolGoal))
        {
            patrolHasGoal = false;
            SetPatrolState(HermitPatrolState::Idle);
            return;
        }
    }

    // Follow path if present
    if (navHasPath)
        FollowNavPath(dt);

    // If arrived, chill
    if (!navHasPath)
    {
        patrolHasGoal = false;
        SetPatrolState(HermitPatrolState::Idle);
        idleNoTargetTimer = 0.0f;
    }

    // stuck detection (same logic you had)
    Vector3 dd = Vector3Subtract(position, patrolLastPos);
    float movedSq = dd.x*dd.x + dd.z*dd.z;

    if (movedSq < 2.0f) patrolStuckTimer += dt;
    else patrolStuckTimer = 0.0f;

    patrolLastPos = position;

    if (patrolStuckTimer > 2.0f)
    {
        patrolStuckTimer = 0.0f;
        ClearNav();
        patrolHasGoal = false;
        SetPatrolState(HermitPatrolState::Idle);
    }
}

// -------------------------------------------------------------
// Follow brain
// -------------------------------------------------------------
void NPC::SetFollowState(HermitFollowState s)
{
    if (followState == s) return;
    followState = s;
    stateTimer = 0.0f;
}

void NPC::UpdateHermitFollowBrain(float dt)
{
    if (!hasIslandNav)
    {
        animIntent = AnimIntent::Idle;
        return;
    }

    stateTimer += dt;
    navRepathTimer += dt;

    static FollowFn fns[(int)HermitFollowState::Count] = {
        &NPC::Hermit_Follow_Move
    };

    (this->*fns[(int)followState])(dt);
}

void NPC::Hermit_Follow_Move(float dt)
{
    // If player is close enough, stop and idle
    Vector3 toP = Vector3Subtract(player.position, position);
    toP.y = 0.0f;
    float distToP = Vector3Length(toP);

    if (distToP <= followStopRadius)
    {
        ClearNav();

        if (distToP > 0.001f)
            rotationY = atan2f(toP.x, toP.z) * RAD2DEG;

        animIntent = AnimIntent::Idle;
        return;
    }

    animIntent = AnimIntent::Walk;

    Vector3 goal = player.position;
    goal.y = GetHeightAtWorldPosition(goal, heightmap, terrainScale);

    Vector3 goalDelta = Vector3Subtract(goal, followLastGoal);
    goalDelta.y = 0.0f;
    float goalMovedSq = Vector3DotProduct(goalDelta, goalDelta);

    bool needPath = !navHasPath;
    bool goalMovedFar = goalMovedSq >= (followRepathDist * followRepathDist);

    if ((needPath || goalMovedFar) && navRepathTimer >= navRepathCooldown)
    {
        navRepathTimer = 0.0f;

        if (BuildPathTo(goal))
        {
            followLastGoal = goal;
        }
        else
        {
            ClearNav();
            animIntent = AnimIntent::Idle;
            return;
        }
    }

    if (navHasPath)
        FollowNavPath(dt);
}

// -------------------------------------------------------------
// Pathfinding helpers (kept from your version)
// -------------------------------------------------------------
bool NPC::BuildPathTo(const Vector3& goalWorld)
{
    navPath.clear();
    navPathIndex = -1;
    navHasPath   = false;

    if (!hasIslandNav) return false;

    std::vector<Vector3> path;
    bool ok = HeightmapPathfinding::FindPathBFS(
        gIslandNav,
        position,
        goalWorld,
        terrainScale.y,
        path
    );

    if (!ok || path.empty()) return false;

    navPath      = std::move(path);
    navPathIndex = 0;
    navHasPath   = true;
    navRepathTimer = 0.0f;
    return true;
}

void NPC::FollowNavPath(float dt)
{
    if (!navHasPath || navPathIndex < 0 || navPathIndex >= (int)navPath.size())
    {
        navHasPath = false;
        return;
    }

    Vector3 wp = navPath[navPathIndex];
    wp.y = GetHeightAtWorldPosition(wp, heightmap, terrainScale);

    Vector3 to = Vector3Subtract(wp, position);
    to.y = 0.0f;

    float dist = Vector3Length(to);

    if (dist <= patrolStopRadius)
    {
        navPathIndex++;
        if (navPathIndex >= (int)navPath.size())
        {
            navHasPath = false;
        }
        return;
    }

    Vector3 dir = Vector3Scale(to, 1.0f / (dist + 0.0001f));

    position.x += dir.x * patrolSpeed * dt;
    position.z += dir.z * patrolSpeed * dt;

    position.y = GetCenterPosY();
    rotationY = atan2f(dir.x, dir.z) * RAD2DEG;
}

// -------------------------------------------------------------
// Animation system (your existing impl kept)
// -------------------------------------------------------------
void NPC::ResetAnim(int row, int startFrame, int frameCount, float speed)
{
    int fc = (frameCount <= 0) ? 1 : frameCount;

    const float EPS = 0.0001f;
    bool sameClip =
        (rowIndex == row) &&
        (animationStart == startFrame) &&
        (animationFrameCount == fc) &&
        (fabsf(animationSpeed - speed) < EPS);

    if (sameClip) return;

    rowIndex = row;
    animationStart      = startFrame;
    animationFrameCount = fc;

    animationSpeed = speed;
    animationTimer = 0.0f;

    currentFrame = animationStart;
    maxFrames = animationStart + animationFrameCount;
}

void NPC::PlayTalkLoopForSeconds(float seconds)
{
    if (seconds <= 0.0f) seconds = 0.05f;

    animMode = NPCAnimMode::TimedLoop;
    talkLoopTimeLeft = seconds;

    returnRow = 0;
    returnFrame = 0;
    state = NPCState::Talk;
}

void NPC::PlayTalkOneShot()
{
    animMode = NPCAnimMode::OneShot;
    returnRow = 0;
    returnFrame = 0;
    state = NPCState::Talk;
}

void NPC::UpdateAnim(float dt)
{
    if (animMode == NPCAnimMode::TimedLoop)
    {
        talkLoopTimeLeft -= dt;
        if (talkLoopTimeLeft <= 0.0f)
        {
            animMode = NPCAnimMode::None;
            state = NPCState::Idle;
            return;
        }
    }

    if (animationFrameCount <= 1) return;

    animationTimer += dt;
    if (animationTimer >= animationSpeed)
    {
        animationTimer = 0.0f;
        currentFrame++;

        int end = animationStart + animationFrameCount;

        if (currentFrame >= end)
        {
            if (animMode == NPCAnimMode::OneShot)
            {
                animMode = NPCAnimMode::None;
                state = NPCState::Idle;
            }
            else
            {
                currentFrame = animationStart;
            }
        }
    }
}

Rectangle NPC::GetSourceRect() const
{
    return Rectangle{
        (float)(currentFrame * frameWidth),
        (float)(rowIndex     * frameHeight),
        (float)frameWidth,
        (float)frameHeight
    };
}

// -------------------------------------------------------------
// Interaction + height helpers (same as yours)
// -------------------------------------------------------------
bool NPC::CanInteract(const Vector3& playerPos) const
{
    if (!isActive || !isVisible || !isInteractable) return false;

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist2 = dx*dx + dz*dz;

    return dist2 <= (interactRadius * interactRadius);
}

float NPC::GetFeetPosY()
{
    return GetHeightAtWorldPosition(position, heightmap, terrainScale);
}

float NPC::GetCenterPosY() const
{
    return GetHeightAtWorldPosition(position, heightmap, terrainScale) + (frameHeight * scale) * 0.5f;
}
