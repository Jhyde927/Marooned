#pragma once
#include "raylib.h"
#include <vector>
#include "emitter.h"



// ---------------------------------------------------------
// Spider Egg States
// ---------------------------------------------------------
enum class SpiderEggState {
    Dormant,
    Awakening,   // logical state only (for now shares row with Dormant)
    Hatching,
    Destroyed,
    Husk
};

struct SpiderEgg {
    // Position / world
    Vector3 position{};
    float rotationY = 0.0f;

    Emitter gooEmitter;

    // Health
    float maxHealth = 100.0f;
    float health    = 100.0f;

    // Hatching behavior
    float hatchDelay = 3.0f;   // seconds after triggered
    float hatchTimer = 0.0f;
    bool  triggered  = false;
    bool  spiderSpawned = false;

    // Animation handling
    Texture2D texture{};
    int frameWidth   = 0;
    int frameHeight  = 0;
    int framesPerRow = 1;   // same for all rows (simplest)
    float frameTime  = 0.12f;
    float frameTimer = 0.0f;
    int currentFrame = 0;
    int currentRow   = 0;

    // Animation rows
    int rowDormant   = 0;   // Awakening will reuse this row for now
    int rowHatching  = 1;
    int rowHusk      = 2;
    int rowDestroyed = 3;

    float scale = 1.0f;
    float hitFlashTimer = 0.0f;
    float flashCooldown = 0.0f;

    SpiderEggState state = SpiderEggState::Dormant;

    // Collision
    BoundingBox collider{};
};

extern std::vector<SpiderEgg> eggs;

void DamageSpiderEgg(SpiderEgg& egg, float amount, Vector3 playerPos);
void ClearSpiderEggs();
void UpdateSpiderEggs(float dt, const Vector3& playerPos);
SpiderEgg& SpawnSpiderEgg(Vector3 pos,
                           Texture2D texture,
                           int frameW,
                           int frameH,
                           int framesPerRow,
                           float scale);
