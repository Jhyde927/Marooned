#pragma once
#include "raylib.h"
#include "inventory.h"

enum class CollectableType {
    HealthPotion,
    GoldKey,
    SilverKey,
    SkeletonKey,
    Gold,
    ManaPotion,
    Harpoon,
    // Add more types as needed
};

class Collectable {
public:
    Vector3 position;
    BoundingBox collider;
    BoundingBox hitBox;     // NEW: bullet collision (harpoon hit test)
    CollectableType type;
    Texture2D icon;
    float scale;
    int value;
    float bobTimer = 0.0f;

        // Harpoon pull
    bool  isHarpooned = false;
    float pullSpeed   = 3000.0f;   // tune this
    float stopDist    = 80.0f;     // when close enough, let pickup happen
    int   lastHarpoonBulletId = -1;

    //Texture2d needs to be not a reference, needs to be a copy for transparentDraw
    Collectable(CollectableType type, Vector3 position, Texture2D icon, float scale = 100.0f);
    void RebuildBoxes();
    //void Update(float deltaTime);
    void Update(float dt, const Vector3& playerPos);
    bool CheckPickup(const Vector3& playerPos, float pickupRadius) const;
};

