#pragma once

#include "raylib.h"
#include "transparentDraw.h" // BillboardDrawRequest, billboardRequests, Billboard_FixedFlat

#include <vector>

namespace Grass
{
    struct GrassCard
    {
        Vector3 position = { 0, 0, 0 }; // ground position
        Vector2 size = { 100, 140 };
        float rotationY = 0.0f;         // radians, because DrawFlatWeb uses MatrixRotateY(rotationY)
        int textureIndex = 0;
        Color tint = WHITE;
    };

    void Clear();

    void GenerateFromHeightmap(
        Image& heightmap,
        Vector3 terrainScale,
        float grassSpacing,
        float heightThreshold,
        int maxGrassCards
    );

    void Gather(const Camera& camera);

    int GetCount();
}