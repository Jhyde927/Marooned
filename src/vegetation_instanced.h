#pragma once

#include "raylib.h"
#include "vegetation.h"
#include <vector>
#include <string>

struct VegetationInstanceBatch
{
    std::string modelName;

    // All transforms generated for this model
    std::vector<Matrix> transforms;

    // Optional visible list rebuilt every frame for draw-distance/frustum later
    std::vector<Matrix> visibleTransforms;

    // Same positions as transforms, used for CPU culling
    std::vector<Vector3> positions;



    void Clear()
    {
        transforms.clear();
        visibleTransforms.clear();
        positions.clear();
    }
};

namespace VegetationInstanced
{
    extern bool showVeg;

    //void Init();
    // Call after normal vegetation generation, or let this call generation itself.
    void Generate();
    void InitShader();
    // Clears only instanced draw data.
    // Does not unload models/textures.
    void Clear();

    // Draw all vegetation batches with DrawMeshInstanced().
    void Draw(Camera& camera);

    // Optional for debugging
    int GetTotalInstanceCount();
    int GetVisibleInstanceCount();

    void SetShaderValues(Camera& camera);
}