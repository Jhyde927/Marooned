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
#include "utilities.h"


std::vector<TreeInstance> swampTrees;
std::vector<TreeInstance> trees;
std::vector<BushInstance> bushes;
std::vector<const TreeInstance*> sortedTrees;

BoundingBox GetTreeAABB(const TreeInstance& t)
{
    float r = t.colliderRadius;
    float h = t.colliderHeight;

    Vector3 min {
        t.position.x - r,
        t.position.y,
        t.position.z - r
    };

    Vector3 max {
        t.position.x + r,
        t.position.y + h,
        t.position.z + r
    };

    return { min, max };
}



void sortTrees(Camera& camera){
    // Sort farthest to nearest (back-to-front)
    std::sort(sortedTrees.begin(), sortedTrees.end(), [&](const TreeInstance* a, const TreeInstance* b) {
        float da = Vector3Distance(camera.position, a->position);
        float db = Vector3Distance(camera.position, b->position);
        return da > db; // draw farther trees first
    });
}

TreeType RandomTreeType()
{
    //TODO: Get random palm tree, get random swamp tree. 
    int count = static_cast<int>(TreeType::COUNT);
    int r = GetRandomValue(0, count - 1); // raylib RNG
    return static_cast<TreeType>(r);
}



void generateVegetation(){

    float treeSpacing = (levelIndex == 23) ? 150 : 150; // higher spacing for swamp
    float minTreeSpacing = (levelIndex == 23) ? 50 : 50;

    float treeHeightThreshold = terrainScale.y * 0.8f;
    float bushHeightThreshold = terrainScale.y * 0.9f;
    heightmapPixels = (unsigned char*)heightmap.data; //for iterating heightmap data for tree placement
    // Generate the trees
    trees = GenerateTrees(heightmap, heightmapPixels, terrainScale, treeSpacing, minTreeSpacing, treeHeightThreshold);
    bushes = GenerateBushes(heightmap, heightmapPixels, terrainScale, treeSpacing, bushHeightThreshold);

    auto treesOnLand = FilterInstancesOnLand<TreeInstance>(
        trees,
        [](const TreeInstance& t) { return t.position; },
        heightmap, heightmapPixels, terrainScale,
        waterHeightY, 80.0f, 120.0f
    );

    auto bushesOnLand = FilterInstancesOnLand<BushInstance>(
        bushes,
        [](const BushInstance& b) { return b.position; }, // or b.pos, whatever you named it
        heightmap, heightmapPixels, terrainScale,
        waterHeightY, 40.0f, 60.0f
    );

    trees = treesOnLand;
    bushes = bushesOnLand;


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
                    TreeType randomType = RandomTreeType();
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

static inline float SampleHeightBilinear_U8(
    const unsigned char* px, int w, int h,
    float u, float v) // u,v in [0..1]
{
    u = Clamp01(u);
    v = Clamp01(v);

    float x = u * (w - 1);
    float y = v * (h - 1);

    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = (x0 + 1 < w) ? x0 + 1 : x0;
    int y1 = (y0 + 1 < h) ? y0 + 1 : y0;

    float tx = x - x0;
    float ty = y - y0;

    auto H = [&](int ix, int iy) -> float {
        return (float)px[iy * w + ix] / 255.0f;
    };

    float h00 = H(x0, y0);
    float h10 = H(x1, y0);
    float h01 = H(x0, y1);
    float h11 = H(x1, y1);

    float hx0 = h00 + (h10 - h00) * tx;
    float hx1 = h01 + (h11 - h01) * tx;
    return hx0 + (hx1 - hx0) * ty;
}

static inline void WorldXZ_ToHeightUV(
    float worldX, float worldZ,
    const Vector3& terrainScale, // terrain spans [-sx/2..+sx/2], [-sz/2..+sz/2]
    float& outU, float& outV)
{
    outU = (worldX + terrainScale.x * 0.5f) / terrainScale.x;
    outV = (worldZ + terrainScale.z * 0.5f) / terrainScale.z;
}

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
)
{
    std::vector<T> out;
    out.reserve(input.size());

    const int w = heightmap.width;
    const int h = heightmap.height;

    const float du = (terrainScale.x > 0.0f) ? (trunkRadiusWorld / terrainScale.x) : 0.0f;
    const float dv = (terrainScale.z > 0.0f) ? (trunkRadiusWorld / terrainScale.z) : 0.0f;

    const float landMinY = seaLevelWorldY + shoreMarginWorldY;

    for (const auto& inst : input)
    {
        const Vector3 p = getPos(inst);

        float u, v;
        WorldXZ_ToHeightUV(p.x, p.z, terrainScale, u, v);
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) continue;

        float hC01 = SampleHeightBilinear_U8(pixels, w, h, u, v);
        float hE01 = SampleHeightBilinear_U8(pixels, w, h, u + du, v);
        float hW01 = SampleHeightBilinear_U8(pixels, w, h, u - du, v);
        float hN01 = SampleHeightBilinear_U8(pixels, w, h, u, v - dv);
        float hS01 = SampleHeightBilinear_U8(pixels, w, h, u, v + dv);

        float yC = hC01 * terrainScale.y;
        float yE = hE01 * terrainScale.y;
        float yW = hW01 * terrainScale.y;
        float yN = hN01 * terrainScale.y;
        float yS = hS01 * terrainScale.y;

        float yMin = std::min(std::min(std::min(yC, yE), std::min(yW, yN)), yS);
        if (yMin < landMinY) continue;

        out.push_back(inst);
    }

    return out;
}


// std::vector<TreeInstance> FilterTreesOnLand(
//     const std::vector<TreeInstance>& input,
//     const Image& heightmap,
//     const unsigned char* pixels,   // 1 byte per pixel height
//     const Vector3& terrainScale,
//     float seaLevelWorldY,          // your water plane height in world units
//     float shoreMarginWorldY,       // how far above sea level a tree must be (e.g. 40..150)
//     float trunkRadiusWorld         // sampling radius around trunk (e.g. 80..200)
// )
// {
//     std::vector<TreeInstance> out;
//     out.reserve(input.size());

//     const int w = heightmap.width;
//     const int h = heightmap.height;

//     // Convert a world-space radius to UV offsets (approx)
//     const float du = (terrainScale.x > 0.0f) ? (trunkRadiusWorld / terrainScale.x) : 0.0f;
//     const float dv = (terrainScale.z > 0.0f) ? (trunkRadiusWorld / terrainScale.z) : 0.0f;

//     for (const auto& tree : input)
//     {
//         float u, v;
//         WorldXZ_ToHeightUV(tree.position.x, tree.position.z, terrainScale, u, v);

//         // Quick reject if out of bounds
//         if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) continue;

//         // Sample a small footprint: center + 4 cardinal taps
//         float hCenter01 = SampleHeightBilinear_U8(pixels, w, h, u, v);
//         float hE01      = SampleHeightBilinear_U8(pixels, w, h, u + du, v);
//         float hW01      = SampleHeightBilinear_U8(pixels, w, h, u - du, v);
//         float hN01      = SampleHeightBilinear_U8(pixels, w, h, u, v - dv);
//         float hS01      = SampleHeightBilinear_U8(pixels, w, h, u, v + dv);

//         // Convert to world Y
//         float yCenter = hCenter01 * terrainScale.y;
//         float yMin    = std::min(std::min(std::min(yCenter, hE01 * terrainScale.y),
//                                           std::min(hW01 * terrainScale.y, hN01 * terrainScale.y)),
//                                  hS01 * terrainScale.y);

//         // Land test: must be above sea level + margin everywhere under the trunk
//         float landMinY = seaLevelWorldY + shoreMarginWorldY;

//         if (yMin < landMinY)
//             continue;

//         // If you still want your “height threshold * cullFactor” behavior,
//         // apply it using the *actual sampled* height:
//         // if (yCenter <= treeHeightThreshold * tree.cullFactor) continue;

//         out.push_back(tree);
//     }

//     return out;
// }


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
    float cullDist = 50000;
    for (const TreeInstance* tree : sortedTrees) {
        float dist = Vector3Distance(tree->position, camera.position);
        Vector3 pos = tree->position;
        pos.y += tree->yOffset;
        pos.x += tree->xOffset;
        pos.z += tree->zOffset;
        //Model& treeModel = R.GetModel("palmTree");

        // switch (tree->type) {
        //     case TreeType::Big: treeModel = (gCurrentLevelIndex == 23) ? R.GetModel("swampTree") : R.GetModel("palmTree");

        //     case TreeType::Medium: treeModel = (gCurrentLevelIndex == 23) ? R.GetModel("swampTree") : R.GetModel("palm2");

        //     default: treeModel = R.GetModel("palmTree");
        // }

        Model& treeModel = tree->useAltModel ? R.GetModel("palmTree") : R.GetModel("palm2");
        Model& swampTree = R.GetModel("swampTree");
        float scale = tree->useAltModel ? 40.0f : 20.0f;
        if (dist < cullDist){

            if (gCurrentLevelIndex == 23){
                DrawModelEx(swampTree, pos, { 0, 1, 0 }, tree->rotationY, { scale, scale, scale }, WHITE);
            }else{
                DrawModelEx(treeModel, pos, { 0, 1, 0 }, tree->rotationY, { tree->scale, tree->scale, tree->scale }, WHITE);
            }
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