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
    FloorWood,

    WallStone,
    WallWood,
    WallWoodHalf
};

struct DungeonInstanceSource
{
    Vector3 position = { 0, 0, 0 };
    Matrix transform = {};

    DungeonInstanceKind kind = DungeonInstanceKind::FloorGray;
};

struct DungeonInstancingBatch
{
    Mesh mesh = {};
    Shader shader;
    Material material;

    std::vector<Matrix> transforms;
    DungeonInstanceKind kind = DungeonInstanceKind::FloorGray;

    bool initialized = false;
};

// Sources generated once when the dungeon is built.
extern std::vector<DungeonInstanceSource> gDungeonInstanceSources;

// Floor batches.
extern DungeonInstancingBatch gGrayFloorInstancing;
extern DungeonInstancingBatch gWoodFloorInstancing;

extern DungeonInstancingBatch gStoneWallInstancing;
extern DungeonInstancingBatch gWoodWallInstancing;
extern DungeonInstancingBatch gWoodHalfWallInstancing;

// Public API.
void ClearDungeonInstancingSources();

void InitDungeonInstancing();
void AddFloorInstanceSource(Vector3 position, FloorType floorType);
//void AddFloorInstanceSource(const FloorTile& tile);
void AddWallInstanceSource(const WallInstance& wall);
void BuildVisibleDungeonInstanceTransforms(Camera& camera, float maxDrawDist);

void DrawDungeonInstancedFloors();
void DrawDungeonInstancedWalls();