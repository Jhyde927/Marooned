#pragma once

#include "raylib.h"
#include <vector>
#include "player.h"

enum class TentacleState
{
    Hidden,
    Emerging,
    Idle,
    Windup,
    Slam,
    Recover,
    Withdraw
};

class Tentacle
{
public:

    Tentacle();

    // Setup
    void Init(const Vector3& rootPosition, int segmentCount, float segmentLength);
    Vector3 undersideOffset;
    // Game loop
    void Update(float dt, const Vector3& target, Player& player);
    void Draw() const;

private:
    void SolveChain();
    void UpdateEmerging(float dt);
    void UpdateIdle(float dt);
    void UpdateWindup(float dt);
    void UpdateSlam(float dt);
    void UpdateRecover(float dt);
    void UpdateWithdraw(float dt);
    void UpdateHidden(float dt);
    void ChangeState(TentacleState newState);
    void UpdateTipHitbox();


private:
    // Chain data
    std::vector<Vector3> joints;

    Vector3 rootPos;
    Vector3 surfaceRootPos;
    Vector3 hiddenRootPos;
    Vector3 tipTarget;

    float segmentLength;
    float totalLength;

    

    // State machine
    TentacleState state;
    float stateTimer;

    // Attack target
    Vector3 attackTarget;
    Vector3 slamStartTip;
    Vector3 slamTarget;
    float attackRange;
    bool playerInRange;

    //Ermerge
    int visibleSegments;
    float emergeStepTimer;
    float emergeStepDelay;

    //Collision
    Vector3 tipHitCenter;
    float tipHitRadius;
    bool tipHitActive;
    bool canHit;
};
