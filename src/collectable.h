#pragma once
#include "raylib.h"
#include "inventory.h"

enum class CollectableVisualType {
    Billboard,
    Model
};

enum class CollectableType {
    HealthPotion,
    GoldKey,
    SilverKey,
    SkeletonKey,
    Gold,
    ManaPotion,
    Harpoon,
    raftMast,
    raftSail,
    raftBody,
    // Add more types as needed
};

class Collectable {
public:
    CollectableType type;
    CollectableVisualType visualType = CollectableVisualType::Billboard;
    Vector3 position;
    BoundingBox collider;
    BoundingBox hitBox;     // NEW: bullet collision (harpoon hit test)

    Texture2D icon = {};
    Model model = {};
    float scale;

    float modelScale = 1.0f;    // model size
    float modelRotation = 0.0f;
    int value;
    float bobTimer = 0.0f;
    float baseY = 160.0f;

        // Harpoon pull the collectable
    bool  isHarpooned = false;
    float pullSpeed   = 3000.0f;   // tune this
    float stopDist    = 80.0f;     // when close enough, let pickup happen
    unsigned int   lastHarpoonBulletId = -1;

    // Billboard collectable
    Collectable(
        CollectableType type,
        Vector3 position,
        Texture2D icon,
        float scale = 100.0f
    );

    // Model collectable
    Collectable(
        CollectableType type,
        Vector3 position,
        Model model,
        float modelScale,
        float modelRotation = 0.0f
    );

    //Texture2d needs to be not a reference, needs to be a copy for transparentDraw
    //Collectable(CollectableType type, Vector3 position, Texture2D icon, float scale = 100.0f);
    void RebuildBoxes();
    //void Update(float deltaTime);
    void Update(float dt, const Vector3& playerPos);
    bool CheckPickup(const Vector3& playerPos, float pickupRadius) const;
};

