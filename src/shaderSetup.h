// shaderSetup.h
#pragma once

#include "raylib.h"
#include "resourceManager.h"



namespace ShaderSetup
{

    struct PortalShader
    {
        // Non-owning pointer to the shader stored inside ResourceManager.
        // (Important: don't store a copy of Shader if you want live updates/hot reload later.)
        Shader* shader = nullptr;

        // Cached locations
        int loc_speed         = -1;
        int loc_swirlStrength = -1;
        int loc_swirlScale    = -1;
        int loc_colorA        = -1;
        int loc_colorB        = -1;
        int loc_edgeFeather   = -1;
        int loc_rings         = -1;
        int loc_glowBoost     = -1;

        // Optional: keep defaults here so you can re-apply them easily (hot reload, reset, etc.)
        float speed         = 1.4f;
        float swirlStrength = 1.2f;
        float swirlScale    = 12.0f;
        float edgeFeather   = 0.08f;
        float rings         = 0.7f;
        float glowBoost     = 0.8f;

        Vector3 colorA      = { 0.0f, 0.25f, 1.0f };
        Vector3 colorB      = { 0.5f, 0.2f,  1.0f };
    };



    // -------------------------
    // Water shader
    // -------------------------
    struct WaterShader
    {
        Shader* shader = nullptr;

        // Cached locations
        int loc_WaterCenterXZ = -1;
        int loc_PatchHalfSize = -1;
        int loc_FadeStart     = -1;
        int loc_FadeEnd       = -1;
        int loc_cameraPos     = -1;

        // Constants / tunables
        float patchHalf  = 8000.0f;   // u_PatchHalfSize
        float fadeStart  = 10000.0f;  // u_FadeStart
        float fadeEnd    = 16000.0f;  // u_FadeEnd
        float feather    = 600.0f;    // used for clamping center

        // World bounds inputs (store these so Update() can clamp)
        Vector2 worldMinXZ  = { 0, 0 };   // (minX, minZ)
        Vector2 worldSizeXZ = { 0, 0 };   // (sizeX, sizeZ)
    };

    // 

    struct LavaShader
    {
        Shader* shader = nullptr;

        // Cached locations
        int locTime   = -1;
        int locDir    = -1;
        int locSpeed  = -1;
        int locOff    = -1;
        int locScale  = -1;
        int locFreq   = -1;
        int locAmp    = -1;
        int locEmis   = -1;
        int locGain   = -1;

        // Tunables / defaults
        Vector2 scrollDir     = { 0.07f, 0.0f };
        float   speed         = 0.5f;
        float   distortFreq   = 6.0f;
        float   distortAmp    = 0.02f;
        Vector3 emissive      = { 3.0f, 1.0f, 0.08f };
        float   emissiveGain  = 0.1f;

        // World-to-UV mapping params
        float   tileSize          = 1.0f;      // you pass this in at init
        float   uvsPerWorldUnit   = 0.5f;      // your "0.5 / tileSize" factor base
        Vector2 worldOffset       = { 0.0f, 0.0f };
    };

    struct BloomShader
    {
        Shader* shader = nullptr;

        // Cached locations
        int loc_bloomStrength   = -1;
        int loc_exposure        = -1;
        int loc_toneMapOperator = -1;
        int loc_resolution      = -1;

        // Stored params (so you can re-apply easily)
        float   bloomStrength = 0.0f;
        float   exposure      = 1.0f;
        int     toneOp        = 0; // 0 = island, 1 = dungeon (based on your code)
        Vector2 resolution    = { 0, 0 };
    };

    // Cache locations + apply initial values
    void InitBloomShader(Shader& shader, BloomShader& out);
    // Apply values (call when bloomStrength/exposure/toneOp/resolution changes)
    void ApplyBloomParams(BloomShader& bs);
    // Convenience: set resolution + apply (call on window resize)
    void SetBloomResolution(BloomShader& bs, int screenW, int screenH);
    // Convenience: set tonemap based on your game state + apply
    void SetBloomTonemap(BloomShader& bs, bool isDungeon, float islandExposure, float dungeonExposure);
    // Convenience: set bloom strength + apply
    void SetBloomStrength(BloomShader& bs, float strength);

    extern PortalShader gPortal;
    extern WaterShader  gWater;
    extern LavaShader gLava;
    extern BloomShader gBloom;

    void InitPortalShader(Shader& shader, PortalShader& out);

    void ApplyPortalDefaults(PortalShader& ps);

    void InitLavaShader(Shader& shader, LavaShader& out, Model& lavaTileModel);

    // Update per-frame: only sets uTime (and anything else you later decide is dynamic).
    void UpdateLavaShaderPerFrame(LavaShader& ls, float t, bool isLoadingLevel);

    // Call once after water shader is loaded. Pass terrainScale so we can compute world bounds.
    void InitWaterShader(Shader& shader, WaterShader& out, Vector3 terrainScale);

    // Call every frame (or whenever camera moves) to update dynamic uniforms only.
    void UpdateWaterShaderPerFrame(WaterShader& ws, const Camera& camera);
}
