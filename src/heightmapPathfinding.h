#pragma once

#include "raylib.h"
#include <vector>

// Basic grid cell coordinate
struct HMCell
{
    int x = 0;
    int z = 0;
};

// Navigation grid built from a heightmap
struct HeightmapNavGrid
{
    int gridWidth  = 0;   // number of cells in X
    int gridHeight = 0;   // number of cells in Z

    // World-space info
    float cellWorldSizeX = 1.0f;       // size of a cell in world units (X)
    float cellWorldSizeZ = 1.0f;       // size of a cell in world units (Z)
    Vector2 worldOrigin  = {0.0f, 0.0f}; // bottom-left corner of the grid in world XZ

    // Height & walkability info (1 value per cell)
    float seaLevel = 0.4f;                   // normalized sea level threshold (0..1)
    std::vector<float> heightSamples;        // normalized heights per cell
    std::vector<bool>  walkable;             // true if cell is traversable

    bool IsInside(int gx, int gz) const;
    bool IsWalkable(int gx, int gz) const;

    // Convert cell index to world-space center position
    Vector3 CellToWorldCenter(int gx, int gz, float terrainScaleY) const;

    // Convert world-space XZ position to grid cell indices
    void WorldToCell(const Vector3& worldPos, int& outX, int& outZ) const;
};

namespace HeightmapPathfinding
{
    // Build a coarse nav grid from a grayscale heightmap.
    // gridWidth / gridHeight is *nav* resolution (e.g. 256x256, 512x512, etc.),
    // not the heightmap resolution.
    HeightmapNavGrid BuildNavGridFromHeightmap(
        const Image& heightmap,
        int gridWidth,
        int gridHeight,
        float seaLevel,
        float worldSizeX,
        float worldSizeZ
    );

    // Very basic BFS on the nav grid.
    // Returns true if a path was found, and writes world-space waypoints to outPath.
    bool FindPathBFS(
        const HeightmapNavGrid& nav,
        const Vector3& startWorld,
        const Vector3& goalWorld,
        float terrainScaleY,
        std::vector<Vector3>& outPath
    );
}
