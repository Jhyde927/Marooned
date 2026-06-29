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

    Vector3 undersideOffset{ 0.0f, 0.0f, 0.0f };
    bool playerInRange = false;
    bool isDead = false;
    bool canDie = false;
    void Rise();   
    void Init(const Vector3& rootPosition, int segmentCount, float segmentLength);
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

private:
    std::vector<Vector3> joints;

    Vector3 rootPos{ 0.0f, 0.0f, 0.0f };
    Vector3 surfaceRootPos{ 0.0f, 0.0f, 0.0f };
    Vector3 hiddenRootPos{ 0.0f, 0.0f, 0.0f };
    Vector3 tipTarget{ 0.0f, 0.0f, 0.0f };

    float baseYaw = 0.0f;

    float segmentLength = 100.0f;
    float totalLength = 0.0f;

    TentacleState state = TentacleState::Hidden;
    float stateTimer = 0.0f;

    Vector3 attackTarget{ 3526.0f, 0.0f, 3290.0f };
    float attackRange = 3000.0f;
    Vector3 slamStartTip{ 0.0f, 0.0f, 0.0f };
    Vector3 slamTarget{ 0.0f, 0.0f, 0.0f };
    bool slamTargetLocked = false;

    bool pirateInRange = false;

    int visibleSegments = 0;
    float emergeStepTimer = 0.0f;
    float emergeStepDelay = 0.0f;

    Vector3 tipHitCenter{ 0.0f, 0.0f, 0.0f };
    float tipHitRadius = 0.0f;
    float deckHeight = 0.0f;
    bool tipHitActive = false;
    bool canHit = false;
    bool slamSoundPlayed = false;
};