// shaderSetup.h
#pragma once

#include "raylib.h"
#include "resourceManager.h"
#include "game_settings.h"



namespace ShaderSetup
{

    enum class SkyCyclePhase
    {
        DayHold,
        ToNight,
        NightHold,
        ToDay
    };

    struct ShadowShader 
    {
        Shader* shader = nullptr;

        int loc_strength = -1;

        float shadowStrength = 0.0f;
    };

    struct AlphaCutoutShader
    {

        Shader* shader = nullptr;

        int loc_alphaCutoff = -1;

        float alphaCutoff = 0.0f;;
    };

    struct CeilingShader 
    {
        Shader* shader = nullptr;

        int loc_grid     = -1;
        int loc_dynStr   = -1;
        int loc_amb      = -1;
        int loc_gridSize = -1;
        int loc_tiling   = -1;

        float grid[4] = {0, 0, 0, 0};
        float dynStrength  = 0.0f;
        float ambientBoost = 0.0f;

        Vector2 gridSize = { 0.0f, 0.0f };
        Vector2 tiling   = { 1.0f, 1.0f };

    };

    struct GhostShader 
    {
        Shader* shader = nullptr;

        int loc_viewPos = -1;
        int loc_ghostColor = -1;
        int loc_baseAlpha = -1;
        int loc_rimPower = -1;
        int loc_rimStrength = -1;
        int loc_camPos = -1;
        int loc_darkness = -1;
            

        Vector3 ghostTint = {0, 0, 0};// = {0.4f, 0.8f, 1.0f}; // spectral blue
        Vector3 camPos = {0, 0, 0};
        float alpha = 0.0f;
        float rimPow = 0.0f;;
        float rimStr = 0.0f;;

        float nightDarkness = 0.0f;;


            // 0.35f;
            // 2.0f;
            // 1.2f;
        // Vector3 ghostTint = {0.4f, 0.8f, 1.0f}; // spectral blue

    };

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

        int portalOpenLoc     = -1;

        // Optional: keep defaults here so you can re-apply them easily (hot reload, reset, etc.)
        float speed         = 1.4f;
        float swirlStrength = 1.2f;
        float swirlScale    = 12.0f;
        float edgeFeather   = 0.08f;
        float rings         = 0.7f;
        float glowBoost     = 0.8f;

  

        // Vector3 colorA = { 0.5f, 0.5f, 0.5f }; //gray
        // Vector3 colorB = { 0.1f, 0.1f, 0.1f };

        Vector3 colorA      = { 0.0f, 0.25f, 1.0f }; //purplish pink
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
        int loc_isSwamp       = -1;

        int loc_skyColorTop = -1;
        int loc_skyColorHorz = -1;
        int loc_skyReflectStrength = -1;
        int loc_waterNightDark = -1;

        // Constants / tunables
        float patchHalf  = 8000.0f;   // u_PatchHalfSize
        float fadeStart  = 8500.0f;  // u_FadeStart
        float fadeEnd    = 20000.0f;  // u_FadeEnd
        float feather    = 600.0f;    // used for clamping center
        int   isSwamp    = 0;
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
        int loc_letterboxAmount = -1;
        int loc_letterboxTarget = -1;
        int loc_letterboxSoftness = -1;
        int loc_vignetteMode = -1;
        int loc_baseVignetteStrength = -1;
        int loc_vignetteIntensity = -1;
        int loc_fadeToBlack = -1;
        int loc_dungeonDarkness = -1;
        int loc_dungeonContrast = -1;
        int loc_isDungeon = -1;

        // Stored params (so you can re-apply easily)]
        float   bloomStrength = 0.0f;
        float   exposure      = 1.0f;
        int     toneOp        = 0; // 0 = island, 1 = dungeon (based on your code)
        Vector2 resolution    = { 0, 0 };

        int vignetteMode;
        float vignetteStrengthValue;
        float vignetteIntensity;
        float vignetteFade;
        float fadeToBlack;
        float dungeonDarkness;
        float dungeonContrast;
        int   isDungeon;

        float letterboxAmount;   // 0.0 = none, 0.12 = 12% screen height per bar
        float letterboxTarget;
        float letterboxSoftness; // 0.0 = hard edge, 0.003-0.01 = soft edge
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
        int loc_useFog    = -1;
        int loc_cam       = -1;
        int loc_darkenss  = -1;
    

        // Params you want to store
        Vector3 skyTop  = {0.55f, 0.75f, 1.00f};
        Vector3 skyHorz = {0.60f, 0.80f, 0.95f};
        float fogStart  = 0.0f;
        float fogEnd    = GameSettings::treeFogEnd;
        float seaLevel  = 400.0f;
        float falloff   = 0.002f;
        float alphaCutoff = 0.30f;
        int useFog = 1;
        float nightDarkness = 0.0f;

    };

    struct SkyShader
    {
        Shader* shader = nullptr;

        // Cached locations
        int loc_time      = -1;
        int loc_isSwamp   = -1;
        int loc_isDungeon = -1;
        int skyTransitionLoc = -1;

        // Stored params
        int   isSwamp = 0;   // 0/1
        int   isDungeon = 0;

        //Transition
        float skyTransition = 0.0f; // 0 = day, 1 = night
        bool  gSkyTransitionActive = false;
        float gSkyTransitionStart = 0.0f;
        float gSkyTransitionTarget = 0.0f;
        float gSkyTransitionTimer = 0.0f;
        float gSkyTransitionDuration = 1.0f;

        //Sunset
        // Vector3 sunsetHorizon = { 0.55f, 0.34f, 0.22f }; // muted warm orange
        // Vector3 sunsetZenith  = { 0.08f, 0.06f, 0.13f }; // muted purple navy
        Vector3 sunsetHorizon = { 1.0f, 0.28f, 0.08f };  // hot orange
        Vector3 sunsetZenith  = { 0.12f, 0.06f, 0.22f }; // purple upper sky
        float sunsetStrength  = 0.25f;

        float timeSec   = 0.0f;
    };

    struct SkyCycle
    {
        bool active = false;

        SkyCyclePhase phase = SkyCyclePhase::DayHold;
        float timer = 0.0f;

        float dayAmount = 0.0f;
        float nightAmount = 0.8f;

        float dayHoldDuration = 120.0f;
        float nightHoldDuration = 120.0f;
        float transitionDuration = 30.0f;
    };




    extern PortalShader gPortal;
    extern WaterShader  gWater;
    extern LavaShader gLava;
    extern BloomShader gBloom;
    extern TreeShader gTree;
    extern SkyShader gSky;
    extern GhostShader gGhost;
    extern CeilingShader gCeiling;
    extern AlphaCutoutShader gAlpha;
    extern ShadowShader gShadow;

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
    void UpdateBloomShaderPerFrame(BloomShader& bs, float dt);

    //enemy shadows
    void InitShadowShader(ShadowShader& ss);

    //dungeon billboard cutout shader. 
    void InitAlphaCutout(AlphaCutoutShader& as);

    //Ceiling lighting
    void InitCeilingShader(CeilingShader& cs); //we need an update for dynamic lights. 

    //Ghost Shader
    void InitGhostShader(GhostShader& gs);
    void UpdateGhostShaderPerFrame(GhostShader& gs);

    //Portal Shader
    void InitPortalShader(Shader& shader, PortalShader& out);
    void ApplyPortalDefaults(PortalShader& ps);

    //Lava shader
    void InitLavaShader(Shader& shader, LavaShader& out, Model& lavaTileModel);
    void UpdateLavaShaderPerFrame(LavaShader& ls, float t, bool isLoadingLevel);
    void UpdatePortalShader(float t);
    void UpdateTreeShader(TreeShader& ts, Camera& camera);
    //WaterShader
    void InitWaterShader(Shader& shader, WaterShader& out, Vector3 terrainScale);
    void UpdateWaterShaderPerFrame(WaterShader& ws, float elapsedTime, const Camera& camera);

    void UpdateSkyCycle(float dt);
    void StartSkyCycle(float dayHold, float nightHold, float transitionDuration, float nightAmount);
    void SetSkyCycleTimer(float amount);
    void StopSkyCycle();
    void SetSkyInstant(float value);
    void StopSkyTransition();
    void StartSkyTransition(float targetValue, float duration);
    void UpdateSkyTransition(float dt);
    void ApplyLevelDefaultSky();
    void ToggleSkyTransition(float duration);
    Vector3 GetCurrentSkyFogColor();
    Vector3 GetCurrentSkyTopFogColor();
    void StartLevelSkyCycle();
}
