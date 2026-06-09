#pragma once

#include "raylib.h"

#include <vector>
#include "dungeonGeneration.h"

// Later you can add:
// NormalWallGray,
// NormalWallWood,
// SecretWall,
// etc.
enum class DungeonInstanceKind
{
    FloorGray,
    FloorWood
};

struct DungeonInstanceSource
{
    Vector3 position = { 0 };
    Matrix transform = {};

    DungeonInstanceKind kind = DungeonInstanceKind::FloorGray;
};

struct DungeonInstancingBatch
{
    Mesh mesh = { 0 };
    Shader shader = { 0 };
    Material material = { 0 };

    std::vector<Matrix> transforms;

    bool initialized = false;
};

// Sources generated once when the dungeon is built.
extern std::vector<DungeonInstanceSource> gDungeonInstanceSources;

// Floor batches.
extern DungeonInstancingBatch gGrayFloorInstancing;
extern DungeonInstancingBatch gWoodFloorInstancing;

// Public API.
void ClearDungeonInstancingSources();

void InitDungeonInstancing();

void AddFloorInstanceSource(const FloorTile& tile);

void BuildVisibleDungeonInstanceTransforms(Camera& camera, float maxDrawDist);

void DrawDungeonInstancedFloors();