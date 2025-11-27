// vegetation.h

#pragma once

#include "raylib.h"
#include <vector>




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