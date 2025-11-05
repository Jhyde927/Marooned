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

extern BakedLightmap gDynamic;  //is it really a bakedLighmap if it's dynamic? it's a hybrid


float SmoothFalloff(float d, float radius);

void InitDynamicLightmap(int res);
void BuildStaticLightmapOnce(const std::vector<LightSource>& dungeonLights);
void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights);

void LogDynamicLightmapNonBlack(const char* tag);
