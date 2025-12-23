#include "collectable.h"
#include "raymath.h"
#include <iostream>
#include "player.h"

// NEW constructor with icon and scale
Collectable::Collectable(CollectableType type, Vector3 position, Texture2D icon, float scale)
    : type(type), position(position), icon(icon), scale(scale)
    
{
    collider = {
        position - Vector3{20, 0, 20},
        position + Vector3{20, 60, 20}
    };
}


inline static BoundingBox MakeBoxCentered(Vector3 p, Vector3 half)
{
    return BoundingBox{
        { p.x - half.x, p.y - half.y, p.z - half.z },
        { p.x + half.x, p.y + half.y, p.z + half.z }
    };
}

void Collectable::RebuildBoxes()
{
    // Harpoon hit box: small target, tweak as needed
    Vector3 halfHit = { 50.0f, 50.0f, 50.0f };

    // If collider is currently used as a physical box, keep it too
    // Otherwise you can make it the same as hitBox.
    Vector3 halfCol = { 60.0f, 60.0f, 60.0f };

    hitBox   = MakeBoxCentered(position, halfHit);
    collider = MakeBoxCentered(position, halfCol);
}



void Collectable::Update(float dt, const Vector3& playerPos) {
    bobTimer += dt;
    float bobAmount = sinf(bobTimer) * 0.1f;
    position.y += bobAmount;


    if (isHarpooned)
    {
        Vector3 toPlayer = Vector3Subtract(playerPos, position);
        float dist = Vector3Length(toPlayer);

        if (dist > stopDist)
        {
            Vector3 dir = Vector3Scale(toPlayer, 1.0f / dist);
            float step = pullSpeed * dt;
            if (step > dist - stopDist) step = dist - stopDist;

            position = Vector3Add(position, Vector3Scale(dir, step));
        }
        // If dist <= stopDist, we just leave it there and pickup logic grabs it
    }

    RebuildBoxes();

    // Update collider height to match
    // collider.min.y = position.y;
    // collider.max.y = position.y + 60.0f;
}



bool Collectable::CheckPickup(const Vector3& playerPos, float pickupRadius) const {
    return Vector3Distance(playerPos, position) < pickupRadius;
}
