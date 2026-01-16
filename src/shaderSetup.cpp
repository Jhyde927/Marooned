#include "shaderSetup.h"
#include <cassert>
#include "raymath.h"
#include "world.h"

namespace ShaderSetup
{

    PortalShader gPortal;
    WaterShader  gWater;
    LavaShader   gLava;
    BloomShader  gBloom;
    TreeShader   gTree;
    SkyShader    gSky;

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

        SetShaderValue(sh, ws.loc_WaterCenterXZ, &centerXZ,       SHADER_UNIFORM_VEC2);
        SetShaderValue(sh, ws.loc_cameraPos,     &camera.position, SHADER_UNIFORM_VEC3);

        //water shader needs cameraPos for reasons. 
        SetShaderValue(sh, ws.loc_cameraPos, &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, GetShaderLocation(sh, "time"), &elapsedTime, SHADER_UNIFORM_FLOAT);
        int isSwamp = (gCurrentLevelIndex == 23) ? 1 : 0;
        Vector3 swampColor = {0.32, 0.45, 0.30};
        Vector3 oceanColor = {0.22, 0.55, 0.88};
        Vector3 waterColor = (isSwamp == 1) ? swampColor : oceanColor;
        SetShaderValue(R.GetShader("waterShader"), ws.loc_waterColor, &waterColor, SHADER_UNIFORM_VEC3);
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
    }

    void ApplyBloomParams(BloomShader& bs)
    {
        assert(bs.shader && "BloomShader must be initialized before applying params");
        Shader& sh = *bs.shader;

        SetShaderValue(sh, bs.loc_bloomStrength,   &bs.bloomStrength, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_exposure,        &bs.exposure,      SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, bs.loc_toneMapOperator, &bs.toneOp,        SHADER_UNIFORM_INT);
        SetShaderValue(sh, bs.loc_resolution,      &bs.resolution,    SHADER_UNIFORM_VEC2);
    }

    void InitBloomShader(Shader& shader, BloomShader& out)
    {
        out.shader = &shader;

        CacheBloomLocations(out);

        // Initial defaults (you can change these after init)
        out.bloomStrength = 0.0f;
        out.exposure      = 1.0f;
        out.toneOp        = 0;
        out.resolution    = { (float)GetScreenWidth(), (float)GetScreenHeight() };

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
    }

    void ApplyTreeFogParams(TreeShader& ts)
    {
        assert(ts.shader && "TreeShader not initialized");
        Shader& sh = *ts.shader;
        float fogStart = (currentGameState == GameState::Menu) ? 10000 : 100;
        SetShaderValue(sh, ts.loc_skyTop,   &ts.skyTop,   SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ts.loc_skyHorz,  &ts.skyHorz,  SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, ts.loc_fogStart, &fogStart, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_fogEnd,   &ts.fogEnd,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_seaLevel, &ts.seaLevel, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, ts.loc_falloff,  &ts.falloff,  SHADER_UNIFORM_FLOAT);

        SetShaderValue(sh, ts.loc_alphaCut, &ts.alphaCutoff, SHADER_UNIFORM_FLOAT);
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
        int isSwamp = (gCurrentLevelIndex == 23) ? 1 : 0;
        int Dungeon = isDungeon ? 1 : 0;
        out.isSwamp = isSwamp ? 1 : 0;
        out.isDungeon = Dungeon;
        SetShaderValue(shader, out.loc_isSwamp, &out.isSwamp, SHADER_UNIFORM_INT);
        SetShaderValue(shader, out.loc_isDungeon, &out.isDungeon, SHADER_UNIFORM_INT);

        // (Time is updated per-frame, so no need to set here)
    }


    //UPDATE

    void UpdateLavaShaderPerFrame(LavaShader& ls, float t, bool isLoadingLevel)
    {
        if (isLoadingLevel) return;
        assert(ls.shader && "LavaShader must be initialized before updating");

        Shader& sh = *ls.shader;
        SetShaderValue(sh, ls.locTime, &t, SHADER_UNIFORM_FLOAT);
    }

    void UpdatePortalShader(PortalShader& ps, float t){
        
        int loc_time_p = GetShaderLocation(R.GetShader("portalShader"), "u_time");
        //portal
        SetShaderValue(R.GetShader("portalShader"), loc_time_p, &t, SHADER_UNIFORM_FLOAT);

    }

    void UpdateTreeShader(TreeShader& ts,  Camera& camera){
        Vector3 camPos = camera.position;
        float fogStart = (currentGameState == GameState::Menu) ? 10000 : 100;
        
        int locCam_Trees   = GetShaderLocation(R.GetShader("treeShader"),   "cameraPos");
        int fogLoc = GetShaderLocation(R.GetShader("treeShader"), "u_FogStart");
        SetShaderValue(R.GetShader("treeShader"),   locCam_Trees,   &camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(R.GetShader("treeShader"), fogLoc, &fogStart, SHADER_UNIFORM_FLOAT);
        
    }

    void UpdateSkyShaderPerFrame(SkyShader& ss, float timeSeconds)
    {
        assert(ss.shader && "SkyShader must be initialized");
        Shader& sh = *ss.shader;

        ss.timeSec = timeSeconds;
        SetShaderValue(sh, ss.loc_time, &ss.timeSec, SHADER_UNIFORM_FLOAT);


    }

}
