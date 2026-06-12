#pragma once

#include "raylib.h"
#include <vector>
#include <string>


struct BillboardDrawRequest;

enum class DungeonPropType
{
    None,
    SpiderWebCorner,
    TableSet,
    CratePile,
    Stool,
    BonePile,
    WallBanner,
    Candelabra,

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
    std::string textureName;
    std::string modelName;
    Color tint = WHITE;
    bool active = true;
    bool hasCollision = false;

    float ambientBoost = 0.20f;
    float maxBrightness = 1.25f;
};

extern std::vector<DungeonProp> gDungeonProps;

void ClearDungeonProps();
void  DrawDungeonPropModels(Camera& camera);
void GenerateProps(float baseY);

void SpawnDungeonProp(
    DungeonPropType type,
    Vector3 position,
    float rotationY = 0.0f
);

void GenerateAutoCornerProps(float baseY);

// Optional first-pass generator.
void GenerateDungeonPropsForCurrentLevel();

// This feeds props into existing transparent/draw request pipeline.
void GatherDungeonPropDrawRequests(
    const Vector3& cameraPos,
    std::vector<BillboardDrawRequest>& requests
);

DungeonProp MakeDefaultProp(DungeonPropType type, Vector3 position, float rotationY);