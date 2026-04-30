#pragma once

#include "raylib.h"
#include <vector>
#include "character.h"


enum class SpawnerType
{
    Pirate,
    Skeleton
};

struct Spawner
{
    Vector3 position{};
    SpawnerType type = SpawnerType::Pirate;

    float cooldown = 8.0f;
    float timer = 0.0f;

    bool showPortal = false;
    float portalTimer = 0.0f;
    bool enabled = true;

    
};

namespace SpawnManager
{
    extern std::vector<Spawner> spawners;
    extern int maxAlive;
    extern bool startSpawning;
    int CountAliveSpawnedByType(CharacterType type);
    void Clear();
    void Update(float dt);
}