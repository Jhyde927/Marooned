#pragma once
#include "raylib.h"
#include "inventory.h"

enum class CollectableType {
    HealthPotion,
    Key,
    Gold,
    // Add more types as needed
};

class Collectable {
public:
    Vector3 position;
    BoundingBox collider;
    CollectableType type;
    int value;
    float bobTimer = 0.0f;

    Collectable(CollectableType type, Vector3 position);

    void Update(float deltaTime);
    void Draw(Texture2D icon, const Camera& camera, float scale) const;
    bool CheckPickup(const Vector3& playerPos, float pickupRadius) const;
};

