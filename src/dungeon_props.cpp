#include "dungeon_props.h"
#include "transparentDraw.h"
#include "resourceManager.h"
#include "world.h"
#include "dungeonColors.h"
#include "weapon.h"
#include "viewCone.h"
#include "game_settings.h"
#include "debug_console.h"
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
        Vector3 spawnPos = {6060, floorHeight+200, 5000};
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

void  DrawDungeonPropModels(Camera& camera){

    ViewConeParams vp = MakeViewConeParams( 
        camera,
        55.0f,
        GameSettings::maxDrawDist,
        400.0f
    );


    for (DungeonProp& prop : gDungeonProps){

        if (!IsInViewCone(vp, prop.position)) continue;//frustum cull

        //DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);
        switch (prop.type)
        {
        case DungeonPropType::TableSet:
            DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);
            break;

        case DungeonPropType::CratePile:
            DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);
            break;
        case DungeonPropType::Stool:
            DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);
            break;

        case DungeonPropType::BonePile:
            DrawModelEx(R.GetModel(prop.modelName), prop.position, Vector3{0, 1, 0}, prop.rotationY, prop.modelSize, WHITE);
            break;

        default:
            break;
        }

    }
}
