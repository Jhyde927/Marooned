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
        int loc_waterLevel    = -1;
        int loc_waterColor    = -1;

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

    struct TreeShader
    {
        Shader* shader = nullptr;

        // Uniform locations (cached)
        int loc_skyTop    = -1;
        int loc_skyHorz   = -1;
        int loc_fogStart  = -1;
        int loc_fogEnd    = -1;
        int loc_seaLevel  = -1;
        int loc_falloff   = -1;
        int loc_alphaCut  = -1;
    

        // Params you want to store
        Vector3 skyTop  = {0.55f, 0.75f, 1.00f};
        Vector3 skyHorz = {0.60f, 0.80f, 0.95f};
        float fogStart  = 0.0f;
        float fogEnd    = 18000.0f;
        float seaLevel  = 400.0f;
        float falloff   = 0.002f;

        float alphaCutoff = 0.30f;
    };

    struct SkyShader
    {
        Shader* shader = nullptr;

        // Cached locations
        int loc_time      = -1;
        int loc_isSwamp   = -1;
        int loc_isDungeon = -1;

        // Stored params
        int   isSwamp = 0;   // 0/1
        int   isDungeon = 0;

        float timeSec   = 0.0f;
    };




    extern PortalShader gPortal;
    extern WaterShader  gWater;
    extern LavaShader gLava;
    extern BloomShader gBloom;
    extern TreeShader gTree;
    extern SkyShader gSky;

    //sky shader
    void InitSkyShader(Shader& shader, SkyShader& out, Model& skyModel, bool isDungeon);
    void UpdateSkyShaderPerFrame(SkyShader& ss, float timeSeconds);


    //treeShader
    void InitTreeShader(Shader& shader, TreeShader& out, std::initializer_list<Model*> modelsToBind);
    void ApplyTreeFogParams(TreeShader& ts);
    void SetTreeAlphaCutoff(TreeShader& ts, float cutoff);

    //Bloom
    void InitBloomShader(Shader& shader, BloomShader& out);
    void ApplyBloomParams(BloomShader& bs);
    void SetBloomResolution(BloomShader& bs, int screenW, int screenH);
    void SetBloomTonemap(BloomShader& bs, bool isDungeon, float islandExposure, float dungeonExposure);
    void SetBloomStrength(BloomShader& bs, float strength);

    //Portal Shader
    void InitPortalShader(Shader& shader, PortalShader& out);
    void ApplyPortalDefaults(PortalShader& ps);

    //Lava shader
    void InitLavaShader(Shader& shader, LavaShader& out, Model& lavaTileModel);
    void UpdateLavaShaderPerFrame(LavaShader& ls, float t, bool isLoadingLevel);
    void UpdatePortalShader(PortalShader& ps, float t);
    void UpdateTreeShader(TreeShader& ts, Camera& camera);
    //WaterShader
    void InitWaterShader(Shader& shader, WaterShader& out, Vector3 terrainScale);
    void UpdateWaterShaderPerFrame(WaterShader& ws, float elapsedTime, const Camera& camera);
}
