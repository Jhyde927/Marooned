#pragma once

#include "raylib.h"
#include <vector>
#include "player.h"
#include "character.h"

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

    Vector3 undersideOffset;
    bool playerInRange; 
    bool isDead;
    bool canDie;

    // Setup
    void Init(const Vector3& rootPosition, int segmentCount, float segmentLength);

    // Game loop
    void Update(float dt, const Vector3& target, Player& player, std::vector<Character*> pirates);
    void Draw() const;

private:
    void SolveChain();
    void UpdateEmerging(float dt, Player& player);
    void UpdateIdle(float dt);
    void UpdateWindup(float dt);
    void UpdateSlam(float dt, Player& player);
    void UpdateRecover(float dt);
    void UpdateWithdraw(float dt);
    void UpdateHidden(float dt);
    void ChangeState(TentacleState newState);
    void UpdateTipHitbox();
    void ResolveTentacleVsShip();
    //Vector3 FindClosestPirate( const Vector3& tentaclePos, const Vector3& fallbackTarget, const std::vector<Character*>& pirates);


private:
    // Chain data
    std::vector<Vector3> joints;

    Vector3 rootPos;
    Vector3 surfaceRootPos;
    Vector3 hiddenRootPos;
    Vector3 tipTarget;

    float baseYaw = 0.0f;

    float segmentLength;
    float totalLength;

    // State machine
    TentacleState state;
    float stateTimer;

    // Attack target
    Vector3 attackTarget;
    Vector3 slamStartTip;
    Vector3 slamTarget;
    bool slamTargetLocked = false;
    float attackRange;

    bool pirateInRange;

    //Ermerge
    int visibleSegments;
    float emergeStepTimer;
    float emergeStepDelay;

    //Collision
    Vector3 tipHitCenter;
    float tipHitRadius;
    float deckHeight;
    bool tipHitActive;
    bool canHit;
    bool slamSoundPlayed;
};
