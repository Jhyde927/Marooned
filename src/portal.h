#pragma once
#include "raylib.h"
#include <vector>

struct PortalRenderState
{
    Camera3D camera{};
    RenderTexture2D target{};
    int sourcePortalIndex = -1;
    int destPortalIndex = -1;
    int groupID = -1;
    bool initialized = false;
    bool enabled = false;
};

// A single portal tile in the dungeon
struct Portal
{
    Vector3 position;   // world position (center of tile)
    BoundingBox box;    // trigger volume
    int tileX;
    int tileY;
    int groupID;
    Color tint;
};

// Holds and manages all portals in the current dungeon
namespace PortalSystem
{
    // Generate portals from dungeon pixel data
    void GenerateFromDungeon(
        const Image& dungeonImage,
        int dungeonWidth,
        int dungeonHeight,
        float tileSize,
        float baseY
    );

    // Update portal logic (player collision)
    void Update(Vector3& playerPos, float playerRadius, float dt);

    // Debug draw (red cubes for now)
    void DrawDebug(Camera& camera);

    // Clear all portals (when unloading dungeon)
    void Clear();

    // Portal render proof-of-concept
    void InitPortalRender(int width = 512, int height = 512);
    void ShutdownPortalRender();
    bool SetTestRenderPairFromGroup(int groupID);
    void UpdatePortalRenderCamera(const Camera3D& mainCamera);
    void RenderPortalView(void (*drawSceneFunc)(const Camera3D&));
    const PortalRenderState& GetRenderState();
}