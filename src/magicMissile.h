#pragma once

#include "raylib.h"
#include "sound_manager.h"

class MagicMissile
{
public:
    //MagicMissile(Vector3 startPosition, Vector3 launchDirection, Vector3 targetPoint);

    MagicMissile(Vector3 startPosition, Vector3 launchDirection, Vector3 targetPoint, bool playWoosh);

    void Update(float deltaTime);
    void InitParameters();


    Vector3 position;
    Vector3 travelDirection;
    Vector3 targetPoint;
    float speed;
    float lifetime;
    bool active;
    bool targetAcquired;


    float age;
    float burstTime;
    float turnSpeed;
    float arrivalRadius;
    float particleAccumulator = 1.0f;
    float collisionRadius;
    float damage;

    float retargetTimer;
    float retargetInterval;

    bool playWoosh = false;
    bool wooshStarted = false;
    SoundInstanceId wooshId = 0;

    float corkscrewPhase;
    float corkscrewRadius;
    float corkscrewAngularSpeed;

    bool carriesLight = false;


private:
    bool HandleWorldCollision(
        Vector3 previousPosition,
        Vector3 nextPosition
    );

    bool HandleEnemyCollision();

    void DestroyOnImpact(Vector3 impactPosition);

};