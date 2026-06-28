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
#include "game_settings.h"



std::vector<TreeInstance> trees;
std::vector<BushInstance> bushes;
std::vector<const TreeInstance*> sortedTrees;

float GetRandomTreeScale()
{
    // 0 - 99
    int roll = GetRandomValue(0, 99);

    // Base scale ranges.
    // Tweak these numbers until they look right with your palm model.
    if (roll < 45)
    {
        // Small: 45% chance
        return GetRandomValue(70, 90) / 100.0f;      // 0.70 - 0.90
    }
    else if (roll < 80)
    {
        // Medium: 35% chance
        return GetRandomValue(90, 115) / 100.0f;     // 0.90 - 1.15
    }
    else if (roll < 97)
    {
        // Large: 17% chance
        return GetRandomValue(115, 145) / 100.0f;    // 1.15 - 1.45
    }
    else
    {
        // Extra large: 3% chance
        return GetRandomValue(145, 180) / 100.0f;    // 1.45 - 1.80
    }
}

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
    bool isSwamp = CurrentLevelIs("Swamp");
    float treeSpacing = isSwamp ? 150 : 150; // higher spacing for swamp
    float minTreeSpacing = isSwamp ? 50 : 50;

    float treeHeightThreshold = terrainScale.y * 0.8f;
    //float bushHeightThreshold = terrainScale.y * 0.9f;
    heightmapPixels = (unsigned char*)heightmap.data; //for iterating heightmap data for tree placement
    // Generate the trees
    trees = GenerateTrees(heightmap, heightmapPixels, terrainScale, treeSpacing, minTreeSpacing, treeHeightThreshold);
    //bushes = GenerateBushes(heightmap, heightmapPixels, terrainScale, treeSpacing, bushHeightThreshold);
    //we now do flat quad bushes. 
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


    // for (const auto& tree : trees) { 
    //     sortedTrees.push_back(&tree); //trees no longer need to be sorted, alpha cutoff shader solves the issue. 
    // }

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

std::vector<TreeInstance> GenerateTrees(
    Image& heightmap,
    unsigned char* pixels,
    Vector3 terrainScale,
    float treeSpacing,
    float minTreeSpacing,
    float treeHeightThreshold
)
{
    std::vector<TreeInstance> trees;

    if (!pixels || heightmap.width <= 0 || heightmap.height <= 0)
    {
        return trees;
    }

    int step = (int)treeSpacing;
    if (step < 1) step = 1;

    const float minTreeSpacingSq = minTreeSpacing * minTreeSpacing;

    const float startClearRadius = 700.0f;
    const float startClearRadiusSq = startClearRadius * startClearRadius;

    const float entranceClearRadius = treeSpacing * 2.0f;
    const float entranceClearRadiusSq = entranceClearRadius * entranceClearRadius;

    for (int z = 0; z < heightmap.height; z += step)
    {
        for (int x = 0; x < heightmap.width; x += step)
        {
            int i = z * heightmap.width + x;

            float height = ((float)pixels[i] / 255.0f) * terrainScale.y;

            if (height <= treeHeightThreshold)
                continue;

            Vector3 pos = {
                (float)x / (float)heightmap.width  * terrainScale.x - terrainScale.x * 0.5f,
                height - 5.0f, // sink the tree into the ground a little
                (float)z / (float)heightmap.height * terrainScale.z - terrainScale.z * 0.5f
            };

            bool tooClose = false;

            // Don't spawn trees too close to the player/start position.
            if (Vector3DistanceSqr(pos, startPosition) < startClearRadiusSq)
            {
                tooClose = true;
            }

            // Don't spawn trees on top of dungeon entrances.
            if (!tooClose)
            {
                for (const DungeonEntrance& d : dungeonEntrances)
                {
                    if (Vector3DistanceSqr(pos, d.position) < entranceClearRadiusSq)
                    {
                        tooClose = true;
                        break;
                    }
                }
            }

            // Don't spawn too close to other accepted trees.
            if (!tooClose)
            {
                for (const TreeInstance& other : trees)
                {
                    if (Vector3DistanceSqr(pos, other.position) < minTreeSpacingSq)
                    {
                        tooClose = true;
                        break;
                    }
                }
            }

            if (tooClose)
                continue;

            TreeInstance tree;
            tree.position = pos;

            // NOTE:
            // If your tree drawing expects degrees, keep this as degrees.
            // If it expects radians, change this to RandomFloat(0, PI * 2).
            tree.rotationY = (float)GetRandomValue(0, 359);

            tree.scale = 20.0f + ((float)GetRandomValue(0, 1000) / 100.0f); // 20.0 - 30.0
            tree.yOffset = ((float)GetRandomValue(-600, 200)) / 100.0f;     // -6.0 to 2.0
            tree.xOffset = (float)GetRandomValue(-(int)treeSpacing, (int)treeSpacing);
            tree.zOffset = (float)GetRandomValue(-(int)treeSpacing, (int)treeSpacing);
            tree.useAltModel = GetRandomValue(0, 1);
            tree.randomScale = GetRandomTreeScale();
            tree.cullFactor = 1.15f;

            // You had this but were not using it.
            // TreeType randomType = RandomTreeType();

            trees.push_back(tree);
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

std::vector<BushInstance> GenerateBushes(
    Image& heightmap,
    unsigned char* pixels,
    Vector3 terrainScale,
    float bushSpacing,
    float heightThreshold
)
{
    std::vector<BushInstance> bushes;

    if (!pixels || heightmap.width <= 0 || heightmap.height <= 0)
    {
        return bushes;
    }

    int step = (int)bushSpacing;
    if (step < 1) step = 1;

    const float startClearRadius = 700.0f;
    const float startClearRadiusSq = startClearRadius * startClearRadius;

    const int jitterRange = (int)(bushSpacing * 2.0f);

    Model bushModel = R.GetModel("bush");

    for (int z = 0; z < heightmap.height; z += step)
    {
        for (int x = 0; x < heightmap.width; x += step)
        {
            int i = z * heightmap.width + x;

            float height = ((float)pixels[i] / 255.0f) * terrainScale.y;

            if (height <= heightThreshold)
                continue;

            Vector3 pos = {
                (float)x / (float)heightmap.width  * terrainScale.x - terrainScale.x * 0.5f,
                height - 5.0f,
                (float)z / (float)heightmap.height * terrainScale.z - terrainScale.z * 0.5f
            };

            if (Vector3DistanceSqr(pos, startPosition) < startClearRadiusSq)
                continue;

            // Keep your random 75% spawn chance.
            if (GetRandomValue(1, 4) <= 1)
                continue;

            BushInstance bush;
            bush.position = pos;
            bush.scale = 100.0f + ((float)GetRandomValue(0, 1000) / 100.0f);
            bush.model = bushModel;

            bush.yOffset = ((float)GetRandomValue(-200, 200)) / 100.0f; // -2.0 to 2.0

            bush.xOffset = (float)GetRandomValue(-jitterRange, jitterRange);
            bush.zOffset = (float)GetRandomValue(-jitterRange, jitterRange);

            // Aggressively cull bushes. As high as it can be before no bushes.
            bush.cullFactor = 1.09f;

            bushes.push_back(bush);
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
