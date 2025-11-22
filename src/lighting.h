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



// One place for all lighting tweakables.
// Keep it POD/simple so you can hot-tweak easily.
struct LightingConfig
{
    // ----- Forward lighting -----
    float ambient            = 0.2f;  // uAmbient
    float maxLightingClamp   = 2.0f;   // clamp upper bound in shader
    float falloffStrength    = 1.0f;   // optional multiplier if you add it later

    // ----- Static torch lights -----
    float staticRadius       = 1600.0f;  // world units
    float staticIntensity    = 0.6f;    // scalar multiplier
    Vector3 staticColor      = {1.0f, 0.95f, 1.0f}; //slight purple tint

    // ----- Dynamic (frame) lights -----
    float dynamicRange   = 400.0f;   
    float dynamicIntensity= 0.5f;
    Vector3 dynamicFireColor      = {1.0f, 0.3f, 0.0f}; // red glow
    Vector3 dynamicIceColor       = {0.0f, 0.7f, 1.0f}; //ice glow blue

    // ----- Occlusion / visibility mask -----
    int   losNumRays         = 13;
    float losSpreadFrac      = 0.25f;  // spreadMeters = losSpreadFrac * tileSize
    float losOriginY         = 20.0f;  // originYOffset
    float losTargetY         = 20.0f;  // targetYOffset
    float losEpsilonFrac     = 0.005f;

    // // ----- Occlusion texture resolution -----
    // int subtilesPerTile      = 2; // 2 = 2x2, 4 = 4x4 if you want smoother later

    // // ----- Filtering choices -----
    // bool occlusionBilinear   = false; // point by default for crisp 2x2
};

// Global instance (defined in lightingConfig.cpp)
extern LightingConfig lightConfig;
extern BakedLightmap gDynamic; 
extern Texture2D gDungeonOcclusionTex;
float SmoothFalloff(float d, float radius);

void InitDynamicLightmap(int res);
void BuildStaticLightmapOnce(const std::vector<LightSource>& dungeonLights);
void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights);

void LogDynamicLightmapNonBlack(const char* tag);

void BuildStaticOcclusionTexture();
void InitDungeonLightingBounds();