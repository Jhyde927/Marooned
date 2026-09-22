#pragma once

#include "raylib.h"

class MagicMissile
{
public:
    MagicMissile(Vector3 startPosition, Vector3 launchDirection, Vector3 targetPoint);

    void Update(float deltaTime);

    Vector3 position;
    Vector3 travelDirection;
    Vector3 targetPoint;
    float speed;
    float lifetime = 3.0f;
    bool active = true;
    bool targetAcquired = false;

    float age = 0.0f;
    float burstTime = 0.2f;
    float turnSpeed = 4.0f;


};