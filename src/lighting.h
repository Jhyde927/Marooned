#pragma once

#include "raylib.h"
#include "vector"
#include "dungeonGeneration.h"

struct XZBounds { float minX, maxX, minZ, maxZ; };

struct BakedLightmap {
    Texture2D tex = {0};
    int w = 0, h = 0;
    // World-space mapping (XZ -> [0..1])
    float minX = 0, minZ = 0, sizeX = 1, sizeZ = 1; // size = max - min
    std::vector<Color> pixels; // CPU buffer (RGB in 0..255)
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
};



// Global instance (defined in lightingConfig.cpp)
extern LightingConfig lightConfig;
extern BakedLightmap gDynamic; 


LightSource MakeStaticTorch(Vector3 pos); 

float SmoothFalloff(float d, float radius);
void GatherFrameLights();
void InitDynamicLightmap(int res);
void BuildStaticLightmapOnce(const std::vector<LightSource>& dungeonLights);
void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights);