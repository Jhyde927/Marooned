#pragma once

#include "raylib.h"
#include <vector>
#include <string>


struct BillboardDrawRequest;

enum class DungeonPropType
{
    None,
    SpiderWebCorner,
    HangingWeb,
    WallBanner,
    TableSet,
    CratePile, 

};

enum class DungeonPropRenderMode
{
    Billboard,      // faces camera
    FixedFlat,      // one fixed vertical quad
    CrossQuads,     // two fixed vertical quads in an X
    WallQuad,       // flat against a wall
    FloorQuad,      // flat on floor, later
    Model           // later: table/chair combo, butcher table, etc.
};

struct DungeonProp
{
    DungeonPropType type = DungeonPropType::SpiderWebCorner;
    DungeonPropRenderMode renderMode = DungeonPropRenderMode::CrossQuads;

    Vector3 position = { 0, 0, 0 };
    float rotationY = 0.0f;

    Vector2 size = { 200.0f, 200.0f };

    Vector3 modelSize = {100.0f, 100.0f, 100.0f};

    // Use a string key so ResourceManager owns the texture.
    std::string textureName;

    // Later, for real 3D props.
    std::string modelName;

    Color tint = WHITE;

    bool active = true;

    // Keep false for decoration props.
    bool hasCollision = false;
};

extern std::vector<DungeonProp> gDungeonProps;

void ClearDungeonProps();
void  DrawDungeonPropModels();
void GenerateProps(float baseY);

void SpawnDungeonProp(
    DungeonPropType type,
    Vector3 position,
    float rotationY = 0.0f
);

// Optional first-pass generator.
void GenerateDungeonPropsForCurrentLevel();

// This feeds props into existing transparent/draw request pipeline.
void GatherDungeonPropDrawRequests(
    const Vector3& cameraPos,
    std::vector<BillboardDrawRequest>& requests
);

DungeonProp MakeDefaultProp(DungeonPropType type, Vector3 position, float rotationY);