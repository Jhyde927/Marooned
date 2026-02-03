#pragma once

#include "raylib.h"
#include "player.h"

enum class BoxType
{
    WoodenCrate,
    HeavyCrate,
    ExplosiveCrate   // future
};

enum class BoxState
{
    OnGround,
    Carried
};

class Box
{
public:
    // --- Identity ---
    BoxType type = BoxType::WoodenCrate;
    BoxState state = BoxState::OnGround;

    // --- Transform ---
    Vector3 position = {0, 0, 0};
    Vector3 startPosition = {0, 0, 0};
    float   rotationY = 0.0f;
    float   scale     = 1.0f;

    // --- Interaction ---
    float pickupRadius = 250.0f;
    bool  canBeCarried = true;

    // --- Collision ---
    BoundingBox bounds;

    // --- Carry behavior ---
    Vector3 carryOffset {0.0f, 75.0f, 200.0f}; // waist height + forward
    

    // --- Environment interaction ---
    bool activatesPressurePads = true;

public:
    Box(BoxType t, Vector3 pos);


    void Update(float dt,
                Vector3 PlayerPos,
                Vector3 playerForward,
                Vector2 playerRot,
                bool interactPressed,
                bool dropPressed,
                Vector3 dropTileCenter);


    // --- Interaction API ---
    bool CanPickup(Vector3 playerPos) const;
    void Pickup();
    void DropToTileCenter(Vector3 tileCenter);

private:
    void UpdateBounds();
};
