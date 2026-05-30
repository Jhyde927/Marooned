#include "vegetation_instanced.h"

#include "raymath.h"
#include "resourceManager.h"
#include "world.h"
#include "game_settings.h"
#include "shadows.h"
#include "utilities.h"
#include "shaderSetup.h"

#include <algorithm>

// These are the CPU-side instance batches.
// Each batch = one model drawn with DrawMeshInstanced().
static VegetationInstanceBatch gPalmTreeBatch;
static VegetationInstanceBatch gBushBatch;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------



static void ApplyInstancedShaderToModel(Model& model, Shader shader)
{
    for (int i = 0; i < model.materialCount; ++i)
    {
        model.materials[i].shader = shader;
    }
}

static Matrix MakeInstanceTransform(Vector3 position, float rotationYDeg, float scale)
{
    Matrix matScale = MatrixScale(scale, scale, scale);
    Matrix matRot   = MatrixRotateY(rotationYDeg * DEG2RAD);
    Matrix matTrans = MatrixTranslate(position.x, position.y, position.z);

    // Same order raylib's DrawModelEx effectively uses:
    // scale -> rotate -> translate
    return MatrixMultiply(MatrixMultiply(matScale, matRot), matTrans);
}

static void PushInstance(
    VegetationInstanceBatch& batch,
    Vector3 position,
    float rotationYDeg,
    float scale
)
{
    batch.positions.push_back(position);
    batch.transforms.push_back(MakeInstanceTransform(position, rotationYDeg, scale));
}

static void DrawBatch(VegetationInstanceBatch& batch, Camera& camera)
{
    if (batch.transforms.empty()) return;

    batch.visibleTransforms.clear();

    const float cullDist = GameSettings::maxDrawDist;
    const float cullDistSq = cullDist * cullDist;

    for (int i = 0; i < (int)batch.transforms.size(); ++i)
    {
        float distSq = Vector3DistanceSqr(camera.position, batch.positions[i]);

        if (distSq < cullDistSq)
        {
            batch.visibleTransforms.push_back(batch.transforms[i]);
        }
    }

    if (batch.visibleTransforms.empty()) return;

    Model& model = R.GetModel(batch.modelName.c_str());

    for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
    {
        int matIndex = model.meshMaterial[meshIndex];

        if (matIndex < 0 || matIndex >= model.materialCount)
        {
            matIndex = 0;
        }

        DrawMeshInstanced(
            model.meshes[meshIndex],
            model.materials[matIndex],
            batch.visibleTransforms.data(),
            (int)batch.visibleTransforms.size()
        );
    }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

namespace VegetationInstanced
{
    void Clear()
    {
        gPalmTreeBatch.Clear();
        gBushBatch.Clear();
    }

    void Generate()
    {
        Clear();

        gPalmTreeBatch.modelName = "palmTreeInstanced";
        gBushBatch.modelName = "bushInstanced";

        generateVegetation();

        for (const TreeInstance& tree : trees)
        {
            float finalScale = tree.scale * tree.randomScale;
           
            Vector3 pos = tree.position;
            pos.x += tree.xOffset;
            pos.y += tree.yOffset;
            pos.z += tree.zOffset;

            PushInstance(gPalmTreeBatch, pos, tree.rotationY, finalScale);
        }

        for (const BushInstance& bush : bushes)
        {
            Vector3 pos = bush.position;
            pos.x += bush.xOffset;
            pos.y += bush.yOffset;
            pos.z += bush.zOffset;

            PushInstance(gBushBatch, pos, bush.rotationY, bush.scale);
        }
    }

    void Draw(Camera& camera)
    {
        if (showVeg){
            SetShaderValues(camera);
            DrawBatch(gPalmTreeBatch, camera);
            DrawBatch(gBushBatch, camera);
        }


    }

    int GetTotalInstanceCount()
    {
        return
            (int)gPalmTreeBatch.transforms.size() +
            (int)gBushBatch.transforms.size();
    }

    int GetVisibleInstanceCount()
    {
        return
            (int)gPalmTreeBatch.visibleTransforms.size() +
            (int)gBushBatch.visibleTransforms.size();
    }


}

namespace VegetationInstanced
{
    static Shader gShader = { 0 };

    static int locAlphaCutoff = -1;
    static int locCameraPos = -1;
    static int locSkyTop = -1;
    static int locSkyHorizon = -1;
    static int locFogStart = -1;
    static int locFogEnd = -1;
    static int locSeaLevel = -1;
    static int locFogHeightFalloff = -1;
    static int locUseFog = -1;
    static int locModelNightDarkness = -1;

    bool showVeg = true;

    void InitShader()
    {
        gShader = R.GetShader("tree_instanced");

        gShader.locs[SHADER_LOC_MATRIX_MVP] =
            GetShaderLocation(gShader, "mvp");

        // IMPORTANT:
        // In your project, DrawMeshInstanced is using SHADER_LOC_MATRIX_MODEL
        // as the instanceTransform attribute location.
        gShader.locs[SHADER_LOC_MATRIX_MODEL] =
            GetShaderLocationAttrib(gShader, "instanceTransform");

        if (gShader.locs[SHADER_LOC_MATRIX_MODEL] < 0)
        {
            TraceLog(LOG_ERROR, "VEG INST missing instanceTransform");
            return;
        }

        gShader.locs[SHADER_LOC_MAP_DIFFUSE] =
            GetShaderLocation(gShader, "texture0");

        gShader.locs[SHADER_LOC_COLOR_DIFFUSE] =
            GetShaderLocation(gShader, "colDiffuse");

        locAlphaCutoff        = GetShaderLocation(gShader, "alphaCutoff");
        locCameraPos          = GetShaderLocation(gShader, "cameraPos");
        locSkyTop             = GetShaderLocation(gShader, "u_SkyColorTop");
        locSkyHorizon         = GetShaderLocation(gShader, "u_SkyColorHorizon");
        locFogStart           = GetShaderLocation(gShader, "u_FogStart");
        locFogEnd             = GetShaderLocation(gShader, "u_FogEnd");
        locSeaLevel           = GetShaderLocation(gShader, "u_SeaLevel");
        locFogHeightFalloff   = GetShaderLocation(gShader, "u_FogHeightFalloff");
        locUseFog             = GetShaderLocation(gShader, "u_UseFog");
        locModelNightDarkness = GetShaderLocation(gShader, "u_ModelNightDarkness");

        ApplyInstancedShaderToModel(R.GetModel("palmTreeInstanced"), gShader);
        ApplyInstancedShaderToModel(R.GetModel("bushInstanced"), gShader);

       
    }
}

void VegetationInstanced::SetShaderValues(Camera& camera)
{
    Vector3 camPos = camera.position;
    Vector3 topFogColor = ShaderSetup::GetCurrentSkyTopFogColor();
    Vector3 fogHor = ShaderSetup::GetCurrentSkyFogColor();
    float fogStart = (currentGameState == GameState::Menu) ? 10000 : 100;
    float fogEnd    = 16000.0f;
    float seaLevel  = 400.0f; // Visual fog height base, not literal ocean height
    float falloff   = 0.001f; // Lower = taller/softer height fog; helps giant tree tops stay fogged

    float alphaCutoff = 0.50f;

    SetShaderValue(gShader, locCameraPos, &camPos, SHADER_UNIFORM_VEC3);

    SetShaderValue(gShader, locAlphaCutoff, &alphaCutoff, SHADER_UNIFORM_FLOAT);

    SetShaderValue(gShader, locSkyTop, &topFogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(gShader, locSkyHorizon, &fogHor, SHADER_UNIFORM_VEC3);

    SetShaderValue(gShader, locFogStart, &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gShader, locFogEnd, &fogEnd, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gShader, locSeaLevel, &seaLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gShader, locFogHeightFalloff, &falloff, SHADER_UNIFORM_FLOAT);

    int useFog = 1;
    SetShaderValue(gShader, locUseFog, &useFog, SHADER_UNIFORM_INT);

    float nightDarkness = ShaderSetup::gSky.skyTransition;
    SetShaderValue(gShader, locModelNightDarkness, &nightDarkness, SHADER_UNIFORM_FLOAT);
}