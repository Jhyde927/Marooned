#include "NPC.h"
#include <cmath>
#include "world.h"
#include "sound_manager.h"
#include "utilities.h"

void NPC::Init(Texture2D sheet, int frameW, int frameH, float sc)
{
    texture = sheet;
    frameWidth  = frameW;
    frameHeight = frameH;
    scale = sc;

    homePos = position;
    patrolLastPos = position;

    // Default: idle row 0, single frame
    ResetAnim(/*row*/0, /*start*/0, /*count*/1, /*speed*/0.15f);
}



void NPC::ChangeState(NPCState newState)
{
    if (state == newState) return;

    state = newState;
    stateTimer = 0.0f;

    // per-state entry init
    switch (state)
    {
        case NPCState::Aim:
            aimTimer = 0.0f;
            firedThisCycle = false;
            break;

        case NPCState::Fire:
            // important: ensure fire happens once and timer starts from 0
            firedThisCycle = false;
            // stateTimer already reset above, used as "fire hold timer"
            break;

        case NPCState::Idle:
            targetDropTimer = 0.0f; // optional
            break;

        case NPCState::Patrol:

            break;



        default:
            break;
    }
}


static bool IsWithinAimConeXZ(const Vector3& hermitPos,
                             float hermitYawDeg,
                             const Vector3& enemyPos,
                             float maxDegrees)
{
    // Hermit forward direction in XZ (yaw only)
    float yaw = hermitYawDeg * DEG2RAD;
    Vector3 forward = { sinf(yaw), 0.0f, cosf(yaw) }; // same convention as your player cam

    // Direction to enemy in XZ
    Vector3 toE = Vector3Subtract(enemyPos, hermitPos);
    toE.y = 0.0f;

    float lenSq = Vector3DotProduct(toE, toE);
    if (lenSq < 0.0001f) return true; // basically on top of hermit

    toE = Vector3Scale(toE, 1.0f / sqrtf(lenSq)); // normalize

    // Compare against cosine threshold
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

void NPC::SetFlipX(){
    if (state != NPCState::Fire){
        // --- stable flipX with dead-zone + cooldown ---
        float yaw = rotationY * DEG2RAD;
        Vector3 forward = { sinf(yaw), 0.0f, cosf(yaw) };

        Vector3 toT = Vector3Subtract(target->position, position);
        toT.y = 0.0f;

        // signed left/right measure (same value used inside IsTargetOnLeftOfYaw)
        float side = forward.x * toT.z - forward.z * toT.x;

        // default: keep current orientation
        bool wantFlip = flipX;

        // dead-zone: only change if target is CLEARLY on one side
        if (side >  flipDeadZone) wantFlip = true;
        if (side < -flipDeadZone) wantFlip = false;

        // apply cooldown
        if (wantFlip != flipX && flipCooldownLeft <= 0.0f)
        {
            flipX = !wantFlip; //opposite
            flipCooldownLeft = flipCooldown;
        }  
    }   
}

bool NPC::BuildPathTo(const Vector3& goalWorld)
{
    navPath.clear();
    navPathIndex = -1;
    navHasPath   = false;

    if (!hasIslandNav) return false; // if you have a global, use that
    // or if you store it: if (!hasIslandNavNPC) return false;

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
        std::cout << "NO PATH!\n";
        navHasPath = false;
        return;
    }

    Vector3 wp = navPath[navPathIndex];

    // Keep Y grounded to terrain each step (optional, but usually good on heightmaps)
    wp.y = GetHeightAtWorldPosition(wp, heightmap, terrainScale);

    Vector3 to = Vector3Subtract(wp, position);
    to.y = 0.0f;

    float dist = Vector3Length(to);


    if (dist <= patrolStopRadius)
    {

        navPathIndex++;
        if (navPathIndex >= (int)navPath.size())
        {

            navHasPath = false; // arrived
 
        }
        return;

    }

    Vector3 dir = Vector3Scale(to, 1.0f / (dist + 0.0001f));
    position.x += dir.x * patrolSpeed * dt;
    position.z += dir.z * patrolSpeed * dt;
    


    // keep on terrain
    position.y = GetHeightAtWorldPosition(position, heightmap, terrainScale) + 100;

    // update facing (optional; you already do rotation for turret)
    rotationY = atan2f(dir.x, dir.z) * RAD2DEG;
}

void NPC::UpdateHermitPatrol(float dt)
{


    // only patrol on island
    if (!hasIslandNav){

        return;
    } 

    // count up time between BFS calls
    navRepathTimer += dt;

    switch (state)
    {
        case NPCState::Idle:
        {
            // idleNoTargetTimer should be managed by the turret/no-target logic.
            // But as a simple self-contained approach, do it here too:
            idleNoTargetTimer += dt;
            if (idleNoTargetTimer >= patrolIdleDelay)
            {
                idleNoTargetTimer = 0.0f;

                float cellSize = terrainScale.x / (float)(heightmap.width - 1);
                // Choose a wander goal around home
                //patrolGoal = player.position;
                patrolGoal = RandomPointOnHeightmapRingXZ(
                    homePos,
                    patrolMinR,
                    patrolMaxR,
                    heightmap.width,
                    cellSize,
                    200.0f
                );
                patrolGoal.y = GetHeightAtWorldPosition(patrolGoal, heightmap, terrainScale);

                patrolHasGoal = true;

                // Try build path (respect repath cooldown)
                if (navRepathTimer >= navRepathCooldown)
                {
                    navRepathTimer = 0.0f;
                    if (BuildPathTo(patrolGoal)){
                        ChangeState(NPCState::Patrol);
                    }
             
                    else
                    {
                        // failed path; try again later
                        patrolHasGoal = false;
                        navHasPath = false;
                        state = NPCState::Idle;
                    }
                }
            }
        } break;

        case NPCState::Patrol:
        {

            // If no path, try to build one (occasionally)
            if (!navHasPath)
            {
                // If we lost our goal, pick a new one
                if (!patrolHasGoal)
                {
                    //patrolGoal = player.position;
                    patrolGoal = RandomPointOnHeightmapRingXZ(
                        homePos,
                        patrolMinR,
                        patrolMaxR,
                        heightmap.width,
                        terrainScale.x / (float)(heightmap.width - 1),
                        200.0f
                    );
                    patrolGoal.y = GetHeightAtWorldPosition(patrolGoal, heightmap, terrainScale);
                    patrolHasGoal = true;

                }

                if (navRepathTimer >= navRepathCooldown)
                {
                    navRepathTimer = 0.0f;
                    if (!BuildPathTo(patrolGoal))
                    {
                        // fail -> drop goal and go idle (try later)
                        patrolHasGoal = false;
                        state = NPCState::Idle;
                        break;
                    }
                }
            }

            

            // Follow the path
            FollowNavPath(dt);

            // If we arrived, chill (back to Idle, restart the 10s timer)
            if (!navHasPath)
            {
                patrolHasGoal = false;
                state = NPCState::Idle;
                idleNoTargetTimer = 0.0f;
            }

            // Optional stuck detection: if barely moved for ~2s, drop path
            {
                Vector3 d = Vector3Subtract(position, patrolLastPos);
                float movedSq = d.x*d.x + d.z*d.z;

                if (movedSq < 2.0f) patrolStuckTimer += dt;
                else patrolStuckTimer = 0.0f;

                patrolLastPos = position;

                if (patrolStuckTimer > 2.0f)
                {
                    patrolStuckTimer = 0.0f;
                    navHasPath = false;
                    patrolHasGoal = false;
                    state = NPCState::Idle;
                }
            }
        } break;

        default:
            break;
    }
}


void NPC::Update(float dt)
{

    if (!isActive) return;

    if (state == NPCState::Talk && !CanInteract(player.position)){
        state = NPCState::Idle;
    }

    if (type == NPCType::Hermit && state != NPCState::Talk)
    {
        UpdateHermitTurret(dt, enemyPtrs);

        // If turret isn't actively aiming/firing, allow idle->patrol behavior
        if (state == NPCState::Idle || state == NPCState::Patrol)
            UpdateHermitPatrol(dt);
    }


    previousPosition = position;

    const int FRONT_ROW_IDLE = 0;  // your existing front idle/aim/fire row
    const int FRONT_ROW_AIM  = 2;
    const int FRONT_ROW_FIRE = 2;
    const int REAR_ROW_FIRE  = 4;
    const int REAR_ROW_IDLE  = 3;  // new rear row you made
    const int REAR_ROW_AIM   = 3;


    Vector3 viewerPos = player.position; // or camera.position (better in FPS)

    bool showFront = ViewerIsInFrontOfNPC(position, rotationY, viewerPos);

    int rowIdle = showFront ? FRONT_ROW_IDLE : REAR_ROW_IDLE;
    int rowAim  = showFront ? FRONT_ROW_AIM  : REAR_ROW_AIM;
    int rowFire = showFront ? FRONT_ROW_FIRE : REAR_ROW_FIRE;


    if (type == NPCType::Hermit)
    {
        switch (state)
        {
            case NPCState::Idle:
                // row 0 frame 0
                ResetAnim(rowIdle, 0, 1, 0.10f);
                break;

            case NPCState::Aim:
                // row 2 frame 1
                ResetAnim(rowAim, 1, 1, 0.10f);
                break;

            case NPCState::Fire:
                // row 2 frame 2 (you might want this to be a one-shot feel)
                ResetAnim(rowFire, 2, 1, 0.1f);
                break;

            case NPCState::Talk:
                ResetAnim(1, 0, 3, 0.5);
                break;

            case NPCState::Patrol:
                // pick a walking row or reuse idle if you don’t have walk frames
                ResetAnim(rowIdle, 0, 1, 0.10f);
                break;

            default:
                break;
        }
    }

    UpdateAnim(dt);
}

void NPC::ResetAnim(int row, int startFrame, int frameCount, float speed)
{
    // sanitize
    int fc = (frameCount <= 0) ? 1 : frameCount;

    // If we're already playing this exact clip, do nothing.
    // (Use a small epsilon for float compares.)
    const float EPS = 0.0001f;
    bool sameClip =
        (rowIndex == row) &&
        (animationStart == startFrame) &&
        (animationFrameCount == fc) &&
        (fabsf(animationSpeed - speed) < EPS);

    if (sameClip)
        return;

    // Otherwise switch clips (full reset)
    rowIndex = row;

    animationStart      = startFrame;
    animationFrameCount = fc;

    animationSpeed = speed;
    animationTimer = 0.0f;

    currentFrame = animationStart;

    // Optional convenience
    maxFrames = animationStart + animationFrameCount;
}


void NPC::PlayTalkLoopForSeconds(float seconds)
{
    // Safety
    if (seconds <= 0.0f) seconds = 0.05f;

    animMode = NPCAnimMode::TimedLoop;
    talkLoopTimeLeft = seconds;

    // return to idle pose when done
    returnRow = 0;
    returnFrame = 0;
    state = NPCState::Talk;
    // start (or restart) the looping talk anim
    //ResetAnim(/*row*/1, /*start*/0, /*count*/3, /*speed*/0.8f);
}



void NPC::UpdateAnim(float dt)
{
    // Timed loop countdown (only in that mode)
    if (animMode == NPCAnimMode::TimedLoop)
    {

        talkLoopTimeLeft -= dt;
        if (talkLoopTimeLeft <= 0.0f)
        {
            // stop anim + return to idle pose
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
                // finish one-shot
                animMode = NPCAnimMode::None;
                state = NPCState::Idle;

            }
            else
            {
                // normal looping (this includes TimedLoop mode)
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

void NPC::PlayTalkOneShot()
{
    // Play talk gesture once, then return to idle pose
    animMode = NPCAnimMode::OneShot;
    returnRow = 0;
    returnFrame = 0;
    state = NPCState::Talk;
    //ResetAnim(/*row*/1, /*start*/0, /*count*/3, /*speed*/0.8f);
}




bool NPC::CanInteract(const Vector3& playerPos) const
{
    if (!isActive || !isVisible || !isInteractable) return false;

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist2 = dx*dx + dz*dz;

    return dist2 <= (interactRadius * interactRadius);
}

float NPC::GetFeetPosY(){
    return GetHeightAtWorldPosition(position, heightmap, terrainScale);
}

static bool PtrStillListed(Character* p, const std::vector<Character*>& enemyPtrs)
{
    if (!p) return false;
    for (Character* e : enemyPtrs)
    {
        if (e == p) return true;
    }
    return false;
}

Character* NPC::AcquireClosestEnemy(const std::vector<Character*>& enemyPtrs)
{
    // If current target is still a valid candidate, keep it
    if (target && PtrStillListed(target, enemyPtrs) &&
        !target->isDead && target->currentHealth > 0)
    {
        Vector3 d = Vector3Subtract(target->position, position);
        float distSq = Vector3DotProduct(d, d);
        if (distSq <= visionRadius * visionRadius)
            return target;
    }

    Character* best = nullptr;
    float bestDistSq = visionRadius * visionRadius;

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


void NPC::UpdateHermitTurret(float dt, const std::vector<Character*>& enemyPtrs)
{

    // update state timer (time spent in current state)
    stateTimer += dt;

    // cooldown
    fireCooldownLeft -= dt;
    if (fireCooldownLeft < 0.0f) fireCooldownLeft = 0.0f;

    if (flipCooldownLeft > 0.0f) flipCooldownLeft -= dt;
    if (flipCooldownLeft < 0.0f) flipCooldownLeft = 0.0f;

    if (target && !PtrStillListed(target, enemyPtrs))
    {
        target = nullptr;
        targetDropTimer = 0.0f;
        aimTimer = 0.0f;
        firedThisCycle = false;

        if (state != NPCState::Talk)
            state = NPCState::Idle;
    }



    auto TargetIsValid = [&]() -> bool {
        if (!target) return false;
        if (!PtrStillListed(target, enemyPtrs)) return false; // CRITICAL

        if (target->isDead) return false;
        if (target->currentHealth <= 0) return false;

        Vector3 d = Vector3Subtract(target->position, position);
        float distSq = Vector3DotProduct(d, d);
        return distSq <= (visionRadius * visionRadius);
    };

    // If we lost target, drop / reacquire (but don't interrupt Fire hold)
    // Important: keep Fire visible even if target dies the instant you shoot.
    if (state != NPCState::Fire)
    {
        if (!TargetIsValid())
        {
            if (target)
            {
                targetDropTimer += dt;
                if (targetDropTimer >= targetDropGrace)
                {
                    target = nullptr;
                    targetDropTimer = 0.0f;
                }
            }
            else
            {
                targetDropTimer = 0.0f;
            }

            if (!target)
            {
                target = AcquireClosestEnemy(enemyPtrs);
                if (target){
                    ChangeState(NPCState::Aim);
                } 
                else
                {
                    // IMPORTANT: don't stomp Patrol when there are no enemies
                    if (state == NPCState::Aim || state == NPCState::Fire)
                        ChangeState(NPCState::Idle);
                    // if we're Idle or Patrol, just leave state alone
                }
                return;
            }
            else
            {
                // in grace: don't aim at nothing
                if (state == NPCState::Aim || state == NPCState::Fire)
                    ChangeState(NPCState::Idle);
                return;
            }
        }
        else
        {
            targetDropTimer = 0.0f;
        }
    }

    switch (state)
    {
        case NPCState::Idle:
        {
            if (TargetIsValid())
                ChangeState(NPCState::Aim);
        } break;

        case NPCState::Aim:
        {
            if (!TargetIsValid())
            {
                ChangeState(NPCState::Idle);
                break;
            }

            Vector3 d = Vector3Subtract(target->position, position);
            rotationY = atan2f(d.x, d.z) * RAD2DEG;

            float distSq = Vector3DotProduct(d, d);

            if (fireCooldownLeft > 0.0f) break;
            if (distSq > fireRadiusSq) break;

            SetFlipX();

            

            aimTimer += dt;
            if (aimTimer >= aimDuration)
            {
                ChangeState(NPCState::Fire);
            }
        } break;

        case NPCState::Fire:
        {
            // FIRE EVENT: do once, immediately on entering Fire
            if (!firedThisCycle && TargetIsValid())
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

            // HOLD the firing frame for fireHoldDuration seconds
            if (stateTimer >= fireHoldDuration)
            {
                // After hold, go back to aim if something is still around, else idle.
                if (AcquireClosestEnemy(enemyPtrs))
                    ChangeState(NPCState::Aim);
                else
                {
                    target = nullptr;
                    ChangeState(NPCState::Idle);
                }
            }
        } break;

        default:
            ChangeState(NPCState::Idle);
            break;
    }
}

