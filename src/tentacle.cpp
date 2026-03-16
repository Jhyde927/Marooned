#include "tentacle.h"
#include <iostream>
#include "raylib.h"
#include "raymath.h"


Color krakenPurple = { 60, 25, 70, 255 };
Color suckerPink = { 170, 90, 140, 255 };

Tentacle::Tentacle()
    : rootPos{ 0.0f, 0.0f, 0.0f }
    , tipTarget{ 0.0f, 0.0f, 0.0f }
    , segmentLength(100.0f)
    , state(TentacleState::Hidden)
    , stateTimer(0.0f)
    , attackTarget{3526.0, 0.0, 3290.0}
    , attackRange(1600.0f)
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


void Tentacle::Init(const Vector3& rootPosition, int segmentCount, float newSegmentLength)
{
    rootPos = rootPosition;
    segmentLength = newSegmentLength;
    emergeStepTimer = 0.0;
    emergeStepDelay = 0.12;

    tipHitCenter = rootPos;
    tipHitRadius = 100.0f;
    canHit = true;

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

        if (playerInRange)
        {
            slamTarget = attackTarget;
        }
    }
}

void Tentacle::Update(float dt, const Vector3& target, Player& player)
{
    Vector3 playerPos = player.position;
    stateTimer += dt;
    attackTarget = playerInRange ? playerPos : target;
    tipHitActive = (state == TentacleState::Slam);


    float dist = Vector3Distance(rootPos, playerPos);
    playerInRange = (dist <= attackRange);

    if (state == TentacleState::Slam && stateTimer > 0.2f && canHit){
        if (CheckCollisionBoxSphere(player.GetBoundingBox(), tipHitCenter, tipHitRadius)){
            player.TakeDamage(50);
            canHit = false;
        }
    }


    if (joints.size() < 2)
        return;

    switch (state)
    {

        case TentacleState::Hidden:
        {
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

            if (stateTimer > 0.5f)
            {
                ChangeState(TentacleState::Slam);
            }
            break;
        }

        case TentacleState::Slam:
        {
            UpdateSlam(dt);

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
            UpdateEmerging(dt);
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

void Tentacle::UpdateEmerging(float dt)
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


void Tentacle::UpdateSlam(float dt)
{
    (void)dt;

    float slamDuration = 0.35f;
    float t = stateTimer / slamDuration;
    t = Clamp(t, 0.0f, 1.0f);

    Vector3 desiredTip = Vector3Lerp(slamStartTip, attackTarget, t);

    float lift = (1.0f - t) * (1.0f - t) * 350.0f;
    desiredTip.y += lift;

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
