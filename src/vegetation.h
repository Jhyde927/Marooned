// vegetation.h

#pragma once

#include "raylib.h"
#include <vector>
#include <stdint.h>

enum class TreeType : uint8_t
{
    Big,
    Medium,
    Swamp,
    Dead,
    COUNT
};


struct TreeInstance {
    Vector3 position;
    float rotationY;
    float scale;
    float yOffset;
    float xOffset;
    float zOffset;
    bool useAltModel;
    float cullFactor;

    // Collision info (simple cylinder)
    float colliderRadius = 80.0f;   // trunk radius
    float colliderHeight = 300.0f;  // height of trunk to base of leaves
    TreeType type;

};

struct BushInstance {
    Vector3 position;
    Model model;
    float scale;
    float rotationY;
    float yOffset;
    float xOffset;
    float zOffset;
    float cullFactor;
};


extern std::vector<TreeInstance> trees;
extern std::vector<BushInstance> bushes;
extern std::vector<const TreeInstance*> sortedTrees;


void generateVegetation();
void RemoveAllVegetation();
void sortTrees(Camera& camera);
BoundingBox GetTreeAABB(const TreeInstance& t);

std::vector<TreeInstance> GenerateTrees(Image& heightmap, unsigned char* pixels, Vector3 terrainScale,
                                        float treeSpacing, float minTreeSpacing, float treeHeightThreshold);

std::vector<TreeInstance> FilterTreesAboveHeightThreshold(const std::vector<TreeInstance>& inputTrees, Image& heightmap,
                                                          unsigned char* pixels, Vector3 terrainScale,
                                                          float treeHeightThreshold);
void DrawTrees(const std::vector<TreeInstance>& trees, Camera& camera);
void DrawBushes(const std::vector<BushInstance>& bushes);

std::vector<BushInstance> GenerateBushes(Image& heightmap, unsigned char* pixels, Vector3 terrainScale,
                                         float bushSpacing, float heightThreshold);
std::vector<BushInstance> FilterBushsAboveHeightThreshold(const std::vector<BushInstance>& inputTrees, Image& heightmap,
                                                          unsigned char* pixels, Vector3 terrainScale,
                                                          float treeHeightThreshold);
template<typename T, typename GetPosFn>
std::vector<T> FilterInstancesOnLand(
    const std::vector<T>& input,
    GetPosFn&& getPos, 
    const Image& heightmap,
    const unsigned char* pixels,
    const Vector3& terrainScale,
    float seaLevelWorldY,
    float shoreMarginWorldY,
    float trunkRadiusWorld
);

// std::vector<TreeInstance> FilterTreesOnLand(
//     const std::vector<TreeInstance>& input,
//     const Image& heightmap,
//     const unsigned char* pixels,   // 1 byte per pixel height
//     const Vector3& terrainScale,
//     float seaLevelWorldY,          // your water plane height in world units
//     float shoreMarginWorldY,       // how far above sea level a tree must be (e.g. 40..150)
//     float trunkRadiusWorld         // sampling radius around trunk (e.g. 80..200)
// );