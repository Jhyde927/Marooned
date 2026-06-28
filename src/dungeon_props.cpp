#include "dungeon_props.h"
#include "transparentDraw.h"
#include "resourceManager.h"
#include "world.h"
#include "dungeonColors.h"
#include "weapon.h"
#include "viewCone.h"
#include "game_settings.h"
#include "debug_console.h"
#include "pathfinding.h"
#include <cmath>

using namespace dungeonColors;

std::vector<DungeonProp> gDungeonProps;

static BillboardDrawRequest MakePropBillboardRequest(
    const DungeonProp& prop,
    const Vector3& cameraPos,
    float rotationY,
    BillboardType billboardType
)
{
    BillboardDrawRequest req{}; // important: zero initializes isPortal, isOpen, openAmount, etc.

    req.type = billboardType;
    req.position = prop.position;


    req.texture = ResourceManager::Get().GetTexture(prop.textureName);
    req.sourceRect = {
        0.0f,
        0.0f,
        (float)req.texture.width,
        (float)req.texture.height
    };

    req.size = prop.size;
    req.tint = prop.tint;
    req.distanceToCamera = DistSq(prop.position, cameraPos);
    req.rotationY = rotationY;
    req.flipX = false;

    // These are portal/door-specific, but zero-init should already handle them.
    req.isPortal = false;
    req.isOpen = false;
    req.openAmount = 0.0f;

    return req;
}

static void PushFixedFlatProp(
    const DungeonProp& prop,
    const Vector3& cameraPos,
    std::vector<BillboardDrawRequest>& requests,
    float rotationY
)
{
    BillboardDrawRequest req = MakePropBillboardRequest(
        prop,
        cameraPos,
        rotationY,
        Billboard_FixedFlat
    );

    requests.push_back(req); //push to main drawRequest pipe line. 
}

static void PushCrossQuadProp(
    const DungeonProp& prop,
    const Vector3& cameraPos,
    std::vector<BillboardDrawRequest>& requests
)
{
    // First sheet.
    PushFixedFlatProp(prop, cameraPos, requests, prop.rotationY);

    // Second sheet crossed through it.
    PushFixedFlatProp(prop, cameraPos, requests, prop.rotationY + 90.0f);
}

void GatherDungeonPropDrawRequests(
    const Vector3& cameraPos,
    std::vector<BillboardDrawRequest>& requests
)
{

    for (const DungeonProp& prop : gDungeonProps)
    {
        if (!prop.active) continue;

        switch (prop.renderMode)
        {
            case DungeonPropRenderMode::Billboard:
            {
                BillboardDrawRequest req = MakePropBillboardRequest(
                    prop,
                    cameraPos,
                    prop.rotationY,
                    Billboard_FacingCamera
                );
            
                requests.push_back(req);
            } break;

            case DungeonPropRenderMode::FixedFlat:
            case DungeonPropRenderMode::WallQuad:
            {
                PushFixedFlatProp(prop, cameraPos, requests, prop.rotationY);
            } break;

            case DungeonPropRenderMode::CrossQuads:
            {
                PushCrossQuadProp(prop, cameraPos, requests);
            } break;

            case DungeonPropRenderMode::FloorQuad:
            {
                // Later. Your current Billboard_FixedFlat probably assumes vertical quads.
                // Don't solve this yet.
            } break;

            case DungeonPropRenderMode::Model:
            {

                // Later. Models should probably be drawn in a separate DrawDungeonPropModels().
                //DrawDungeonPropModels(camera);
            } break;
        }
    }
}


void ClearDungeonProps()
{
    gDungeonProps.clear();
}

DungeonProp MakeDefaultProp(DungeonPropType type, Vector3 position, float rotationY)
{

    DungeonProp prop;
    prop.type = type;
    prop.position = position;
    prop.rotationY = rotationY;
    prop.active = true;
    prop.hasCollision = false;

    switch (type)
    {

        case DungeonPropType::TableSet:
        {
            prop.renderMode = DungeonPropRenderMode::Model;
            prop.modelName = "TableSet";
            prop.modelSize = {70.0f, 70.0f, 70.0f};
            prop.tint = WHITE;
        } break;

        case DungeonPropType::CratePile:
        {
            prop.renderMode = DungeonPropRenderMode::Model;
            prop.modelName = "cratePile";
            prop.modelSize = {50.0f, 50.0f, 50.0f};
            prop.tint = WHITE;
        } break;

        case DungeonPropType::Stool:
        {
            prop.renderMode = DungeonPropRenderMode::Model;
            prop.modelName = "stool";
            prop.modelSize = {150.0f, 150.0f, 150.0f};
            prop.tint = WHITE;
        } break;

        case DungeonPropType::BonePile:
        {
            prop.renderMode = DungeonPropRenderMode::Model;
            prop.modelName = "bonePile";
            prop.modelSize = {10.0f, 10.0f, 10.0f};
            prop.tint = WHITE;
        } break;

        case DungeonPropType::Candelabra:
        {
            prop.renderMode = DungeonPropRenderMode::Model;
            prop.modelName = "candelabra";
            prop.modelSize = {100.0f, 100.0f, 100.0f};
            prop.tint = WHITE;
        } break;


        case DungeonPropType::SpiderWebCorner:
        {
            prop.renderMode = DungeonPropRenderMode::CrossQuads;
            prop.textureName = "spiderWebTexture"; // whatever key you add to ResourceManager
            prop.size = { 90.0f, 90.0f };

            float darkness = CalculateDarknessFactor(prop.position, dungeonLights);
            Color tint = TintFromDarkness(darkness);

            prop.tint = tint;

        } break;

        case DungeonPropType::WallBanner:
        {
            prop.renderMode = DungeonPropRenderMode::WallQuad;
            prop.textureName = "wallBanner";
            prop.size = {128.0f, 256.0f};

            float darkness = CalculateDarknessFactor(prop.position, dungeonLights);
            Color tint = TintFromDarkness(darkness);

            prop.tint = tint;
        } break;

        default:
        {
            prop.renderMode = DungeonPropRenderMode::CrossQuads;
            prop.textureName = "spiderWebTexture";
            prop.size = { 180.0f, 180.0f };
            prop.tint = WHITE;
        } break;
    }

    return prop;
}


void GenerateDungeonPropsForCurrentLevel()
{
    //(5975, 220, 4887) //wall banner
    if (CurrentLevelIs("Dungeon1")){
        Vector3 spawnPos = {6060, floorHeight+215, 4905};
        DungeonProp prop = MakeDefaultProp(DungeonPropType::WallBanner, spawnPos, 90.0f);
        gDungeonProps.push_back(prop);

    }


}

static unsigned int gLastPropSeed = 0;

void GenerateProps(float baseY) {
    //debug command: Props = regenerate props. 
    gDungeonProps.clear();
    
    gLastPropSeed = (unsigned int)GetRandomValue(1, 999999999);

    //if (CurrentLevelIs("Dungeon1")) gLastPropSeed = 922767668; //good seed for dungeon1

    std::string seedString = std::to_string(gLastPropSeed);

    DebugConsole::Log("Prop Seed: " + seedString);
    std::cout << "Prop seed: " << gLastPropSeed << "\n";
    
    SetRandomSeed(gLastPropSeed);

    if (CurrentLevelIs("Dungeon1")) GenerateDungeonPropsForCurrentLevel(); //hardcoded props. 

    GenerateAutoCornerProps(baseY);

    //generate tables. 
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::tableSet))) { 
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);
                DungeonProp prop = MakeDefaultProp(DungeonPropType::TableSet, pos, 0.0f);
                gDungeonProps.push_back(prop);
            }
        }
    }

    // Re-randomize the global RNG so gameplay randomness is not locked
    // to the prop seed forever.
    SetRandomSeed((unsigned int)GetTime() * 1000000u);
}



static int GetAutoCornerSpawnChance(DungeonPropType type)
{
    switch (type)
    {
        case DungeonPropType::SpiderWebCorner:
            return 75;

        case DungeonPropType::BonePile:
            return 90;

        case DungeonPropType::CratePile:
            return 100;

        case DungeonPropType::Stool:
            return 50;

        case DungeonPropType::Candelabra:
            return 100;

        default:
            return 0;
    }
}

static float GetAutoCornerInsetMultiplier(DungeonPropType type)
{
    switch (type)
    {
        case DungeonPropType::SpiderWebCorner:
            return 1.0f;   // pushed deep into corner

        case DungeonPropType::Stool:
            return 0.65f;  // closer to wall than crates, but not clipping

        case DungeonPropType::BonePile:
            return 0.45f;  // floor clutter, can sit more toward center

        case DungeonPropType::CratePile:
            return 0.50f;  // your current offsetX / 2 behavior

        default:
            return 1.0f;
    }
}


static DungeonProp MakeAutoCornerProp(
    DungeonPropType type,
    Vector3 basePos,
    float baseY,
    float rotationY)
{
    DungeonProp prop = MakeDefaultProp(type, basePos, rotationY);

    prop.position.y = baseY;
    prop.ambientBoost = 0.2f;

    switch (type)
    {
        case DungeonPropType::SpiderWebCorner:
            prop.position.y = baseY + 40.0f;
            prop.rotationY = rotationY;
            prop.ambientBoost = 0.0f;
            break;

        case DungeonPropType::BonePile:
            prop.ambientBoost = 0.05f;
            prop.rotationY = RandomFloat(0.0f, 359.0f);
            break;

        case DungeonPropType::CratePile:
            prop.ambientBoost = 0.1f;
            break;

        case DungeonPropType::TableSet:
            prop.ambientBoost = 0.0f;

            prop.rotationY = GetRandomValue(0,1) ? 0.0f : 90.0f;
            break;

        case DungeonPropType::WallBanner:
            prop.ambientBoost = 0.0f;
            break;

        case DungeonPropType::Candelabra:
        case DungeonPropType::Stool:
        default:
            break;
    }

    return prop;
}


static DungeonPropType PickAutoCornerPropType()
{
    // Special case: spider boss / web-heavy level.

    int roll = GetRandomValue(0, 4);

    if (roll == 0){
        return DungeonPropType::SpiderWebCorner;
    }else if (roll == 1){
        return DungeonPropType::CratePile;
    }else if (roll == 2){
        return DungeonPropType::Stool;
    }else if (roll == 3){
        return DungeonPropType::BonePile;
    }else if (roll == 4){
        return DungeonPropType::Candelabra;
    }

    return DungeonPropType::None;
}

void GenerateAutoCornerProps(float baseY)
{
    if (!isDungeon) return;

    //const int spawnChancePercent = 75.0f;
    const float cornerInset = tileSize * 0.6f;

    auto IsSolidWall = [&](Color c)
    {
        if (c.a == 0) return false;
        if (!IsWallColor(c)) return false;
        if (IsBarrelColor(c)) return false;
        return true;
    };

    auto IsOpenForCornerProp = [&](Color c)
    {
        if (c.a == 0) return false;

        return EqualsRGB(c, WHITE); //only corners that are empty. 
    };

    auto GetPixelSafe = [&](int x, int y) -> Color
    {
        if (x < 0 || y < 0 || x >= dungeonWidth || y >= dungeonHeight)
            return { 0, 0, 0, 0 };

        return dungeonPixels[y * dungeonWidth + x];
    };

    auto TrySpawnCornerProp = [&](int x, int y, float offsetX, float offsetZ, float rotationY)
    {

        DungeonPropType type = PickAutoCornerPropType();
        if (type == DungeonPropType::None) return;

        int spawnChancePercent = GetAutoCornerSpawnChance(type);

        if (GetRandomValue(1, 100) > spawnChancePercent) return;
        

        if (!IsWalkable(x, y, dungeonImg)) return;

        Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

        float insetMult = GetAutoCornerInsetMultiplier(type);

        pos.x += offsetX * insetMult;
        pos.z += offsetZ * insetMult;

        DungeonProp prop = MakeAutoCornerProp(type, pos, baseY, rotationY);

        gDungeonProps.push_back(prop);
    };

    for (int y = 0; y < dungeonHeight; y++)
    {
        for (int x = 0; x < dungeonWidth; x++)
        {
            Color center = GetPixelSafe(x, y);
            if (!IsOpenForCornerProp(center)) continue;

            bool north = IsSolidWall(GetPixelSafe(x,     y - 1));
            bool south = IsSolidWall(GetPixelSafe(x,     y + 1));
            bool west  = IsSolidWall(GetPixelSafe(x - 1, y));
            bool east  = IsSolidWall(GetPixelSafe(x + 1, y));

            bool nw = IsSolidWall(GetPixelSafe(x - 1, y - 1));
            bool ne = IsSolidWall(GetPixelSafe(x + 1, y - 1));
            bool sw = IsSolidWall(GetPixelSafe(x - 1, y + 1));
            bool se = IsSolidWall(GetPixelSafe(x + 1, y + 1));

            if (north && west && nw)
            {
                TrySpawnCornerProp(x, y,  cornerInset,  cornerInset,  45.0f);
            }

            if (north && east && ne)
            {
                TrySpawnCornerProp(x, y, -cornerInset,  cornerInset, -45.0f);
            }

            if (south && west && sw)
            {
                TrySpawnCornerProp(x, y,  cornerInset, -cornerInset, -45.0f);
            }

            if (south && east && se)
            {
                TrySpawnCornerProp(x, y, -cornerInset, -cornerInset,  45.0f);
            }
        }
    }
}

float GetPropBrightness(DungeonProp& prop){
    //default is 0.20
    switch (prop.type)
    {
    case DungeonPropType::CratePile:
        return 0.10f;
    case DungeonPropType::Candelabra:
        return 0.3f;

    case DungeonPropType::BonePile:
        return 0.1f;

    case DungeonPropType::Stool:
        return 0.2f;

    case DungeonPropType::TableSet:
        return 0.1f;
    default:
        return 0.2f;
        break;
    }
}

void  DrawDungeonPropModels(Camera& camera){
    Shader& sh = R.GetShader("lightingShader");

    ViewConeParams vp = MakeViewConeParams( 
        camera,
        55.0f,
        GameSettings::maxDrawDist,
        400.0f
    );

    int propAmbientLoc = GetShaderLocation(sh, "propAmbientBoost");
    int maxBrightnessLoc = GetShaderLocation(sh, "maxBrightness");


    for (DungeonProp& prop : gDungeonProps){
        if (prop.renderMode != DungeonPropRenderMode::Model) continue;
  
        if (!IsInViewCone(vp, prop.position)) continue;//frustum cull

        

        SetShaderValue(sh, propAmbientLoc, &prop.ambientBoost, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, maxBrightnessLoc, &prop.maxBrightness, SHADER_UNIFORM_FLOAT);

        DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);

    }

    float resetPropAmbientBoost = 0.0f;
    float resetMaxBrightness = 1.0f;

    SetShaderValue(sh, propAmbientLoc, &resetPropAmbientBoost, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, maxBrightnessLoc, &resetMaxBrightness, SHADER_UNIFORM_FLOAT);
}

