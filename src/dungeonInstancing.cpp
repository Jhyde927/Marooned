#include "dungeonInstancing.h"

#include "raymath.h"
#include "resourceManager.h"      // keep lowercase for Linux
#include "dungeonGeneration.h"    // FloorTile, FloorType, CurrentLevelIs, etc.
#include "shaderSetup.h"          // if gDynamic lives there, or include wherever it really lives
#include "world.h"
#include <iostream>
#include "viewCone.h"
#include "lighting.h"
#include "game_settings.h"


// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------

std::vector<DungeonInstanceSource> gDungeonInstanceSources;

DungeonInstancingBatch gGrayFloorInstancing;
DungeonInstancingBatch gWoodFloorInstancing;

void UpdateDungeonInstancingDebugCounts()
{
    int totalFloors = 0;

    for (const DungeonInstanceSource& src : gDungeonInstanceSources)
    {
        if (src.kind == DungeonInstanceKind::FloorGray ||
            src.kind == DungeonInstanceKind::FloorWood)
        {
            totalFloors++;
        }
    }

    GameSettings::gTotalFloorTileCount = totalFloors;

    GameSettings::gVisibleFloorTileCount =
        (int)(gGrayFloorInstancing.transforms.size() +
              gWoodFloorInstancing.transforms.size());
}

static void SetDungeonInstancingShaderValues(DungeonInstancingBatch& batch)
{
    if (!batch.initialized) return;

    Shader& sh = batch.material.shader;

    sh.locs[SHADER_LOC_MATRIX_MVP] =
        GetShaderLocation(sh, "mvp");

    sh.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocationAttrib(sh, "instanceTransform");

    sh.locs[SHADER_LOC_MAP_DIFFUSE] =
        GetShaderLocation(sh, "texture0");

    sh.locs[SHADER_LOC_MAP_EMISSION] =
        GetShaderLocation(sh, "texture4");

    // Do NOT overwrite diffuse.
    // Only refresh the runtime lightmap/emission texture.
    batch.material.maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;

    int locGrid   = GetShaderLocation(sh, "gridBounds");
    int locDynStr = GetShaderLocation(sh, "dynStrength");
    int locAmb    = GetShaderLocation(sh, "ambientBoost");

    float grid[4] = {
        gDynamic.minX,
        gDynamic.minZ,
        gDynamic.sizeX ? 1.0f / gDynamic.sizeX : 0.0f,
        gDynamic.sizeZ ? 1.0f / gDynamic.sizeZ : 0.0f
    };

    float dynStrength  = lightConfig.dynStrength;
    float ambientBoost = lightConfig.ambient;

    if (locGrid >= 0)
        SetShaderValue(sh, locGrid, grid, SHADER_UNIFORM_VEC4);

    if (locDynStr >= 0)
        SetShaderValue(sh, locDynStr, &dynStrength, SHADER_UNIFORM_FLOAT);

    if (locAmb >= 0)
        SetShaderValue(sh, locAmb, &ambientBoost, SHADER_UNIFORM_FLOAT);
}

// ------------------------------------------------------------
// Generic helpers
// ------------------------------------------------------------

static Matrix MakeDungeonInstanceTransform(
    const Vector3& pos,
    const Vector3& scale,
    float rotationYDeg = 0.0f
)
{
    Matrix scaleMat = MatrixScale(scale.x, scale.y, scale.z);
    Matrix rotMat   = MatrixRotateY(rotationYDeg * DEG2RAD);

    Matrix transform = MatrixMultiply(scaleMat, rotMat);

    transform.m12 = pos.x;
    transform.m13 = pos.y;
    transform.m14 = pos.z;

    return transform;
}

static Matrix MakeFloorTransform(const Vector3& pos)
{
    return MakeDungeonInstanceTransform(
        pos,
        Vector3{ 700.0f, 700.0f, 700.0f },
        0.0f
    );
}

static void InitDungeonInstancingBatch(
    DungeonInstancingBatch& batch,
    const char* modelKey,
    const char* shaderKey
)
{
    if (batch.initialized) return;

    batch.shader = R.GetShader(shaderKey);

    batch.shader.locs[SHADER_LOC_MATRIX_MVP] =
        GetShaderLocation(batch.shader, "mvp");

    // raylib DrawMeshInstanced expects this attrib location to be
    // stored in SHADER_LOC_MATRIX_MODEL.
    batch.shader.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocationAttrib(batch.shader, "instanceTransform");

    if (batch.shader.locs[SHADER_LOC_MATRIX_MODEL] < 0)
    {
        TraceLog(LOG_ERROR, "Missing instanceTransform attrib for %s", modelKey);
        batch.initialized = false;
        return;
    }

    Model& model = R.GetModel(modelKey);

    if (model.meshCount <= 0 || model.meshes == nullptr)
    {
        TraceLog(LOG_ERROR, "Model %s has no meshes", modelKey);
        batch.initialized = false;
        return;
    }

    const int meshIndex = 0;

    int matIndex = 0;
    if (model.meshMaterial != nullptr)
    {
        matIndex = model.meshMaterial[meshIndex];
    }

    if (matIndex < 0 || matIndex >= model.materialCount)
    {
        TraceLog(LOG_ERROR, "Bad material index for %s", modelKey);
        batch.initialized = false;
        return;
    }

    batch.mesh = model.meshes[meshIndex];

    // This copies the material struct.
    // NOTE: Material.maps is a pointer, so this still shares the model's maps.
    // That is the same behavior your current code has.
    batch.material = model.materials[matIndex];

    // Override shader after copying the real model material/texture.
    batch.material.shader = batch.shader;

    // Runtime lightmap/emission.
    batch.material.maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;

    batch.transforms.reserve(4096);

    batch.initialized = true;
}

static void PushVisibleTransformToCorrectBatch(const DungeonInstanceSource& src)
{
    switch (src.kind)
    {
        case DungeonInstanceKind::FloorGray:
            gGrayFloorInstancing.transforms.push_back(src.transform);
            break;

        case DungeonInstanceKind::FloorWood:
            gWoodFloorInstancing.transforms.push_back(src.transform);
            break;
    }
}

static void DrawDungeonInstancingBatch(DungeonInstancingBatch& batch)
{
    if (!batch.initialized) return;
    if (batch.transforms.empty()) return;

    // Important if gDynamic gets recreated/reloaded between levels.
    batch.material.maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;

    SetDungeonInstancingShaderValues(batch);

    DrawMeshInstanced(
        batch.mesh,
        batch.material,
        batch.transforms.data(),
        (int)batch.transforms.size()
    );
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void ClearDungeonInstancingSources()
{
    gDungeonInstanceSources.clear();

    gGrayFloorInstancing.transforms.clear();
    gWoodFloorInstancing.transforms.clear();
}

void InitDungeonInstancing()
{
    InitDungeonInstancingBatch(
        gGrayFloorInstancing,
        "floorTileGray",
        "floorInstancedLightingShader"
    );

    InitDungeonInstancingBatch(
        gWoodFloorInstancing,
        "woodFloor",
        "floorInstancedLightingShader"
    );
}

void AddFloorInstanceSource(const FloorTile& tile)
{
    if (tile.floorType != FloorType::Normal) return;

    DungeonInstanceSource src;
    src.position = tile.position;
    src.transform = MakeFloorTransform(tile.position);

    src.kind = CurrentLevelIs("Ship")
        ? DungeonInstanceKind::FloorWood
        : DungeonInstanceKind::FloorGray;

    gDungeonInstanceSources.push_back(src);
}

void BuildVisibleDungeonInstanceTransforms(Camera& camera, float maxDrawDist)
{
    gGrayFloorInstancing.transforms.clear();
    gWoodFloorInstancing.transforms.clear();

    ViewConeParams vp = MakeViewConeParams(
        camera,
        55.0f,
        maxDrawDist,
        400.0f
    );

    for (const DungeonInstanceSource& src : gDungeonInstanceSources)
    {
        if (!IsInViewCone(vp, src.position))
            continue;

        PushVisibleTransformToCorrectBatch(src);
    }

    UpdateDungeonInstancingDebugCounts();
}

void DrawDungeonInstancedFloors()
{
    DrawDungeonInstancingBatch(gGrayFloorInstancing);
    DrawDungeonInstancingBatch(gWoodFloorInstancing);
}