#include "shaderSetup.h"
#include <cassert>
#include "raymath.h"
#include "world.h"
#include "lighting.h"

namespace ShaderSetup
{

    PortalShader      gPortal;
    WaterShader       gWater;
    LavaShader        gLava;
    BloomShader       gBloom;
    TreeShader        gTree;
    SkyShader         gSky;
    GhostShader       gGhost;
    CeilingShader     gCeiling;
    SkyCycle          gSkyCycle;
    AlphaCutoutShader gAlpha;
    ShadowShader      gShadow;


    //Shadow Shader
    static void CacheShadowLocations(ShadowShader& ss)
    {
        assert(ss.shader && "Shadow shader must be set");
        Shader& sh = *ss.shader;

        ss.loc_strength = GetShaderLocation(sh, "shadowStrength");
    }

    void InitShadowShader(ShadowShader& ss)
    {
        ss.shader = &R.GetShader("shadowShader");
        Shader& sh = *ss.shader;

        CacheShadowLocations(ss);

        ss.shadowStrength = 0.6f;
        SetShaderValue(sh, ss.loc_strength, &ss.shadowStrength, SHADER_UNIFORM_FLOAT);


        Model& shadowQuad = R.GetModel("shadowQuad");


        shadowQuad.materials[0].shader = sh;
        SetMaterialTexture(&shadowQuad.materials[0], MATERIAL_MAP_DIFFUSE, R.GetTexture("shadowTex"));


    }

    //Alpha cutout

    static void CacheCutoutLocations(AlphaCutoutShader& as)
    {
        assert(as.shader && "Cutout shader must be set");
        Shader& sh = *as.shader;
        as.loc_alphaCutoff = GetShaderLocation(sh, "alphaCutoff");

    }

    void InitAlphaCutout(AlphaCutoutShader& as){
        as.shader = &R.GetShader("cutoutShader");

        Shader& sh = *as.shader;

        CacheCutoutLocations(as);
        //    float cut = 0.1f;
        as.alphaCutoff = 0.1f;
        SetShaderValue(sh, as.loc_alphaCutoff, &as.alphaCutoff, SHADER_UNIFORM_FLOAT);

    }

    //Ceiling

    static void SetGridBounds(float outGrid[4])
    {
        outGrid[0] = gDynamic.minX;
        outGrid[1] = gDynamic.minZ;
        outGrid[2] = gDynamic.sizeX != 0.0f ? 1.0f / gDynamic.sizeX : 0.0f;
        outGrid[3] = gDynamic.sizeZ != 0.0f ? 1.0f / gDynamic.sizeZ : 0.0f;
    }

    static void CacheCeilingLocations(CeilingShader& cs)
    {
        assert(cs.shader && "Ceiling shader must be set");

        Shader& sh = *cs.shader;

        cs.loc_grid = GetShaderLocation(sh, "gridBounds");
        cs.loc_dynStr = GetShaderLocation(sh, "dynStrength");
        cs.loc_amb = GetShaderLocation(sh, "ambientBoost");
        cs.loc_gridSize = GetShaderLocation(sh, "u_GridSize");
        cs.loc_tiling = GetShaderLocation(sh, "u_TilingXZ");

    }

    void InitCeilingShader(CeilingShader& cs){
        cs.shader = &R.GetShader("ceilingShader");

        Shader& sh = *cs.shader;
        Model&  ceilingPlane  = R.GetModel("ceilingPlane");

        CacheCeilingLocations(cs);

        // ---- A) Tell raylib how to feed built-ins for this shader ----
        sh.locs[SHADER_LOC_MATRIX_MVP]    = GetShaderLocation(sh, "mvp");
        sh.locs[SHADER_LOC_MATRIX_MODEL]  = GetShaderLocation(sh, "matModel");
        sh.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(sh, "colDiffuse");

        sh.locs[SHADER_LOC_MAP_DIFFUSE]   = GetShaderLocation(sh, "texture0");
        sh.locs[SHADER_LOC_MAP_EMISSION]  = GetShaderLocation(sh, "texture4");
        sh.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(sh, "texture3");

        // ---- B) Assign shader to ceiling model ----
        ceilingPlane.materials[0].shader = sh;

        // ---- C) Bind textures used by ceiling shader ----
        // Diffuse (ceiling tiles)
        ceilingPlane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("ceilingTexture");
        ceilingPlane.materials[0].maps[MATERIAL_MAP_DIFFUSE].color   = WHITE;
        SetTextureWrap(ceilingPlane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_WRAP_REPEAT);

        // Emission (dynamic lightmap)
        ceilingPlane.materials[0].maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;

        // Occlusion (void mask)
        ceilingPlane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = ceilingMaskTex;
        SetTextureFilter(ceilingPlane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture, TEXTURE_FILTER_POINT);
        SetTextureWrap(ceilingPlane.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture, TEXTURE_WRAP_CLAMP);

        SetGridBounds(cs.grid);

        cs.gridSize = { (float)dungeonWidth, (float)dungeonHeight };
        cs.tiling   = { (float)dungeonWidth * 0.5f, (float)dungeonHeight * 0.5f };

        float dynStrength  = lightConfig.dynStrength;
        float ambientBoost = lightConfig.ambient;

        if (cs.loc_grid >= 0)
            SetShaderValue(sh, cs.loc_grid, cs.grid, SHADER_UNIFORM_VEC4);

        if (cs.loc_dynStr >= 0)
            SetShaderValue(sh, cs.loc_dynStr, &dynStrength, SHADER_UNIFORM_FLOAT);

        if (cs.loc_amb >= 0)
            SetShaderValue(sh, cs.loc_amb, &ambientBoost, SHADER_UNIFORM_FLOAT);

        if (cs.loc_gridSize >= 0)
            SetShaderValue(sh, cs.loc_gridSize, &cs.gridSize, SHADER_UNIFORM_VEC2);

        if (cs.loc_tiling >= 0)
            SetShaderValue(sh, cs.loc_tiling, &cs.tiling, SHADER_UNIFORM_VEC2);

    }

    //Ghost raft
    static void CacheGhostLocations(GhostShader& gs)
    {
        assert(gs.shader && "Ghost.shader must be set");

        Shader& sh = *gs.shader;

        gs.loc_viewPos     = GetShaderLocation(sh, "viewPos");
        gs.loc_baseAlpha   = GetShaderLocation(sh, "baseAlpha");
        gs.loc_ghostColor  = GetShaderLocation(sh, "ghostColor");
        gs.loc_rimPower    = GetShaderLocation(sh, "rimPower");
        gs.loc_rimStrength = GetShaderLocation(sh, "rimStrength");
        gs.loc_darkness    = GetShaderLocation(sh, "nightDarkness");
    }

    void InitGhostShader(GhostShader& gs)
    {
        gs.shader = &R.GetShader("ghostShader");
        assert(gs.shader && "GhostShader.shader must be set");
        Model& raftModel = R.GetModel("raft");
        Shader& sh = *gs.shader;

        for (int i = 0; i < raftModel.materialCount; i++)
        {
            raftModel.materials[i].shader = sh;
        }

        CacheGhostLocations(gs);

        gs.ghostTint = {0.4f, 0.8f, 1.0f}; // spectral blue
        gs.alpha = 0.35f;
        gs.rimPow = 2.0f;
        gs.rimStr = 1.2f;
        gs.nightDarkness = ShaderSetup::gSky.skyTransition;
        gs.camPos = CameraSystem::Get().Active().position;

        SetShaderValue(sh, gs.loc_darkness, &gs.nightDarkness, SHADER_UNIFORM_FLOAT);

        SetShaderValue(sh, gs.loc_viewPos, &gs.camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, gs.loc_ghostColor, &gs.ghostTint, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, gs.loc_baseAlpha, &gs.alpha, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, gs.loc_rimPower, &gs.rimPow, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, gs.loc_rimStrength, &gs.rimStr, SHADER_UNIFORM_FLOAT);

    }

    void UpdateGhostShaderPerFrame(GhostShader& gs){
        Shader& sh = *gs.shader;
        gs.nightDarkness = ShaderSetup::gSky.skyTransition;
        gs.camPos = CameraSystem::Get().Active().position;
        SetShaderValue(sh, gs.loc_darkness, &gs.nightDarkness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, gs.loc_viewPos, &gs.camPos, SHADER_UNIFORM_VEC3);
    }



    //Portal
    static void CachePortalLocations(PortalShader& ps)
    {
        assert(ps.shader && "PortalShader.shader must be set");

        Shader& sh = *ps.shader;

        ps.loc_speed         = GetShaderLocation(sh, "u_speed");
        ps.loc_swirlStrength = GetShaderLocation(sh, "u_swirlStrength");
        ps.loc_swirlScale    = GetShaderLocation(sh, "u_swirlScale");
        ps.loc_colorA        = GetShaderLocation(sh, "u_colorA");
        ps.loc_colorB        = GetShaderLocation(sh, "u_colorB");
        ps.loc_edgeFeather   = GetShaderLocation(sh, "u_edgeFeather");
        ps.loc_rings         = GetShaderLocation(sh, "u_rings");
        ps.loc_glowBoost     = GetShaderLocation(sh, "u_glowBoost");
        ps.portalOpenLoc =    GetShaderLocation(sh, "u_openAmount");
    
    }

    void ApplyPortalDefaults(PortalShader& ps)
    {
        assert(ps.shader && "PortalShader.shader must be set");

        Shader& sh = *ps.shader;

        SetShaderValue(sh, ps.loc_speed,         &ps.speed,         SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ps.loc_swirlStrength, &ps.swirlStrength, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ps.loc_swirlScale,    &ps.swirlScale,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ps.loc_edgeFeather,   &ps.edgeFeather,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ps.loc_rings,         &ps.rings,         SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ps.loc_glowBoost,     &ps.glowBoost,     SHADER_UNIFORM_FLOAT);

        SetShaderValue(sh, ps.loc_colorA,        &ps.colorA,        SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ps.loc_colorB,        &ps.colorB,        SHADER_UNIFORM_VEC3);
    }

    void InitPortalShader(Shader& shader, PortalShader& out)
    {
        out.shader = &shader;

        CachePortalLocations(out);
        ApplyPortalDefaults(out);
    }

    //WATER
    static void CacheWaterLocations(WaterShader& ws)
    {
        assert(ws.shader && "WaterShader.shader must be set before caching locations");
        Shader& sh = *ws.shader;

        ws.loc_WaterCenterXZ = GetShaderLocation(sh, "u_WaterCenterXZ");
        ws.loc_PatchHalfSize = GetShaderLocation(sh, "u_PatchHalfSize");
        ws.loc_FadeStart     = GetShaderLocation(sh, "u_FadeStart");
        ws.loc_FadeEnd       = GetShaderLocation(sh, "u_FadeEnd");
        ws.loc_cameraPos     = GetShaderLocation(sh, "cameraPos");
        //ws.loc_waterLevel    = GetShaderLocation(sh, "waterLevel");
        ws.loc_waterColor    = GetShaderLocation(sh, "u_waterColor");
        ws.loc_isSwamp       = GetShaderLocation(sh, "u_isSwamp");

        ws.loc_skyColorTop   = GetShaderLocation(sh, "u_SkyColorTop");
        ws.loc_skyColorHorz  = GetShaderLocation(sh, "u_SkyColorHorizon");
        ws.loc_skyReflectStrength  = GetShaderLocation(sh, "u_SkyReflectionStrength");
        ws.loc_waterNightDark  = GetShaderLocation(sh, "u_WaterNightDarkness");

    }

    static void ApplyWaterConstants(WaterShader& ws)
    {
        assert(ws.shader && "WaterShader.shader must be set before applying constants");
        Shader& sh = *ws.shader;

        // These do NOT need to be set every frame unless you change them.
        SetShaderValue(sh, ws.loc_PatchHalfSize, &ws.patchHalf, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ws.loc_FadeStart,     &ws.fadeStart, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ws.loc_FadeEnd,       &ws.fadeEnd,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ws.loc_waterLevel,    &waterHeightY, SHADER_UNIFORM_FLOAT);

        
    }

    void InitWaterShader(Shader& shader, WaterShader& out, Vector3 terrainScale)
    {
        out.shader = &shader;
        R.GetModel("waterModel").materials[0].shader = shader;

        // Precompute world bounds from terrainScale (centered at origin)
        out.worldMinXZ  = { -terrainScale.x * 0.5f, -terrainScale.z * 0.5f };
        out.worldSizeXZ = {  terrainScale.x,         terrainScale.z        };

        CacheWaterLocations(out);
        ApplyWaterConstants(out);
        // Note: per-frame uniforms are set in UpdateWaterShaderPerFrame()
    }

    void UpdateWaterShaderPerFrame(WaterShader& ws, float elapsedTime, const Camera& camera)
    {
        assert(ws.shader && "WaterShader.shader must be initialized");
        Shader& sh = *ws.shader;
        Vector3 camPos = camera.position;
        // Compute world bounds
        Vector2 worldMin = ws.worldMinXZ;
        Vector2 worldMax = { ws.worldMinXZ.x + ws.worldSizeXZ.x,
                            ws.worldMinXZ.y + ws.worldSizeXZ.y };

        // Clamp the patch center so edge + feather stays inside world bounds
        float minX = worldMin.x + (ws.patchHalf - ws.feather);
        float maxX = worldMax.x - (ws.patchHalf - ws.feather);
        float minZ = worldMin.y + (ws.patchHalf - ws.feather);
        float maxZ = worldMax.y - (ws.patchHalf - ws.feather);

        Vector2 centerXZ = {
            Clamp(camera.position.x, minX, maxX),
            Clamp(camera.position.z, minZ, maxZ)
        };

        float nightT = ShaderSetup::gSky.skyTransition;

        float waterReflectStrength = Lerp(0.15f, 0.65f, nightT);
        float waterNightDarkness   = nightT;

        Vector3 skyTop     = ShaderSetup::GetCurrentSkyTopFogColor();
        Vector3 skyHorizon = ShaderSetup::GetCurrentSkyFogColor();

        SetShaderValue(sh, ws.loc_skyColorTop, &skyTop, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ws.loc_skyColorHorz, &skyHorizon, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ws.loc_skyReflectStrength, &waterReflectStrength, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ws.loc_waterNightDark, &waterNightDarkness, SHADER_UNIFORM_FLOAT);

        float patchSize = 14000.0f;

        SetShaderValue(sh, ws.loc_PatchHalfSize, &patchSize, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ws.loc_WaterCenterXZ, &centerXZ,       SHADER_UNIFORM_VEC2);
        SetShaderValue(sh, ws.loc_cameraPos,     &camera.position, SHADER_UNIFORM_VEC3);

        //water shader needs cameraPos for reasons. 
        SetShaderValue(sh, ws.loc_cameraPos, &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, GetShaderLocation(sh, "time"), &elapsedTime, SHADER_UNIFORM_FLOAT);
        int isSwamp = CurrentLevelIs("Swamp") ? 1 : 0;
        SetShaderValue(sh, ws.loc_isSwamp, &isSwamp, RL_SHADER_UNIFORM_INT);

    }

    //LAVA
    static void CacheLavaLocations(LavaShader& ls)
    {
        assert(ls.shader && "LavaShader.shader must be set");
        Shader& sh = *ls.shader;

        ls.locTime  = GetShaderLocation(sh, "uTime");
        ls.locDir   = GetShaderLocation(sh, "uScrollDir");
        ls.locSpeed = GetShaderLocation(sh, "uSpeed");
        ls.locOff   = GetShaderLocation(sh, "uWorldOffset");
        ls.locScale = GetShaderLocation(sh, "uUVScale");
        ls.locFreq  = GetShaderLocation(sh, "uDistortFreq");
        ls.locAmp   = GetShaderLocation(sh, "uDistortAmp");
        ls.locEmis  = GetShaderLocation(sh, "uEmissive");
        ls.locGain  = GetShaderLocation(sh, "uEmissiveGain");
    }

    static void BindShaderToAllMaterials(Model& m, Shader& sh)
    {
        for (int i = 0; i < m.materialCount; ++i)
            m.materials[i].shader = sh;
    }

    static void ApplyLavaStaticUniforms(LavaShader& ls)
    {
        assert(ls.shader && "LavaShader.shader must be set");
        Shader& sh = *ls.shader;

        // Compute UV scale from tileSize
        // Your original: float uvsPerWorldUnit = 0.5 / tileSize;
        float uvScale = ls.uvsPerWorldUnit / ls.tileSize;

        SetShaderValue(sh, ls.locDir,   &ls.scrollDir,     SHADER_UNIFORM_VEC2);
        SetShaderValue(sh, ls.locSpeed, &ls.speed,         SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ls.locFreq,  &ls.distortFreq,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ls.locAmp,   &ls.distortAmp,    SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ls.locEmis,  &ls.emissive,      SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ls.locGain,  &ls.emissiveGain,  SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ls.locOff,   &ls.worldOffset,   SHADER_UNIFORM_VEC2);
        SetShaderValue(sh, ls.locScale, &uvScale,          SHADER_UNIFORM_FLOAT);
    }

    void InitLavaShader(Shader& shader, LavaShader& out, Model& lavaTileModel)
    {
        out.shader = &shader;
        out.tileSize = tileSize;

        // Important: bind the shader to the model materials once.
        BindShaderToAllMaterials(lavaTileModel, shader);

        CacheLavaLocations(out);
        ApplyLavaStaticUniforms(out);

        // Note: uTime is dynamic; we set it in UpdateLavaShaderPerFrame()
    }



    //Bloom shader
    static void CacheBloomLocations(BloomShader& bs)
    {
        assert(bs.shader && "BloomShader.shader must be set");
        Shader& sh = *bs.shader;

        bs.loc_bloomStrength   = GetShaderLocation(sh, "bloomStrength");
        bs.loc_exposure        = GetShaderLocation(sh, "uExposure");
        bs.loc_toneMapOperator = GetShaderLocation(sh, "uToneMapOperator");
        bs.loc_resolution      = GetShaderLocation(sh, "resolution");
        bs.loc_vignetteMode = GetShaderLocation(sh,    "vignetteMode");

        bs.loc_baseVignetteStrength = GetShaderLocation(sh, "baseVignetteStrength");
        bs.loc_vignetteIntensity = GetShaderLocation(sh, "vignetteIntensity");
        bs.loc_fadeToBlack = GetShaderLocation(sh, "fadeToBlack");
        bs.loc_dungeonDarkness = GetShaderLocation(sh, "dungeonDarkness");
        bs.loc_dungeonContrast = GetShaderLocation(sh, "dungeonContrast");
        bs.loc_isDungeon = GetShaderLocation(sh, "isDungeon");

        bs.loc_letterboxAmount = GetShaderLocation(sh, "letterboxAmount");
        bs.loc_letterboxTarget = GetShaderLocation(sh, "letterboxTarget");
        bs.loc_letterboxSoftness = GetShaderLocation(sh, "letterboxSoftness");

        
    }

    void ApplyBloomParams(BloomShader& bs)
    {
        //inital values
        assert(bs.shader && "BloomShader must be initialized before applying params");
        Shader& sh = *bs.shader;

        SetShaderValue(sh, bs.loc_bloomStrength,   &bs.bloomStrength, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_exposure,        &bs.exposure,      SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_toneMapOperator, &bs.toneOp,        SHADER_UNIFORM_INT);
        SetShaderValue(sh, bs.loc_resolution,      &bs.resolution,    SHADER_UNIFORM_VEC2);

        SetShaderValue(sh, bs.vignetteMode, &bs.vignetteMode, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_vignetteIntensity, &bs.vignetteIntensity, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_baseVignetteStrength, &bs.vignetteStrengthValue, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_fadeToBlack, &bs.fadeToBlack, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_dungeonDarkness, &bs.dungeonDarkness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_dungeonContrast, &bs.dungeonContrast, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_isDungeon, &bs.isDungeon, SHADER_UNIFORM_INT);


        SetShaderValue(sh, bs.loc_letterboxAmount, &bs.letterboxAmount, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_letterboxTarget, &bs.letterboxTarget, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_letterboxSoftness, &bs.letterboxSoftness, SHADER_UNIFORM_FLOAT);

    }

    void InitBloomShader(Shader& shader, BloomShader& out)
    {
        out.shader = &shader;

        CacheBloomLocations(out);

        // Initial defaults (you can change these after init)
        out.bloomStrength = 0.0f; //0.0
        out.exposure      = 1.0f; 
        out.toneOp        = 0;
        out.resolution    = { (float)GetScreenWidth(), (float)GetScreenHeight() };

        out.vignetteMode = 0;
        out.vignetteStrengthValue = isDungeon ? 0.8f : 0.25f;
        if (CurrentLevelIs("Ship")) out.vignetteStrengthValue = 0.0f; // no vignette on ship level
        out.vignetteIntensity = 0.0f;
        out.fadeToBlack = 0.0f;
        out.dungeonDarkness = -0.1f;
        out.dungeonContrast = 1.0f;
        out.isDungeon = isDungeon ? 1 : 0; 

        out.letterboxAmount = 0.0f;
        out.letterboxTarget = 0.0f;
        out.letterboxSoftness = 0.004f;

        ApplyBloomParams(out);
    }

    void SetBloomResolution(BloomShader& bs, int screenW, int screenH)
    {
        bs.resolution = { (float)screenW, (float)screenH };
        ApplyBloomParams(bs);
    }

    void SetBloomTonemap(BloomShader& bs, bool isDungeon, float islandExposure, float dungeonExposure)
    {
        bs.toneOp   = isDungeon ? 1 : 0;
        bs.exposure = isDungeon ? dungeonExposure : islandExposure;
        ApplyBloomParams(bs);
    }
    void SetBloomStrength(BloomShader& bs, float strength)
    {
        bs.bloomStrength = strength;
        ApplyBloomParams(bs);
    }

    //treeShader
    static void BindShaderToModels(Shader& shader, std::initializer_list<Model*> models)
    {
        for (Model* m : models)
        {
            if (!m) continue;
            for (int i = 0; i < m->materialCount; ++i)
            {
                m->materials[i].shader = shader;
                m->materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE; // matches your code
            }
        }
    }

    static void CacheTreeLocations(TreeShader& ts)
    {
        assert(ts.shader && "TreeShader.shader must be set");
        Shader& sh = *ts.shader;

        ts.loc_skyTop   = GetShaderLocation(sh, "u_SkyColorTop");
        ts.loc_skyHorz  = GetShaderLocation(sh, "u_SkyColorHorizon");
        ts.loc_fogStart = GetShaderLocation(sh, "u_FogStart");
        ts.loc_fogEnd   = GetShaderLocation(sh, "u_FogEnd");
        ts.loc_seaLevel = GetShaderLocation(sh, "u_SeaLevel");
        ts.loc_falloff  = GetShaderLocation(sh, "u_FogHeightFalloff");
        ts.loc_alphaCut = GetShaderLocation(sh, "alphaCutoff");
        ts.loc_useFog   = GetShaderLocation(sh, "u_UseFog");
        ts.loc_cam      = GetShaderLocation(sh, "cameraPos");
        ts.loc_darkenss = GetShaderLocation(sh, "u_ModelNightDarkness");

    }

    void ApplyTreeFogParams(TreeShader& ts)
    {
        assert(ts.shader && "TreeShader not initialized");
        Shader& sh = *ts.shader;
        ts.fogStart = (currentGameState == GameState::Menu) ? GameSettings::treefogStartMenu : GameSettings::treeFogStart;
        ts.fogEnd = GameSettings::treeFogEnd;
        SetShaderValue(sh, ts.loc_skyTop,   &ts.skyTop,   SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ts.loc_skyHorz,  &ts.skyHorz,  SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ts.loc_fogStart, &ts.fogStart, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_fogEnd,   &ts.fogEnd,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_seaLevel, &ts.seaLevel, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_falloff,  &ts.falloff,  SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_alphaCut, &ts.alphaCutoff, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_useFog,   &ts.useFog, SHADER_UNIFORM_INT);
    }

    void SetTreeAlphaCutoff(TreeShader& ts, float cutoff)
    {
        ts.alphaCutoff = cutoff;
        ApplyTreeFogParams(ts); // includes alphaCut in same apply; simple for now
    }


    void InitTreeShader(Shader& shader,
                        TreeShader& out,
                        std::initializer_list<Model*> modelsToBind)
    {
        out.shader = &shader;

        // Hook raylib standard locations once
        // (These are used by raylib internally when drawing models with material maps.)
        shader.locs[SHADER_LOC_MAP_ALBEDO]    = GetShaderLocation(shader, "textureDiffuse");
        shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "colDiffuse");

        CacheTreeLocations(out);

        // Bind shader to all models that should use it
        BindShaderToModels(shader, modelsToBind);

        // Apply the shared fog + alpha params once
        ApplyTreeFogParams(out);
    }



    //Sky Shader
    static void CacheSkyLocations(SkyShader& ss)
    {
        assert(ss.shader && "SkyShader.shader must be set");
        Shader& sh = *ss.shader;
        ss.loc_time      = GetShaderLocation(sh, "time");
        ss.loc_isSwamp =   GetShaderLocation(sh, "isSwamp");
        ss.loc_isDungeon = GetShaderLocation(sh, "isDungeon");
        ss.skyTransitionLoc = GetShaderLocation(sh, "skyTransition");
    }

    static void BindSkyShaderToModel(Model& skyModel, Shader& sh)
    {
        // Your code used materials[0]; keep that, but make it explicit.
        if (skyModel.materialCount > 0)
            skyModel.materials[0].shader = sh;
    }


    void InitSkyShader(Shader& shader, SkyShader& out, Model& skyModel, bool isDungeon)
    {
        out.shader = &shader;

        BindSkyShaderToModel(skyModel, shader);
        CacheSkyLocations(out);
        //set isSwamp on init
        int isSwamp = CurrentLevelIs("swamp") ? 1 : 0;
        int Dungeon = isDungeon ? 1 : 0;

        //if (CurrentLevelIs("Ship")) Dungeon = 0; // set dungeon to false for ship level so we render blue sky not stars. 

        out.isSwamp = isSwamp ? 1 : 0;
        out.isDungeon = Dungeon;
        SetShaderValue(shader, out.loc_isSwamp, &out.isSwamp, SHADER_UNIFORM_INT);
        SetShaderValue(shader, out.loc_isDungeon, &out.isDungeon, SHADER_UNIFORM_INT);
    

        // (Time is updated per-frame, so no need to set here)
    }


    //UPDATE

    void UpdateBloomShaderPerFrame(BloomShader& bs, float dt){
        bs.letterboxAmount = SmoothTo(bs.letterboxAmount, bs.letterboxTarget, 6.0f, dt);
        Shader sh = R.GetShader("bloomShader");

        Vector2 screenRes = {(float)GetScreenWidth(), (float)GetScreenHeight()}; //update resolution every frame incase fullscreen.

        SetShaderValue(sh, bs.loc_resolution, &screenRes, SHADER_UNIFORM_VEC2); 

        SetShaderValue(sh, bs.loc_fadeToBlack, &bs.fadeToBlack, SHADER_UNIFORM_FLOAT);

        //how is this working?
        SetShaderValue(sh, bs.loc_vignetteMode, &bs.vignetteMode, SHADER_UNIFORM_INT);

        SetShaderValue(sh, bs.loc_vignetteIntensity, &bs.vignetteIntensity, SHADER_UNIFORM_FLOAT);

        SetShaderValue(sh, bs.loc_letterboxAmount,
                    &bs.letterboxAmount, SHADER_UNIFORM_FLOAT);

        SetShaderValue(sh, bs.loc_letterboxSoftness,
                    &bs.letterboxSoftness, SHADER_UNIFORM_FLOAT);
    }

    void UpdateLavaShaderPerFrame(LavaShader& ls, float t, bool isLoadingLevel)
    {
        if (isLoadingLevel) return;
        assert(ls.shader && "LavaShader must be initialized before updating");

        Shader& sh = *ls.shader;
        SetShaderValue(sh, ls.locTime, &t, SHADER_UNIFORM_FLOAT);
    }

    void UpdatePortalShader(float t){
        Shader& portalShader = R.GetShader("portalShader");
        int loc_time_p = GetShaderLocation(R.GetShader("portalShader"), "u_time");
        //portal
        SetShaderValue(portalShader, loc_time_p, &t, SHADER_UNIFORM_FLOAT);

        //need spawners

    }

    void UpdateTreeShader(TreeShader& ts,  Camera& camera){
        Shader& treeShader = R.GetShader("treeShader");
        Vector3 camPos = camera.position;
        
        ts.useFog = GameSettings::useFog;
        ts.nightDarkness = ShaderSetup::gSky.skyTransition;
        ts.skyHorz = ShaderSetup::GetCurrentSkyFogColor();
        ts.skyTop  = ShaderSetup::GetCurrentSkyTopFogColor();

        SetShaderValue(treeShader,   ts.loc_cam,   &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(treeShader, ts.loc_useFog, &ts.useFog, SHADER_UNIFORM_INT);
        SetShaderValue(treeShader, ts.loc_darkenss, &ts.nightDarkness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(treeShader, ts.loc_skyHorz, &ts.skyHorz, SHADER_UNIFORM_VEC3);
        SetShaderValue(treeShader, ts.loc_skyTop, &ts.skyTop, SHADER_UNIFORM_VEC3);

 
        
    }

    void UpdateSkyShaderPerFrame(SkyShader& ss, float timeSeconds)
    {
        assert(ss.shader && "SkyShader must be initialized");
        Shader& sh = *ss.shader;

        //ss.skyTransition = 0.5f + 0.5f * sinf(GetTime() * 0.25f);
        ss.timeSec = timeSeconds;
        SetShaderValue(sh, ss.loc_time, &ss.timeSec, SHADER_UNIFORM_FLOAT);

        SetShaderValue(
            sh,
            ss.skyTransitionLoc,
            &ss.skyTransition,
            SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(sh, GetShaderLocation(sh, "u_SunsetHorizonColor"),
                    &ss.sunsetHorizon, SHADER_UNIFORM_VEC3);

        SetShaderValue(sh, GetShaderLocation(sh, "u_SunsetZenithColor"),
                    &ss.sunsetZenith, SHADER_UNIFORM_VEC3);

        SetShaderValue(sh, GetShaderLocation(sh, "u_SunsetStrength"),
                    &ss.sunsetStrength, SHADER_UNIFORM_FLOAT);


    }

    void SetSkyCycleTimer(float amount){
        gSkyCycle.timer = amount;
    }

    void StopSkyCycle()
    {
        gSkyCycle.active = false;
        gSkyCycle.timer = 0.0f;
    }

    void StartSkyCycle(float dayHold, float nightHold, float transitionDuration, float nightAmount)
    {
        gSkyCycle.active = true;
        gSkyCycle.phase = SkyCyclePhase::DayHold;
        gSkyCycle.timer = 0.0f;

        gSkyCycle.dayAmount = 0.0f;
        gSkyCycle.nightAmount = nightAmount;

        gSkyCycle.dayHoldDuration = dayHold;
        gSkyCycle.nightHoldDuration = nightHold;
        gSkyCycle.transitionDuration = transitionDuration;

        //gSky.skyTransition = gSkyCycle.dayAmount;
    }


    void UpdateSkyCycle(float dt)
    {
        if (!gSkyCycle.active)
            return;

        gSkyCycle.timer += dt;

        switch (gSkyCycle.phase)
        {
            case SkyCyclePhase::DayHold:
            {
                if (gSkyCycle.timer >= gSkyCycle.dayHoldDuration)
                {
                    gSkyCycle.timer = 0.0f;
                    gSkyCycle.phase = SkyCyclePhase::ToNight;

                    StartSkyTransition(
                        gSkyCycle.nightAmount,
                        gSkyCycle.transitionDuration
                    );
                }
            } break;

            case SkyCyclePhase::ToNight:
            {
                if (gSkyCycle.timer >= gSkyCycle.transitionDuration)
                {
                    gSkyCycle.timer = 0.0f;
                    gSkyCycle.phase = SkyCyclePhase::NightHold;
                }
            } break;

            case SkyCyclePhase::NightHold:
            {
                if (gSkyCycle.timer >= gSkyCycle.nightHoldDuration)
                {
                    gSkyCycle.timer = 0.0f;
                    gSkyCycle.phase = SkyCyclePhase::ToDay;

                    StartSkyTransition(
                        gSkyCycle.dayAmount,
                        gSkyCycle.transitionDuration
                    );
                }
            } break;

            case SkyCyclePhase::ToDay:
            {
                if (gSkyCycle.timer >= gSkyCycle.transitionDuration)
                {
                    gSkyCycle.timer = 0.0f;
                    gSkyCycle.phase = SkyCyclePhase::DayHold;
                }
            } break;
        }
    }


    void SetSkyInstant(float value)
    {
        ShaderSetup::SkyShader& ss = gSky;

        value = Clamp01(value);

        ss.skyTransition = value;

        ss.gSkyTransitionActive = false;
        ss.gSkyTransitionTimer = 0.0f;
        ss.gSkyTransitionStart = value;
        ss.gSkyTransitionTarget = value;
    }

    void StopSkyTransition()
    {
        ShaderSetup::SkyShader& ss = gSky;

        ss.gSkyTransitionActive = false;
        ss.gSkyTransitionTimer = 0.0f;
        ss.gSkyTransitionStart = ss.skyTransition;
        ss.gSkyTransitionTarget = ss.skyTransition;
    }

    void ToggleSkyTransition(float duration)
    {
        float nightTarget = isDungeon ? 1.0f : 0.8f; //don't go full night time on island levels. 
        float target = (ShaderSetup::gSky.skyTransition < 0.5f) ? nightTarget : 0.0f;
        StartSkyTransition(target, duration);
    }

    void StartSkyTransition(float targetValue, float duration)
    {
        ShaderSetup::SkyShader& ss = gSky;
        ss.gSkyTransitionStart = ss.skyTransition;
        ss.gSkyTransitionTarget = Clamp01(targetValue);

        ss.gSkyTransitionTimer = 0.0f;
        ss.gSkyTransitionDuration = duration;

        if (ss.gSkyTransitionDuration <= 0.0f)
        {
            ss.skyTransition = ss.gSkyTransitionTarget;
            ss.gSkyTransitionActive = false;
            return;
        }

        ss.gSkyTransitionActive = true;
    }

    void UpdateSkyTransition(float dt)
    {
        
        ShaderSetup::SkyShader& ss = gSky;
        if (!ss.gSkyTransitionActive)
            return;

        ss.gSkyTransitionTimer += dt;

        float t = ss.gSkyTransitionTimer / ss.gSkyTransitionDuration;
        t = SmoothStep01(t);

        ss.skyTransition =
            ss.gSkyTransitionStart +
            (ss.gSkyTransitionTarget - ss.gSkyTransitionStart) * t;

        if (ss.gSkyTransitionTimer >= ss.gSkyTransitionDuration)
        {
            ss.skyTransition = ss.gSkyTransitionTarget;
            ss.gSkyTransitionActive = false;
        }
    }

    void ApplyLevelDefaultSky()
    {

        StopSkyTransition();

        if (CurrentLevelIs("Ship"))
        {
            SetSkyInstant(0.0f); // ship starts day
        }
        else if (isDungeon)
        {
            SetSkyInstant(1.0f); // normal dungeons start night
        }
        else
        {
            SetSkyInstant(0.0f); // islands start day
        }
    }

    Vector3 GetCurrentSkyFogColor()
    {
        // skyAmount: 0 = day, 1 = night
        float t = gSky.skyTransition;

        Vector3 dayFog   = {0.60f, 0.80f, 0.95f};
        Vector3 nightFog = { 0.0f, 0.0f, 0.0f };

        return {
            Lerp(dayFog.x, nightFog.x, t),
            Lerp(dayFog.y, nightFog.y, t),
            Lerp(dayFog.z, nightFog.z, t)
        };
    }

    Vector3 GetCurrentSkyTopFogColor()
    {
        // skyAmount: 0 = day, 1 = night
        float t = gSky.skyTransition;

        Vector3 dayFog   = {0.55f, 0.75f, 1.00f};
        Vector3 nightFog = { 0.0f, 0.0f, 0.0f };

        return {
            Lerp(dayFog.x, nightFog.x, t),
            Lerp(dayFog.y, nightFog.y, t),
            Lerp(dayFog.z, nightFog.z, t)
        };
    }

    void StartLevelSkyCycle(){
        if (CurrentLevelIs("MiddleIsland")) //start day night cycle. 
        {
            StartSkyCycle(
                30.0f, // day hold
                30.0f, // night hold
                15.0f,  // transition
                0.97f    // outdoor night/twilight amount
            );
        }

        if (CurrentLevelIs("River")) //start day night cycle. 
        {
            StartSkyCycle(
                30.0f, // day hold
                120.0f, // night hold
                15.0f,  // transition
                0.97f    // outdoor night/twilight amount
            );
            
            SetSkyCycleTimer(25.0f); //start night immediatly, but keep day hold at 30 for later. 
        
        }
    }




}
