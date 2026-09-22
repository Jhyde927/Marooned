#include "magicMissile.h"
#include "raymath.h"
#include "utilities.h"

MagicMissile::MagicMissile(Vector3 startPosition, Vector3 launchDirection, Vector3 targetPoint)
    : position(startPosition), targetPoint(targetPoint), speed(900.0f)
{
    float scatterAmount = RandomFloat(-0.25, 0.25);
    Vector3 scatter = {
        scatterAmount,
        scatterAmount,
        scatterAmount
    };

    travelDirection = Vector3Normalize(
        Vector3Add(launchDirection, scatter)
    );
}



void MagicMissile::Update(float deltaTime)
{
    if (!active) return;

    age += deltaTime;

    if (age >= burstTime)
    {
        Vector3 toTarget = Vector3Subtract(targetPoint, position);

        if (Vector3LengthSqr(toTarget) > 0.001f)
        {
            Vector3 desiredDirection = Vector3Normalize(toTarget);
            float turnAmount = Clamp(turnSpeed * deltaTime, 0.0f, 1.0f);

            travelDirection = Vector3Normalize(
                Vector3Lerp(travelDirection, desiredDirection, turnAmount)
            );
        }
    }

    position = Vector3Add(
        position,
        Vector3Scale(travelDirection, speed * deltaTime)
    );

    position = Vector3Add(
        position,
        Vector3Scale(travelDirection, speed * deltaTime)
    );

    //Destroy on arrival at target
    // constexpr float arrivalRadius = 100.0f;
    // if (Vector3DistanceSqr(position, targetPoint) <= arrivalRadius * arrivalRadius)
    // {
    //     active = false;
    //     return;
    // }

    lifetime -= deltaTime;
    if (lifetime <= 0.0f) active = false;

    


}