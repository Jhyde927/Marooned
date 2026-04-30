#include "tentacle.h"
#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include "cfloat"
#include "sound_manager.h"
#include "pathfinding.h"


Color krakenPurple = { 60, 25, 70, 255 };
Color suckerPink = { 170, 90, 140, 255 };

Tentacle::Tentacle()
    : rootPos{ 0.0f, 0.0f, 0.0f }
    , tipTarget{ 0.0f, 0.0f, 0.0f }
    , segmentLength(100.0f)
    , state(TentacleState::Hidden)
    , stateTimer(0.0f)
    , attackTarget{3526.0, 0.0, 3290.0}
    , attackRange(3000.0f)
    , playerInRange(false)
{
}

void Tentacle::SolveChain()
{
    if (joints.size() < 2)
        return;

    // -------------------------
    // Backward pass:
    // each joint follows the one in front of it,
    // starting from the tip and working back to the root.
    // -------------------------
    for (int i = (int)joints.size() - 2; i >= 0; --i)
    {
        Vector3 next = joints[i + 1];
        Vector3 current = joints[i];

        Vector3 toCurrent = Vector3Subtract(current, next);
        float len = Vector3Length(toCurrent);

        if (len > 0.001f)
        {
            Vector3 dir = Vector3Scale(toCurrent, 1.0f / len);
            joints[i] = Vector3Add(next, Vector3Scale(dir, segmentLength));
        }
        else
        {
            // fallback if two joints somehow overlap exactly
            joints[i] = (Vector3){ next.x, next.y - segmentLength, next.z };
        }
    }

    // Pin the root back in place
    joints[0] = rootPos;

    // -------------------------
    // Forward pass:
    // rebuild the chain outward from the root
    // so spacing stays consistent after pinning root.
    // -------------------------
    for (size_t i = 1; i < joints.size(); ++i)
    {
        Vector3 prev = joints[i - 1];
        Vector3 current = joints[i];

        Vector3 toCurrent = Vector3Subtract(current, prev);
        float len = Vector3Length(toCurrent);

        if (len > 0.001f)
        {
            Vector3 dir = Vector3Scale(toCurrent, 1.0f / len);
            joints[i] = Vector3Add(prev, Vector3Scale(dir, segmentLength));
        }
        else
        {
            joints[i] = (Vector3){ prev.x, prev.y + segmentLength, prev.z };
        }
    }
}

void Tentacle::UpdateTipHitbox()
{
    if (joints.size() < 2)
        return;

    Vector3 start = joints[joints.size() - 2];
    Vector3 end   = joints[joints.size() - 1];

    tipHitCenter = Vector3Lerp(start, end, 0.85f);
}

void Tentacle::ResolveTentacleVsShip()
{
    if (state == TentacleState::Emerging || state == TentacleState::Withdraw || state == TentacleState::Hidden) return;


    int unclampedBaseJoints = 2; // dont clamp the bottom two segments because they are below ship height. 
    for (size_t i = 0; i < joints.size(); i++)
    {
    if (joints[i].y < deckHeight && i > unclampedBaseJoints)
        joints[i].y = deckHeight;
    }
}


void Tentacle::Init(const Vector3& rootPosition, int segmentCount, float newSegmentLength)
{
    rootPos = rootPosition;
    segmentLength = newSegmentLength;
    emergeStepTimer = 0.0;
    emergeStepDelay = 0.12;
    deckHeight = 200.0f;

    tipHitCenter = rootPos;
    tipHitRadius = 100.0f;
    canHit = true;
    isDead = false;
    canDie = true;

    joints.clear();
    joints.reserve(segmentCount);

    // Start the tentacle hanging straight down from the root.
    for (int i = 0; i < segmentCount; ++i)
    {
        Vector3 jointPos = rootPos;
        jointPos.y += i * segmentLength;
        joints.push_back(jointPos);
    }

    if (!joints.empty())
    {
        tipTarget = joints.back();
    }

    totalLength = (joints.size() - 1) * segmentLength;

    surfaceRootPos = rootPosition;
    hiddenRootPos = rootPosition;
    hiddenRootPos.y -= totalLength;
    rootPos = hiddenRootPos;
    slamSoundPlayed = false;
    attackTarget = tipTarget;
    state = TentacleState::Hidden;
    visibleSegments = 0;
    rootPos = hiddenRootPos;
    stateTimer = 0.0f;
}

void Tentacle::ChangeState(TentacleState newState)
{
    state = newState;
    stateTimer = 0.0f;

    if (newState == TentacleState::Slam)
    {
        slamStartTip = joints.back();
        canHit = true;
        slamSoundPlayed = false;
        slamTargetLocked = false;
        if (playerInRange)
        {
            slamTarget = attackTarget;
        }
    }
}


void Tentacle::Update(float dt, const Vector3& target, Player& player, std::vector<Character*> pirates)
{
    if (canDie && isDead){
        canDie = false;
        state = TentacleState::Windup; //windup before dying for dramatic effect. 
    } 
    stateTimer += dt;
    pirateInRange = false;
    playerInRange = false;

    attackTarget = target;

    float bestDistSq = FLT_MAX;
    float attackRangeSq = attackRange * attackRange;

    // Check player
    float playerDistSq = Vector3DistanceSqr(rootPos, player.position);
    if (playerDistSq <= attackRangeSq)
    {
        bestDistSq = playerDistSq;
        attackTarget = player.position;
        playerInRange = true;

        
    }

    // Check pirates
    for (Character* pirate : pirates)
    {
        if (pirate == nullptr) continue;
        Vector3 offsetPos = rootPos + Vector3{0, 500, 0};
        if (!HasWorldLineOfSight(offsetPos, pirate->position, 0.1f)) continue; 
        if (pirate->isDead) continue;   // add if you have these

        float distSq = Vector3DistanceSqr(rootPos, pirate->position);
        if (distSq > attackRangeSq) continue;

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            attackTarget = pirate->position;
            pirateInRange = true;
            playerInRange = false; // pirate is closer than player
        }
    }

    tipHitActive = (state == TentacleState::Windup);

    if (state == TentacleState::Slam && stateTimer > 0.2f && canHit){
        if (CheckCollisionBoxSphere(player.GetBoundingBox(), tipHitCenter, tipHitRadius)){
            player.TakeDamage(15);
            canHit = false;
            
        }

        for (Character* pirate : pirates){
            if (CheckCollisionBoxSphere(pirate->GetBoundingBox(), tipHitCenter, tipHitRadius)){
                pirate->TakeDamage(200);
                canHit = false;
            }
        }
    }

    


    if (joints.size() < 2)
        return;

    switch (state)
    {

        case TentacleState::Hidden:
        {
            if (isDead){
                //stayHidden
                 break;
            }
            UpdateHidden(dt);
            break;
        }

        case TentacleState::Idle:
        {
            UpdateIdle(dt);
            break;
        }

        case TentacleState::Windup:
        {

            UpdateWindup(dt);

            if (isDead && stateTimer > 2.0f){
                state = TentacleState::Withdraw;
                break;
            }

            if (stateTimer > 0.5f && !isDead)
            {

                ChangeState(TentacleState::Slam);
            }
            break;
        }

        case TentacleState::Slam:
        {
            UpdateSlam(dt, player);

            if (stateTimer > 1.0f)
            {
                ChangeState(TentacleState::Recover);
            }
            break;
        }

        case TentacleState::Recover:
        {
            UpdateRecover(dt);


            break;
        }

        case TentacleState::Emerging:
        {
            UpdateEmerging(dt, player);
                //ChangeState(TentacleState::Idle);
            break;
        }

        case TentacleState::Withdraw:
        {
            UpdateWithdraw(dt);

            break;
        }
    }

    SolveChain();
    UpdateTipHitbox();
    ResolveTentacleVsShip();
}

void Tentacle::UpdateHidden(float dt)
{
    (void)dt;

    rootPos = hiddenRootPos;
    visibleSegments = 0;

    if (playerInRange)
    {
        ChangeState(TentacleState::Emerging);
    }
}

void Tentacle::UpdateEmerging(float dt, Player& player)
{
    rootPos = Vector3Lerp(rootPos, surfaceRootPos, dt * 3.0f);

    float totalRise = surfaceRootPos.y - hiddenRootPos.y;
    float risen = rootPos.y - hiddenRootPos.y;

    float emergeT = 0.0f;
    if (fabs(totalRise) > 0.001f)
        emergeT = Clamp(risen / totalRise, 0.0f, 1.0f);

    visibleSegments = (int)ceilf(emergeT * (joints.size() - 1));

    float dist = Vector3Distance(rootPos, surfaceRootPos);
    if (dist < 5.0f && visibleSegments >= (int)joints.size() - 1)
    {

        rootPos = surfaceRootPos;
        visibleSegments = (int)joints.size() - 1;
        ChangeState(TentacleState::Idle);
    }

    Vector3 toPlayer = {
        player.position.x - rootPos.x,
        0.0f,
        player.position.z - rootPos.z
    };

    baseYaw = atan2f(toPlayer.x, toPlayer.z);

}

void Tentacle::UpdateIdle(float dt)
{
    (void)dt;

    Vector3 desiredTip = rootPos;

    float totalHeight = (joints.size() - 1) * segmentLength;

    // Neutral upright pose
    desiredTip.y += totalHeight * 0.9f;

    // Side to side sway
    desiredTip.x += sinf(stateTimer * 1.2f) * 300.0f;

    // Forward/back sway
    desiredTip.z += cosf(stateTimer * 0.9f) * 150.0f;

    // Small bob so it doesn't feel robotic
    desiredTip.y += sinf(stateTimer * 1.8f) * 300.0f;

    joints.back() = Vector3Lerp(joints.back(), desiredTip, dt * 1.0f);

    if (playerInRange && stateTimer > 1.0f)
    {
        ChangeState(TentacleState::Windup);
    }

    if (pirateInRange && stateTimer > 1.0f){
        ChangeState(TentacleState::Windup);
    }

    if (stateTimer > 10.0f && !playerInRange){
        ChangeState(TentacleState::Withdraw);
    }




}

void Tentacle::UpdateWindup(float dt)
{
    (void)dt;

    // Pull back and upward before striking
    Vector3 desiredTip = rootPos;

    desiredTip.x += 250.0f;
    desiredTip.y += totalLength;
    desiredTip.z += 100.0f;

    joints.back() = Vector3Lerp(joints.back(), desiredTip, 0.1f);
}


void Tentacle::UpdateSlam(float dt, Player& player)
{
    (void)dt;

    const float slamDuration = 0.35f;
    const float lockTime = 0.50f; // halfway through slam
    float t = stateTimer / slamDuration;
    t = Clamp(t, 0.0f, 1.0f);

    // Track target during early slam, then lock it.
    if (!slamTargetLocked)
    {
        slamTarget = attackTarget;

        if (t >= lockTime)
        {
            slamTargetLocked = true;
        }
    }

    // Target the enemy's feet / deck position, not their body center.
    Vector3 feetTarget = { slamTarget.x, deckHeight, slamTarget.z };

    // Ease the horizontal travel slightly so it doesn't feel perfectly linear.
    float moveT = t * t * (3.0f - 2.0f * t); // smoothstep

    Vector3 desiredTip = Vector3Lerp(slamStartTip, feetTarget, moveT);

    // Big early lift that collapses quickly near the end.
    const float liftHeight = 350.0f;
    float arc = sinf(t * PI);              // rises then falls
    float slamBias = powf(t, 2.5f);        // makes the end drop harder

    desiredTip.y += arc * liftHeight * (1.0f - slamBias);

    // Tiny overshoot near impact, then clamp back to deck.
    // This helps it read like it hit the floor instead of politely stopping.
    const float overshootDepth = 45.0f;

    if (t > 0.85f)
    {
        float impactT = (t - 0.85f) / 0.15f;
        impactT = Clamp(impactT, 0.0f, 1.0f);

        desiredTip.y = Lerp(desiredTip.y, deckHeight - overshootDepth, impactT);
    }

    // Final physical clamp.
    if (desiredTip.y < deckHeight)
        desiredTip.y = deckHeight;

    // Play slam sound once when the tip reaches the deck.
    if (!slamSoundPlayed && t >= 0.95f)
    {
        slamSoundPlayed = true;
        SoundManager::GetInstance().PlaySoundAtPosition(
            "tentacleSlam",
            desiredTip,
            player.position,
            0.0f,
            4000.0f
        );
    }

    joints.back() = desiredTip;
}

void Tentacle::UpdateRecover(float dt)
{
    Vector3 desiredTip = rootPos;

    float totalHeight = (joints.size() - 1) * segmentLength;

    desiredTip.y += totalHeight * 0.9f;

    joints.back() = Vector3Lerp(joints.back(), desiredTip, dt * 1.0f);

    if (!playerInRange && stateTimer > 6.0f)
    {
        ChangeState(TentacleState::Withdraw);
    }
    else if (stateTimer > 1.0f)
    {
        ChangeState(TentacleState::Idle);
    }
}

void Tentacle::UpdateWithdraw(float dt)
{
    rootPos = Vector3Lerp(rootPos, hiddenRootPos, dt * 2.0f);

    emergeStepTimer += dt;
    if (emergeStepTimer >= emergeStepDelay)
    {
        emergeStepTimer = 0.0f;

        if (visibleSegments > 0)
            visibleSegments--;
    }

    float dist = Vector3Distance(rootPos, hiddenRootPos);
    if (dist < 5.0f && visibleSegments <= 0)
    {
        rootPos = hiddenRootPos;
        ChangeState(TentacleState::Hidden);
    }
}

void Tentacle::Draw() const
{
    if (joints.size() < 2)
        return;

    if (visibleSegments <= 0)
        return;

    float baseRadius = 40.0f;
    float tipRadius  = 8.0f;

    const float gap = 2.0f;

    const size_t bodySegmentCount = joints.size() - 2;

    // visibleSegments includes the tip, so body gets one less
    int visibleBodySegments = visibleSegments - 1;
    if (visibleBodySegments < 0)
        visibleBodySegments = 0;

    size_t firstVisibleBodyIndex = bodySegmentCount;
    if ((size_t)visibleBodySegments < bodySegmentCount)
        firstVisibleBodyIndex = bodySegmentCount - visibleBodySegments;
    else
        firstVisibleBodyIndex = 0;

    // Draw visible body segments only
    for (size_t i = firstVisibleBodyIndex; i < joints.size() - 2; ++i)
    {
        Vector3 start = joints[i];
        Vector3 end   = joints[i + 1];

        Vector3 diff = Vector3Subtract(end, start);
        float len = Vector3Length(diff);
        if (len <= gap * 2.0f)
            continue;

        Vector3 dir = Vector3Scale(diff, 1.0f / len);

        Vector3 newStart = Vector3Add(start, Vector3Scale(dir, gap));
        Vector3 newEnd   = Vector3Subtract(end,   Vector3Scale(dir, gap));

        float t0 = powf((float)i / (float)(joints.size() - 1), 1.8f);
        float t1 = powf((float)(i + 1) / (float)(joints.size() - 1), 1.8f);

        float radiusStart = Lerp(baseRadius, tipRadius, t0);
        float radiusEnd   = Lerp(baseRadius, tipRadius, t1);

        Vector3 pinkOffset = undersideOffset;

        DrawCylinderEx(newStart, newEnd, radiusStart, radiusEnd, 8, krakenPurple);

        DrawCylinderEx(
            Vector3Add(newStart, pinkOffset),
            Vector3Add(newEnd, pinkOffset),
            radiusStart * 0.85f,
            radiusEnd * 0.85f,
            8,
            suckerPink
        );
    }

    // Draw tip only if at least 1 visible segment
    if (visibleSegments >= 1)
    {
        Vector3 start = joints[joints.size() - 2];
        Vector3 end   = joints[joints.size() - 1];

        Vector3 diff = Vector3Subtract(end, start);
        float len = Vector3Length(diff);

        if (len > gap * 2.0f)
        {
            Vector3 dir = Vector3Scale(diff, 1.0f / len);

            Vector3 newStart = Vector3Add(start, Vector3Scale(dir, gap));
            Vector3 newEnd   = Vector3Subtract(end,   Vector3Scale(dir, gap));

            DrawCylinderEx(newStart, newEnd, 14.0f, 2.0f, 8, krakenPurple);

            //DrawSphereWires(tipHitCenter, tipHitRadius, 8, 8, RED);  //debug hitbox
        }
    }
}
