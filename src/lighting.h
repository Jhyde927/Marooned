#pragma once

#include "raylib.h"
#include "vector"
#include "dungeonGeneration.h"

struct XZBounds { float minX, maxX, minZ, maxZ; };

struct BakedLightmap {
    Texture2D tex = {};
    int w = 0, h = 0;
    // World-space mapping (XZ -> [0..1])
    float minX = 0, minZ = 0, sizeX = 1, sizeZ = 1; // size = max - min
    std::vector<Color> pixels; // CPU buffer (RGB in 0..255)
};

struct RectI
{
    int x0, y0, x1, y1; // inclusive bounds: x in [x0..x1], y in [y0..y1]
};


struct LightingConfig
{
    float ambient;
    float dynStrength;

    float staticRadius;
    float staticIntensity;
    Vector3 staticColor;

    float dynamicRange;
    float dynamicIntensity;
    Vector3 dynamicFireColor;
    Vector3 dynamicIceColor;

    Vector3 playerColor;
    float   playerRadius;
    float   playerIntensity;

    int   losNumRays;
    float losSpreadFrac;
    float losOriginY;
    float losTargetY;
    float losEpsilonFrac;

    float dungeonExposure;
    float islandExposure;
};



// Global instance (defined in lightingConfig.cpp)
extern LightingConfig lightConfig;
extern BakedLightmap gDynamic; 
extern std::vector<int> StaticLightIndices;

LightSource MakeStaticTorch(Vector3 pos); 

float SmoothFalloff(float d, float radius);
void GatherFrameLights();
void InitDynamicLightmap(int res);
void BuildStaticLightmapOnce(const std::vector<LightSource>& dungeonLights);
void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights);
std::vector<int> GetStaticLightIndices(const Vector3& doorPos);

// Call this right after a door toggles open/closed.
// Rebuilds static base lighting only in a local region around the door,
// using only the affected static lights (by index).
void OnDoorToggled_RebakeStaticLights(const Vector3& doorWorldPos, const std::vector<int>& affectedStaticLightIndices);

void SubtileVis2x2(float vis[2][2],
                          const Vector3& lightPos,
                          float cx, float cz, float tileSize, float floorY);

                          
