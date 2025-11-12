// vegetation.cpp

#include "vegetation.h"
#include "raylib.h"
#include <vector>
#include <cmath>
#include "raymath.h"
#include "rlgl.h"
#include "resourceManager.h"
#include "world.h"
#include "algorithm"
#include "shadows.h"

std::vector<TreeInstance> trees;
std::vector<BushInstance> bushes;
std::vector<const TreeInstance*> sortedTrees;




void sortTrees(Camera& camera){
    // Sort farthest to nearest (back-to-front)
    std::sort(sortedTrees.begin(), sortedTrees.end(), [&](const TreeInstance* a, const TreeInstance* b) {
        float da = Vector3Distance(camera.position, a->position);
        float db = Vector3Distance(camera.position, b->position);
        return da > db; // draw farther trees first
    });
}



void generateVegetation(){

    float treeSpacing = 150.0f;
    float minTreeSpacing = 50.0f;
    float treeHeightThreshold = terrainScale.y * 0.8f;
    float bushHeightThreshold = terrainScale.y * 0.9f;
    heightmapPixels = (unsigned char*)heightmap.data; //for iterating heightmap data for tree placement
    // Generate the trees
    trees = GenerateTrees(heightmap, heightmapPixels, terrainScale, treeSpacing, minTreeSpacing, treeHeightThreshold);



    //Filter trees based on final height cutoff
    trees = FilterTreesAboveHeightThreshold(trees, heightmap, heightmapPixels, terrainScale, treeHeightThreshold);

    bushes = GenerateBushes(heightmap, heightmapPixels, terrainScale, treeSpacing, bushHeightThreshold);
    bushes = FilterBushsAboveHeightThreshold(bushes, heightmap, heightmapPixels, terrainScale, bushHeightThreshold);


    for (const auto& tree : trees) { 
        sortedTrees.push_back(&tree); //trees no longer need to be sorted, alpha cutoff shader solves the issue. 
    }

    // Define world XZ bounds from your terrainScale (centered at origin)
    Rectangle worldXZ = {
        -terrainScale.x * 0.5f,   // minX
        -terrainScale.z * 0.5f,   // minZ
        terrainScale.x,          // sizeX
        terrainScale.z           // sizeZ
    };

    // Create/update a global or stored mask
    //static TreeShadowMask gTreeShadowMask;
    InitOrResizeTreeShadowMask(gTreeShadowMask, /*tex size*/ 4096, 4096, worldXZ);

    BuildTreeShadowMask_Tex(gTreeShadowMask, trees, R.GetTexture("treeShadow"));

    
}

std::vector<TreeInstance> GenerateTrees(Image& heightmap, unsigned char* pixels, Vector3 terrainScale,
                                        float treeSpacing, float minTreeSpacing, float treeHeightThreshold) {
    std::vector<TreeInstance> trees;

    float startClearRadiusSq = 700.0f;
    for (int z = 0; z < heightmap.height; z += (int)treeSpacing) {
        for (int x = 0; x < heightmap.width; x += (int)treeSpacing) {
            int i = z * heightmap.width + x;
            float height = ((float)pixels[i] / 255.0f) * terrainScale.y;


            if (height > treeHeightThreshold) {
                Vector3 pos = {
                    (float)x / heightmap.width * terrainScale.x - terrainScale.x / 2,
                    height-5, //sink the tree into the ground a little. 
                    (float)z / heightmap.height * terrainScale.z - terrainScale.z / 2
                };
                

                // Check distance from other trees
                bool tooClose = false;
                for (const auto& other : trees) {
                    if (Vector3Distance(pos, other.position) < minTreeSpacing) {
                        tooClose = true;
                        break;
                    }

                    if (Vector3Distance(pos, startPosition) < startClearRadiusSq) tooClose = true;

                    for (DungeonEntrance& d : dungeonEntrances){//dont spawn trees ontop of entrances
                        
                        if (Vector3Distance(pos, d.position) < treeSpacing * 2 ){
                            
                            tooClose = true;
                            break;
                        }
                    }
                }

                if (!tooClose) { //not too close, spawn a tree. 
                    TreeInstance tree;
                    tree.position = pos;
                    tree.rotationY = (float)GetRandomValue(0, 359);
                    tree.scale = 20.0f + ((float)GetRandomValue(0, 1000) / 100.0f); // 20.0 - 30.0
                    tree.yOffset = ((float)GetRandomValue(-600, 200)) / 100.0f;     // -2.0 to 2.0
                    tree.xOffset = ((float)GetRandomValue(-treeSpacing, treeSpacing));
                    tree.zOffset = ((float)GetRandomValue(-treeSpacing, treeSpacing));
                    tree.useAltModel = GetRandomValue(0, 1);
                    tree.cullFactor = 1.15f; //5 percent higher than tree threshold. 
                    //if (GetRandomValue(1, 4) > 1){
                    trees.push_back(tree);
                    //}
                    

                    
                }
            }
        }
    }

    return trees;
}

std::vector<TreeInstance> FilterTreesAboveHeightThreshold(const std::vector<TreeInstance>& inputTrees, Image& heightmap,
                                                          unsigned char* pixels, Vector3 terrainScale,
                                                          float treeHeightThreshold) {
    std::vector<TreeInstance> filtered;

    for (const auto& tree : inputTrees) {
        float xPercent = (tree.position.x + terrainScale.x / 2) / terrainScale.x;
        float zPercent = (tree.position.z + terrainScale.z / 2) / terrainScale.z;

        int xPixel = (int)(xPercent * heightmap.width);
        int zPixel = (int)(zPercent * heightmap.height);

        if (xPixel < 0 || xPixel >= heightmap.width || zPixel < 0 || zPixel >= heightmap.height) continue;

        int i = zPixel * heightmap.width + xPixel;
        float height = ((float)pixels[i] / 255.0f) * terrainScale.y;

        if (height > treeHeightThreshold * tree.cullFactor) {
            filtered.push_back(tree);
        }
    }

    return filtered;
}

std::vector<BushInstance> GenerateBushes(Image& heightmap, unsigned char* pixels, Vector3 terrainScale,
                                         float bushSpacing, float heightThreshold) {
    std::vector<BushInstance> bushes;
    float startClearRadiusSq = 700.0f;  
    for (int z = 0; z < heightmap.height; z += (int)bushSpacing) {
        for (int x = 0; x < heightmap.width; x += (int)bushSpacing) {
            int i = z * heightmap.width + x;
            float height = ((float)pixels[i] / 255.0f) * terrainScale.y;

            if (height > heightThreshold) {
                Vector3 pos = {
                    (float)x / heightmap.width * terrainScale.x - terrainScale.x / 2,
                    height-5,
                    (float)z / heightmap.height * terrainScale.z - terrainScale.z / 2
                };

                if (Vector3Distance(pos, startPosition) < startClearRadiusSq) continue;
            
                BushInstance bush;
                bush.position = pos;
                bush.scale = 100.0f + ((float)GetRandomValue(0, 1000) / 100.0f);
                bush.model = R.GetModel("bush");
                bush.yOffset = ((float)GetRandomValue(-200, 200)) / 100.0f;     // -2.0 to 2.0
                bush.xOffset = ((float)GetRandomValue(-bushSpacing*2, bushSpacing*2));
                bush.zOffset = ((float)GetRandomValue(-bushSpacing*2, bushSpacing*2)); //space them out wider, then cull more aggresively. 
                bush.cullFactor = 1.09f; //agressively cull bushes. as high as it can be before no bushes

                if (GetRandomValue(1,4) > 1){
                    bushes.push_back(bush);

                }


 
            }
        }
    }

    return bushes;
}

std::vector<BushInstance> FilterBushsAboveHeightThreshold(const std::vector<BushInstance>& inputBushes, Image& heightmap,
                                                          unsigned char* pixels, Vector3 terrainScale,
                                                          float treeHeightThreshold) {
    std::vector<BushInstance> filtered;

    for (const auto& bush : inputBushes) {
        float xPercent = (bush.position.x + terrainScale.x / 2) / terrainScale.x;
        float zPercent = (bush.position.z + terrainScale.z / 2) / terrainScale.z;

        int xPixel = (int)(xPercent * heightmap.width);
        int zPixel = (int)(zPercent * heightmap.height);

        if (xPixel < 0 || xPixel >= heightmap.width || zPixel < 0 || zPixel >= heightmap.height) continue;

        int i = zPixel * heightmap.width + xPixel;
        float height = ((float)pixels[i] / 255.0f) * terrainScale.y;

        if (height > treeHeightThreshold * bush.cullFactor) {
            filtered.push_back(bush);
        }
    }

    return filtered;
}

void RemoveAllVegetation() {
    trees.clear();
    bushes.clear();
    sortedTrees.clear();
}



void DrawTrees(const std::vector<TreeInstance>& trees, Camera& camera){
    float cullDist = 15000;
    for (const TreeInstance* tree : sortedTrees) {
        float dist = Vector3Distance(tree->position, camera.position);

        Vector3 pos = tree->position;
        pos.y += tree->yOffset;
        pos.x += tree->xOffset;
        pos.z += tree->zOffset;

        Model& treeModel = tree->useAltModel ? R.GetModel("palmTree") : R.GetModel("palm2");
        if (dist < cullDist){
            DrawModelEx(treeModel, pos, { 0, 1, 0 }, tree->rotationY,
                        { tree->scale, tree->scale, tree->scale }, WHITE);

        }

    }


}

void DrawBushes(const std::vector<BushInstance>& bushes) {
    float cullDistance = 15000.0f;
    for (const auto& bush : bushes) {
        float distanceTo = Vector3Distance(player.position, bush.position);

        Vector3 pos = bush.position;
        pos.x += bush.xOffset;
        pos.y += bush.yOffset-10;
        pos.z += bush.zOffset;
        Color customGreen = { 0, 160, 0, 255 };
        if (distanceTo < cullDistance){
            DrawModel(bush.model, pos, bush.scale, WHITE);

        }


    }
}