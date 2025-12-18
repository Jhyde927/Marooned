#pragma once
#include "raylib.h"
#include "inventory.h"

enum class CollectableType {
    HealthPotion,
    GoldKey,
    SilverKey,
    Gold,
    ManaPotion,
    Harpoon,
    // Add more types as needed
};

class Collectable {
public:
    Vector3 position;
    BoundingBox collider;
    CollectableType type;
    Texture2D icon;
    float scale;
    int value;
    float bobTimer = 0.0f;

    //Texture2d needs to be not a reference, needs to be a copy for transparentDraw
    Collectable(CollectableType type, Vector3 position, Texture2D icon, float scale = 100.0f);

    void Update(float deltaTime);
    
    bool CheckPickup(const Vector3& playerPos, float pickupRadius) const;
};

