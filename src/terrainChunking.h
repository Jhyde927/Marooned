#pragma once
#include "raylib.h"
#include <vector>

struct TerrainChunk {
    Model      model;     // GPU model for this chunk
    BoundingBox aabb;     // world-space bounds (computed from mesh)
    Vector3    center;    // world-space center (for cheap distance cull)
};

struct TerrainGrid {
    std::vector<TerrainChunk> chunks;
    int   tilesX = 0, tilesZ = 0;   // chunk grid size
    int   tileRes = 129;            // samples per side inside a chunk (e.g., 129)
    Vector3 terrainScale;           // { worldX, height, worldZ } total world span
    int   heightmapW = 0, heightmapH = 0; // original heightmap size in samples
    // Optional: a material applied to all models (set externally if you have a terrain shader)
    // Leave .materials[0] alone if you use default.
};

extern TerrainGrid terrain;

// Build from an already-loaded grayscale heightmap image (one byte per pixel).
// - terrainScale: your {16000, 200, 16000}
// - tileRes: usually 129 (keeps vertex index values < 65535)
TerrainGrid BuildTerrainGridFromHeightmap(const Image& heightmapGray, Vector3 terrainScale, int tileRes = 129, bool addSkirt = true);

// Draw with a simple distance ring (fast + good enough to start).
// maxDrawDist is horizontal (XZ) distance in world units.
void DrawTerrainGrid(const TerrainGrid& T, const Camera3D& cam, float maxDrawDist);

// Free all GPU resources owned by the grid.
void UnloadTerrainGrid(TerrainGrid& T);
