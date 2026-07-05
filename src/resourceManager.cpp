
#include "resourceManager.h"
#include "world.h"
#include "lighting.h"
#include "level.h"
#include "vegetation.h"
#include "terrainChunking.h"
#include "rlgl.h"
#include "utilities.h"
#include "camera_system.h"
#include "shaderSetup.h"
#include "game_settings.h"




ResourceManager* ResourceManager::_instance = nullptr;

void ResourceManager::ensureFallback_() const {
    if (_fallbackReady) return;
    Image img = GenImageChecked(64, 64, 8, 8, MAGENTA, BLACK);
    _fallbackTex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (_fallbackTex.id == 0) {
        Image tiny = GenImageColor(1, 1, WHITE);
        _fallbackTex = LoadTextureFromImage(tiny);
        UnloadImage(tiny);
    }
    _fallbackReady = (_fallbackTex.id != 0);
}



ResourceManager& ResourceManager::Get() {
    if (!_instance) _instance = new ResourceManager();
    return *_instance;
}

// Texture
Texture2D& ResourceManager::LoadTexture(const std::string& name, const std::string& path) {
    auto it = _textures.find(name);
    if (it != _textures.end()) return it->second;

    if (path.empty()) {
        TraceLog(LOG_ERROR, "❌ LoadTexture failed: empty path for texture '%s'", name.c_str());
        exit(1);
    }

    Texture2D tex = ::LoadTexture(path.c_str());

    if (tex.id == 0) {
        TraceLog(LOG_ERROR, "❌ LoadTexture failed for '%s' at path '%s'", name.c_str(), path.c_str());
        exit(1);
    }

    _textures.emplace(name, tex);
    return _textures[name];
}


Texture2D& ResourceManager::GetTexture(const std::string& name) {
    auto it = _textures.find(name);
    if (it != _textures.end()) return it->second;

    ensureFallback_();
    TraceLog(LOG_ERROR, "Missing texture: %s (using fallback)", name.c_str());
    return _fallbackTex; // safe: owned member, not a temporary
}

// Model
Model& ResourceManager::LoadModel(const std::string& name, const std::string& path) {
    auto it = _models.find(name);
    if (it != _models.end()) return it->second;
    Model m = ::LoadModel(path.c_str());
    _models.emplace(name, m);
    return _models[name];
}

Model& ResourceManager::AddModelFromMesh(const std::string& name, Mesh mesh)   // note: Mesh by value is fine
{
    auto it = _models.find(name);
    if (it != _models.end()) return it->second;

    // Construct directly in the map (no intermediate copy)
    auto [insertIt, inserted] = _models.emplace(name, ::LoadModelFromMesh(mesh));
    return insertIt->second;
}

Model& ResourceManager::GetModel(const std::string& name) {
    auto it = _models.find(name);
    if (it == _models.end()) throw std::runtime_error("Model not found: " + name);
    return it->second;
}

const Model& ResourceManager::GetModel(const std::string& name) const {
    auto it = _models.find(name);
    if (it == _models.end()) throw std::runtime_error("Model not found: " + name);
    return it->second;
}

// Shader
Shader& ResourceManager::LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath) {
    auto it = _shaders.find(name);
    if (it != _shaders.end()) return it->second;
    Shader s = ::LoadShader(vsPath.c_str(), fsPath.c_str());
    _shaders.emplace(name, s);
    return _shaders[name];
}
Shader& ResourceManager::GetShader(const std::string& name) const {
    auto it = _shaders.find(name);
    if (it == _shaders.end()) throw std::runtime_error("Shader not found: " + name);
    return const_cast<Shader&>(it->second);
}



// RenderTexture
RenderTexture2D& ResourceManager::LoadRenderTexture(const std::string& name, int w, int h)
{
    auto it = _renderTextures.find(name);
    if (it != _renderTextures.end())
    {
        // If you want: validate size here, or just return existing.
        return *it->second;
    }

    RenderTexture2D rt = ::LoadRenderTexture(w, h);
    auto ptr = std::make_unique<RenderTexture2D>(rt);

    auto [insertIt, ok] = _renderTextures.emplace(name, std::move(ptr));
    return *insertIt->second;
}

RenderTexture2D& ResourceManager::GetRenderTexture(const std::string& name)
{
    auto it = _renderTextures.find(name);
    if (it == _renderTextures.end())
        throw std::runtime_error("RenderTexture not found: " + name);

    return *it->second;
}

    const RenderTexture2D& ResourceManager::GetRenderTexture(const std::string& name) const
    {
        auto it = _renderTextures.find(name);
        if (it == _renderTextures.end())
            throw std::runtime_error("RenderTexture not found: " + name);

        return *it->second;
    }


Font& ResourceManager::LoadFont(const std::string& name,const std::string& path, int baseSize, int filter)
{
    auto it = _fonts.find(name);
    if (it != _fonts.end()) return it->second;

    // Load with control over bake size (crisp at large sizes)
    Font f = LoadFontEx(path.c_str(), baseSize, nullptr, 0);

    // Optional safety: fall back to default font if something failed
    if (f.texture.id == 0) {
        TraceLog(LOG_WARNING, "LoadFontEx failed for %s, using default font.", path.c_str());
        f = GetFontDefault();
    } else {
        // Choose smoothing/pixel look
        SetTextureFilter(f.texture, filter);
    }

    _fonts.emplace(name, f);
    return _fonts[name];
}

Font& ResourceManager::GetFont(const std::string& name)
{
    auto it = _fonts.find(name);
    if (it == _fonts.end()) {
        TraceLog(LOG_WARNING, "GetFont: '%s' not found, returning default font.", name.c_str());
        static Font fallback = GetFontDefault(); // static to keep ref valid
        return fallback;
    }
    return it->second;
}

void ResourceManager::UnloadAllFonts()
{
    for (auto& kv : _fonts) {
        // Don't unload default font (raylib manages it)
        if (kv.second.texture.id != GetFontDefault().texture.id) {
            UnloadFont(kv.second);
        }
    }
    _fonts.clear();
}

// ResourceManager.cpp (example)
//static int gLastW = -1, gLastH = -1;




void ResourceManager::LoadAllResources() {
    Vector2 screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    // ResourceManager init once

    //render textures
    RenderTexture& scene = R.LoadRenderTexture("sceneTexture", screenResolution.x, screenResolution.y);
    SetTextureFilter(scene.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(scene.texture, TEXTURE_WRAP_CLAMP);

    // RenderTexture& post = R.LoadRenderTexture("postProcessTexture", screenResolution.x, screenResolution.y);
    // SetTextureFilter(post.texture, TEXTURE_FILTER_BILINEAR);
    // SetTextureWrap(post.texture, TEXTURE_WRAP_CLAMP);

    SetTextureWrap(R.GetRenderTexture("sceneTexture").texture, TEXTURE_WRAP_CLAMP);
    //SetTextureWrap(R.GetRenderTexture("postProcessTexture").texture, TEXTURE_WRAP_CLAMP);

    R.LoadFont("Pieces", "assets/fonts/PiecesOfEight.ttf", 128, 1);
    R.LoadFont("Kingthings", "assets/fonts/KingthingsPetrock.ttf", 128, 1);
    R.LoadFont("jetBrains", "assets/fonts/JetBrainsMono-Bold.ttf");
    R.LoadFont("terminal", "assets/fonts/VT323-Regular.ttf");

    //Resources are saved to unordered maps, with a string key. Get a resource by calling R.GetModel("blunderbuss") for example. 
    R.LoadTexture("raptorTexture",      "assets/sprites/bigRaptorSheet.png");
    R.LoadTexture("skeletonSheet",      "assets/sprites/skeletonSheet.png");
    R.LoadTexture("muzzleFlash",        "assets/sprites/muzzleFlash.png");
    R.LoadTexture("backDrop",           "assets/screenshots/dungeon1.png");
    R.LoadTexture("smokeSheet",         "assets/sprites/smokeSheet.png");
    R.LoadTexture("bloodSheet",         "assets/sprites/bloodDecalSheet.png");
    R.LoadTexture("doorTexture",        "assets/sprites/Door.png");
    R.LoadTexture("healthPotTexture",   "assets/sprites/Healthpot.png");
    R.LoadTexture("keyTexture",         "assets/sprites/key.png");
    R.LoadTexture("swordBloody",        "assets/textures/swordBloody.png");
    R.LoadTexture("swordClean",         "assets/textures/swordClean.png");
    R.LoadTexture("fireSheet",          "assets/sprites/fireSheet.png");
    R.LoadTexture("pirateSheet",        "assets/sprites/pirateSheet.png");
    R.LoadTexture("coinTexture",        "assets/sprites/coin.png");
    R.LoadTexture("spiderSheet",        "assets/sprites/spiderSheet.png");
    R.LoadTexture("spiderWebTexture",   "assets/sprites/spiderWeb.png");
    R.LoadTexture("brokeWebTexture",    "assets/sprites/brokeWeb.png");
    R.LoadTexture("explosionSheet",     "assets/sprites/explosionSheet.png");
    R.LoadTexture("manaPotion",         "assets/sprites/manaPotion.png");
    R.LoadTexture("fireIcon",           "assets/sprites/fireIcon.png");
    R.LoadTexture("iceIcon",            "assets/sprites/iceIcon.png");
    R.LoadTexture("shadowTex",          "assets/textures/shadow_decal.png");
    R.LoadTexture("ghostSheet",         "assets/sprites/ghostSheet.png");
    R.LoadTexture("magicAttackSheet",   "assets/sprites/magicAttackSheet.png");
    R.LoadTexture("treeShadow",         "assets/textures/treeShadow.png");
    R.LoadTexture("grassTexture",       "assets/textures/grass2.png");
    R.LoadTexture("sandTexture",        "assets/textures/sand.png");
    R.LoadTexture("trexSheet",          "assets/sprites/trexSheet.png");
    R.LoadTexture("blockSheet",         "assets/sprites/blockSheet2.png");
    R.LoadTexture("playerSlashSheet",   "assets/sprites/playerSlashSheet.png");
    R.LoadTexture("slashSheet",         "assets/sprites/slashSheet.png");
    R.LoadTexture("slashSheetLeft",     "assets/sprites/slashSheetLeft.png");
    R.LoadTexture("biteSheet",          "assets/sprites/biteSheet.png");
    R.LoadTexture("bulletHoleSheet",    "assets/sprites/bulletHoleSheet.png");
    R.LoadTexture("GiantSpiderSheet",   "assets/sprites/giantSpiderSheet.png");
    R.LoadTexture("spiderEggSheet",     "assets/sprites/spiderEggSheet.png");
    R.LoadTexture("blank",              "assets/textures/blank.png");
    R.LoadTexture("silverKey",          "assets/sprites/silverKey.png");
    R.LoadTexture("harpoon",            "assets/sprites/harpoon.png");
    R.LoadTexture("grapplePoint",       "assets/sprites/grapplePoint.png");
    R.LoadTexture("swordIcon",          "assets/sprites/cutlassIcon.png");
    R.LoadTexture("crossbowIcon",       "assets/sprites/crossbowIcon2.png");
    R.LoadTexture("blunderbussIcon",    "assets/sprites/blunderbussIcon2.png");
    R.LoadTexture("staffIcon",          "assets/sprites/staffIcon2.png");
    R.LoadTexture("shotgunReticle",     "assets/sprites/shotgunReticle.png");
    R.LoadTexture("dactylSheet",        "assets/sprites/dactylSheet.png");
    R.LoadTexture("ceilingTexture",     "assets/textures/ceilingTilesTexture.png");
    R.LoadTexture("wizardSheet",        "assets/sprites/wizardSheet.png");
    R.LoadTexture("skeletonKey",        "assets/sprites/skeletonKey.png");
    R.LoadTexture("batSheet",           "assets/sprites/batSheet.png");
    R.LoadTexture("hermitSheet",        "assets/sprites/hermitSheet.png");
    R.LoadTexture("whiteGradient",      "assets/textures/whiteGradient.png");
    R.LoadTexture("zombieSheet",        "assets/sprites/zombieSheet.png");
    R.LoadTexture("zombieSheetArmless", "assets/sprites/zombieSheetArmless.png");
    R.LoadTexture("zombieSheetHeadless","assets/sprites/zombieSheetHeadless.png");
    R.LoadTexture("headSpin",           "assets/sprites/headSpin.png");
    R.LoadTexture("armSpin",            "assets/sprites/armSpin.png");
    R.LoadTexture("zombieGib",          "assets/sprites/zombieGib.png");
    R.LoadTexture("boneSpin",           "assets/sprites/boneSpin.png");
    R.LoadTexture("quadDamage",         "assets/sprites/quadDamage.png");
    R.LoadTexture("haste",              "assets/sprites/haste.png");
    R.LoadTexture("overHealth",         "assets/sprites/overHealth.png");
    R.LoadTexture("grassCard",          "assets/textures/grassCard1.png");
    R.LoadTexture("grassCard2",          "assets/textures/grassCard2.png");
    R.LoadTexture("grassCard3",          "assets/textures/grassCard3.png");
    R.LoadTexture("grassCard4",          "assets/textures/grassCard4.png");
    R.LoadTexture("cratePile",           "assets/textures/cratePile.png");
    R.LoadTexture("wallBanner",          "assets/sprites/wallBanner.png");

    R.LoadTexture("raftMast", "assets/sprites/raftMast.png");
    R.LoadTexture("raftBody", "assets/sprites/raftBody.png");
    R.LoadTexture("raftSail", "assets/sprites/raftSail.png");

    // Models (registering with string keys)
    R.LoadModel("palmTree",               "assets/Models/bigPalmTree.glb");

    R.LoadModel("palmTreeInstanced", "assets/Models/bigPalmTree.glb");
     R.LoadModel("bushInstanced",    "assets/Models/grass(stripped).glb");

    R.LoadModel("palm2",                  "assets/Models/smallPalmTree.glb");
    R.LoadModel("bush",                   "assets/Models/grass(stripped).glb");
    R.LoadModel("boatModel",              "assets/Models/boat.glb");
    R.LoadModel("blunderbuss",            "assets/Models/blunderbus.glb");
    R.LoadModel("floorTileGray",          "assets/Models/floorTileGray.glb");
    R.LoadModel("doorWayGray",            "assets/Models/doorWayGray.glb");
    R.LoadModel("wallSegment",            "assets/Models/wallSegment.glb");
    R.LoadModel("barrelModel",            "assets/Models/barrel.glb");
    R.LoadModel("swordModel",             "assets/Models/sword2.glb");
    R.LoadModel("lampModel",              "assets/Models/lamp.glb");
    R.LoadModel("brokeBarrel",            "assets/Models/brokeBarrel.glb");
    R.LoadModel("chestModel",             "assets/Models/chest.glb");
    R.LoadModel("staffModel",             "assets/Models/staff.glb");
    R.LoadModel("fireballModel",          "assets/Models/fireball.glb");
    R.LoadModel("iceballModel",           "assets/Models/iceBall.glb");
    R.LoadModel("campFire",               "assets/Models/campFire.glb");
    R.LoadModel("stonePillar",            "assets/Models/stonePillar.glb");
    R.LoadModel("lavaTile",               "assets/Models/lavaTileSquare.glb");
    R.LoadModel("crossbow",               "assets/Models/crossbow.glb");
    R.LoadModel("crossbowRest",           "assets/Models/crossbowRest.glb");
    R.LoadModel("bolt",                   "assets/Models/bolt.glb");
    R.LoadModel("windowedWall",           "assets/Models/windowedWall.glb");
    R.LoadModel("windowWay",              "assets/Models/windowHoleSquare.glb");
    R.LoadModel("box",                    "assets/Models/box.glb");
    R.LoadModel("healthPotion",           "assets/Models/healthPotion.glb");
    R.LoadModel("raft",                   "assets/Models/raft.glb");
    R.LoadModel("raftBody",               "assets/Models/raftBody.glb");
    R.LoadModel("raftMast",               "assets/Models/raftMast.glb");
    R.LoadModel("raftBoom",               "assets/Models/raftBoom.glb");
    R.LoadModel("raftSail",               "assets/Models/raftSail.glb");

    R.LoadModel("collectableMast",        "assets/Models/collectableMast.glb");
    R.LoadModel("collectableBoom",        "assets/Models/collectableBoom.glb");
    R.LoadModel("collectableSail",        "assets/Models/collectableSail.glb");

    R.LoadModel("woodWall",               "assets/Models/woodWall.glb");
    R.LoadModel("woodDoorWay",            "assets/Models/woodDoorWay.glb");
    R.LoadModel("woodWallHalf",           "assets/Models/woodWallHalf.glb");
    R.LoadModel("woodFloor",              "assets/Models/floorTileWood.glb");
    R.LoadModel("shipMast",               "assets/Models/cartoonMast.glb");
    R.LoadModel("squidHead",              "assets/Models/squidHead2.glb");
    R.LoadModel("cannon",                 "assets/Models/cannon.glb");
    R.LoadModel("cannonBall",             "assets/Models/cannonBall.glb");
    R.LoadModel("cannonBalls",            "assets/Models/cannonBalls.glb");

    R.LoadModel("grassCardInstanced",     "assets/Models/grassCard1.glb");
    R.LoadModel("grassCardInstanced2",    "assets/Models/grassCard2.glb");
    R.LoadModel("grassCardInstanced3",    "assets/Models/grassCard3.glb");
    R.LoadModel("grassCardInstanced4",    "assets/Models/grassCard4.glb");

    R.LoadModel("TableSet",               "assets/Models/TableSet.glb");
    R.LoadModel("cratePile",              "assets/Models/cratePile.glb");
    R.LoadModel("stool",                  "assets/Models/stool.glb");
    R.LoadModel("bonePile",               "assets/Models/bonePile.glb");
    R.LoadModel("candelabra",             "assets/Models/candelabra.glb");

    //generated models

    R.AddModelFromMesh("squareBolt", GenMeshCube(2.0f, 2.0f, 20.0f));
    R.AddModelFromMesh("skyModel", GenMeshCube(1.0f, 1.0f, 1.0f));
    // R.AddModelFromMesh("waterModel",GenMeshPlane(50000, 50000, 1, 1));
    R.AddModelFromMesh("waterModel", GenMeshCylinder(16000.0f, 1.0f, 128));
    R.AddModelFromMesh("ceilingPlane", GenMeshPlane(1.0f, 1.0f, 1, 1)); //we can scale it later. 
    R.AddModelFromMesh("shadowQuad",GenMeshPlane(1.0f, 1.0f, 1, 1)); //still used for enemy shadows
    
    //shaders
    R.LoadShader("terrainShader",  "assets/shaders/height_color.vs",       "assets/shaders/height_color.fs");
    R.LoadShader("shadowShader",   "assets/shaders/shadow_decal.vs",       "assets/shaders/shadow_decal.fs");
    R.LoadShader("skyShader",      "assets/shaders/skybox.vs",             "assets/shaders/skybox.fs");
    R.LoadShader("waterShader",    "assets/shaders/water.vs",              "assets/shaders/water.fs");
    R.LoadShader("bloomShader",    /*vsPath=*/"",                          "assets/shaders/bloom.fs");
    R.LoadShader("cutoutShader",   /*vsPath=*/"",                          "assets/shaders/leaf_cutout.fs");
    R.LoadShader("lightingShader", "assets/shaders/lighting_baked_xz.vs",  "assets/shaders/lighting_baked_xz.fs");
    R.LoadShader("lavaShader",     "assets/shaders/lava_world.vs",         "assets/shaders/lava_world.fs");
    R.LoadShader("treeShader",     "assets/shaders/treeShader.vs",         "assets/shaders/treeShader.fs");
    R.LoadShader("portalShader",   "assets/shaders/portal.vs",             "assets/shaders/portal.fs");
    R.LoadShader("ceilingShader",  "assets/shaders/ceiling.vs",            "assets/shaders/ceiling.fs");
    R.LoadShader("ghostShader",    "assets/shaders/ghost_raft.vs",         "assets/shaders/ghost_raft.fs");
    R.LoadShader("floorInstancedLightingShader", "assets/shaders/floor_instanced_lighting.vs", "assets/shaders/floor_instanced_lighting.fs");
    R.LoadShader("tree_instanced", "assets/shaders/tree_instanced.vs",     "assets/shaders/tree_instanced.fs");
    R.LoadShader("weapon_outline", "assets/shaders/weapon_outline.vs",     "assets/shaders/weapon_outline.fs");

}



void ResourceManager::SetShaderValues(){

    ShaderSetup::InitGhostShader(ShaderSetup::gGhost);
    ShaderSetup::InitAlphaCutout(ShaderSetup::gAlpha);
    ShaderSetup::InitShadowShader(ShaderSetup::gShadow);
    ShaderSetup::InitWeaponOutlineFx();

}

void ResourceManager::SetTerrainShaderValues(){ //plus palm tree shader
    // Load textures (tileable, power-of-two helps mips)
    Shader& terrainShader = R.GetShader("terrainShader");

    Shader& sh = R.GetShader("terrainShader");
    //
    Vector3 oceanColor = MakeTerrainWaterColor(ShaderSetup::GetCurrentSkyTopFogColor());//{0.25, 0.73, 1.0};

    int waterColorLoc = GetShaderLocation(sh, "u_waterColor");
    SetShaderValue(sh, waterColorLoc, &oceanColor, SHADER_UNIFORM_VEC3);

    sh.locs[SHADER_LOC_MAP_ALBEDO]    = GetShaderLocation(sh, "texGrass");
    sh.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(sh, "texSand");
    sh.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(sh, "textureOcclusion");

    Texture2D grass = R.GetTexture("grassTexture");
    Texture2D sand  = R.GetTexture("sandTexture");

    GenTextureMipmaps(&grass);
    GenTextureMipmaps(&sand);

    SetTextureFilter(grass, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(sand,  TEXTURE_FILTER_TRILINEAR);

    SetTextureWrap(grass, TEXTURE_WRAP_REPEAT);
    SetTextureWrap(sand,  TEXTURE_WRAP_REPEAT);

    for (auto& c : terrain.chunks) {
        if (c.model.materialCount == 0) continue;
        Material& m = c.model.materials[0];
        m.shader = sh;
        SetMaterialTexture(&m, MATERIAL_MAP_ALBEDO,    grass);
        SetMaterialTexture(&m, MATERIAL_MAP_METALNESS, sand);
        SetMaterialTexture(&m, MATERIAL_MAP_OCCLUSION, gTreeShadowMask.rt.texture);
    }

    // world extents (whole island)
    Vector2 minXZ = { -terrainScale.x*0.5f, -terrainScale.z*0.5f };
    Vector2 sizeXZ= {  terrainScale.x,        terrainScale.z      };
    SetShaderValue(sh, GetShaderLocation(sh,"u_WorldMinXZ"),  &minXZ,  SHADER_UNIFORM_VEC2);
    SetShaderValue(sh, GetShaderLocation(sh,"u_WorldSizeXZ"), &sizeXZ, SHADER_UNIFORM_VEC2);

    // tiling (start simple)
    float grassTiles=60.0f, sandTiles=20.0f;
    SetShaderValue(sh, GetShaderLocation(sh,"grassTiling"), &grassTiles, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, GetShaderLocation(sh,"sandTiling"),  &sandTiles,  SHADER_UNIFORM_FLOAT);
    
    // sampler → unit
    int u2=2;

    int locOcc   = GetShaderLocation(terrainShader, "textureOcclusion");

    SetShaderValue(terrainShader, locOcc,   &u2, SHADER_UNIFORM_INT);


    SetShaderValueTexture(terrainShader, locOcc,   gTreeShadowMask.rt.texture); // unit 2

    // --- World bounds and tiling
    int locWorldMinXZ  = GetShaderLocation(terrainShader, "u_WorldMinXZ");
    int locWorldSizeXZ = GetShaderLocation(terrainShader, "u_WorldSizeXZ");
    // int locGrassTile   = GetShaderLocation(terrainShader, "grassTiling");
    // int locSandTile    = GetShaderLocation(terrainShader, "sandTiling");

    Vector2 t_worldMinXZ  = { -terrainScale.x * 0.5f, -terrainScale.z * 0.5f };
    Vector2 t_worldSizeXZ = {  terrainScale.x,          terrainScale.z       };
    SetShaderValue(terrainShader, locWorldMinXZ,  &t_worldMinXZ,  SHADER_UNIFORM_VEC2);
    SetShaderValue(terrainShader, locWorldSizeXZ, &t_worldSizeXZ, SHADER_UNIFORM_VEC2);


    // --- Fog and sky
    Vector3 skyTop  = ShaderSetup::GetCurrentSkyTopFogColor();
    Vector3 skyHorz = ShaderSetup::GetCurrentSkyFogColor();//{0.60f, 0.80f, 0.95f};
    float fogStart  = 1000.0f; //fog start set every frame in update shaders
    float fogEnd    = GameSettings::terrainFogEnd;
    float seaLevel  = 400.0f;
    float falloff   = 0.002f;

    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SkyColorTop"),      &skyTop,   SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SkyColorHorizon"),  &skyHorz,  SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogStart"),         &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogEnd"),           &fogEnd,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SeaLevel"),         &seaLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogHeightFalloff"), &falloff,  SHADER_UNIFORM_FLOAT);

}




void ResourceManager::SetLightingShaderValues()
{
    Shader& lightingShader = R.GetShader("lightingShader");
    ShaderSetup::InitCeilingShader(ShaderSetup::gCeiling);

    // Tell raylib about emission sampler location (you use texture4)
    lightingShader.locs[SHADER_LOC_MAP_EMISSION] =
        GetShaderLocation(lightingShader, "texture4");

    // Assign lighting shader to all dungeon models (except ceiling)
    // Model& floorModel    = R.GetModel("floorTileGray");
    // Model& wallModel     = R.GetModel("wallSegment");
    Model& windowModel   = R.GetModel("windowWay");
    Model& doorwayModel  = R.GetModel("doorWayGray");
    Model& launcherModel = R.GetModel("stonePillar");
    Model& barrelModel   = R.GetModel("barrelModel");
    Model& brokeModel    = R.GetModel("brokeBarrel");
    Model& boxModel      = R.GetModel("box");
    Model& woodFloor     = R.GetModel("woodFloor");
    Model& woodWall      = R.GetModel("woodWall");
    Model& woodDoorWay   = R.GetModel("woodDoorWay");
    Model& woodWallHalf  = R.GetModel("woodWallHalf");
    Model& tableSet      = R.GetModel("TableSet");
    Model& cratePile     = R.GetModel("cratePile");
    Model& stool         = R.GetModel("stool");
    Model& bonePile      = R.GetModel("bonePile");
    Model& candelabra    = R.GetModel("candelabra");
    // Model& blunderbuss   = R.GetModel("blunderbuss");
    // Model& crossbow      = R.GetModel("crossbow");
    // Model& crossbowRest  = R.GetModel("crossbowRest");
    // Model& sword         = R.GetModel("swordModel");
    // Model& staff         = R.GetModel("staffModel");


    //apply texture to cratePile. Find a better place for this. 
    Texture2D crateTex = R.GetTexture("cratePile");
    for (int i = 0; i < cratePile.materialCount; i++)
    {
        cratePile.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = crateTex;
    }

    

    //for (int i = 0; i < wallModel.materialCount;    ++i) wallModel.materials[i].shader    = lightingShader;
    for (int i = 0; i < windowModel.materialCount;  ++i) windowModel.materials[i].shader  = lightingShader;
    for (int i = 0; i < doorwayModel.materialCount; ++i) doorwayModel.materials[i].shader = lightingShader;
    for (int i = 0; i < launcherModel.materialCount;++i) launcherModel.materials[i].shader= lightingShader;
    for (int i = 0; i < barrelModel.materialCount;  ++i) barrelModel.materials[i].shader  = lightingShader;
    for (int i = 0; i < brokeModel.materialCount;   ++i) brokeModel.materials[i].shader   = lightingShader;
    for (int i = 0; i < boxModel.materialCount;   ++i) boxModel.materials[i].shader   = lightingShader;
    for (int i = 0; i < woodFloor.materialCount;   ++i) woodFloor.materials[i].shader   = lightingShader;
    for (int i = 0; i < woodWall.materialCount;   ++i) woodWall.materials[i].shader   = lightingShader;
    for (int i = 0; i < woodWallHalf.materialCount;   ++i) woodWallHalf.materials[i].shader   = lightingShader;
    for (int i = 0; i < woodDoorWay.materialCount;   ++i) woodDoorWay.materials[i].shader   = lightingShader;
    for (int i = 0; i < tableSet.materialCount;   ++i) tableSet.materials[i].shader   = lightingShader;
    for (int i = 0; i < cratePile.materialCount;   ++i) cratePile.materials[i].shader   = lightingShader;
    for (int i = 0; i < stool.materialCount;   ++i) stool.materials[i].shader   = lightingShader;
    for (int i = 0; i < bonePile.materialCount;   ++i) bonePile.materials[i].shader   = lightingShader;
    for (int i = 0; i < candelabra.materialCount;   ++i) candelabra.materials[i].shader   = lightingShader;

    // if (isDungeon){
    //     for (int i = 0; i < blunderbuss.materialCount;   ++i) blunderbuss.materials[i].shader   = lightingShader;
    //     for (int i = 0; i < crossbow.materialCount;   ++i) crossbow.materials[i].shader   = lightingShader;
    //     for (int i = 0; i < sword.materialCount;   ++i) sword.materials[i].shader   = lightingShader;
    //     for (int i = 0; i < staff.materialCount;   ++i) staff.materials[i].shader   = lightingShader;
    //     for (int i = 0; i < crossbowRest.materialCount;   ++i) crossbowRest.materials[i].shader   = lightingShader;

    // }


    // Bind the lightmap texture to EMISSION slot for each model material
    auto setLightmap = [&](Model& m){
        for (int i = 0; i < m.materialCount; ++i){
            m.materials[i].maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;
        }
    };

    //setLightmap(floorModel);
    //setLightmap(wallModel);
    setLightmap(windowModel);
    setLightmap(doorwayModel);
    setLightmap(launcherModel);
    setLightmap(barrelModel);
    setLightmap(brokeModel);
    setLightmap(boxModel);
    setLightmap(woodFloor);
    setLightmap(woodWall);
    setLightmap(woodWallHalf);
    setLightmap(woodDoorWay);
    setLightmap(tableSet);
    setLightmap(cratePile);
    setLightmap(stool);
    setLightmap(bonePile);
    setLightmap(candelabra);
    // if (isDungeon){
    //     setLightmap(blunderbuss);
    //     setLightmap(crossbow);
    //     setLightmap(crossbowRest);
    //     setLightmap(sword);
    //     setLightmap(staff);

    // }

    // Per-level uniforms for lighting shader
    Shader& use = windowModel.materials[0].shader;


    int locGrid   = GetShaderLocation(use, "gridBounds");
    int locDynStr = GetShaderLocation(use, "dynStrength");
    int locAmb    = GetShaderLocation(use, "ambientBoost");

    float grid[4] = {
        gDynamic.minX, gDynamic.minZ,
        gDynamic.sizeX ? 1.0f / gDynamic.sizeX : 0.0f,
        gDynamic.sizeZ ? 1.0f / gDynamic.sizeZ : 0.0f
    };

    float dynStrength  = lightConfig.dynStrength;
    float ambientBoost = lightConfig.ambient;

    if (locGrid   >= 0) SetShaderValue(use, locGrid,   grid,        SHADER_UNIFORM_VEC4);
    if (locDynStr >= 0) SetShaderValue(use, locDynStr, &dynStrength,  SHADER_UNIFORM_FLOAT);
    if (locAmb    >= 0) SetShaderValue(use, locAmb,    &ambientBoost, SHADER_UNIFORM_FLOAT);


}



void ResourceManager::UpdateShaders(Camera& camera){
    //SetWaterShaderValues(camera); //update water every frame
    //runs every frame, updates all shaders

    Shader& terrainShader = R.GetShader("terrainShader");
    Vector3 camPos = camera.position;

    //terrain fog locs
    int camPosLoc = GetShaderLocation(terrainShader, "cameraPos");
    int tFogStartLoc = GetShaderLocation(terrainShader, "u_FogStart");
    int fogColorLoc = GetShaderLocation(terrainShader, "u_SkyColorHorizon");
    int fogColorTopLoc = GetShaderLocation(terrainShader, "u_SkyColorTop");
    int useFogLoc = GetShaderLocation(terrainShader, "u_UseFog");
    int locCam_Terrain = GetShaderLocation(terrainShader, "cameraPos");

    float terrainFogStart = (currentGameState == GameState::Menu) ? GameSettings::terrainFogStartMenu : GameSettings::terrainFogStart;
    int useFog = GameSettings::useFog;//1;//(currentGameState == GameState::Menu) ? 0 : 1; //dont render fog in menu.

    SetShaderValue( terrainShader, useFogLoc, &useFog, SHADER_UNIFORM_INT);

    //SetShaderValue(R.GetShader("treeShader"), fogStartLoc, &fogStart, SHADER_UNIFORM_FLOAT);
    Vector3 fogColor = ShaderSetup::GetCurrentSkyFogColor();
    Vector3 topFogColor = ShaderSetup::GetCurrentSkyTopFogColor();
    SetShaderValue(terrainShader, fogColorLoc, &fogColor, SHADER_UNIFORM_VEC3); //use current sky color
    SetShaderValue(terrainShader, fogColorTopLoc, &topFogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, tFogStartLoc, &terrainFogStart, SHADER_UNIFORM_FLOAT);
    //distance based desaturation on terrain needs camera pos
    SetShaderValue(terrainShader, camPosLoc, &camPos, SHADER_UNIFORM_VEC3);


    //tree shadows
    // Once (cache locations)
    int locShadow      = GetShaderLocation(terrainShader, "u_ShadowMask");
    int locWorldMinXZ  = GetShaderLocation(terrainShader, "u_WorldMinXZ");
    int locWorldSizeXZ = GetShaderLocation(terrainShader, "u_WorldSizeXZ");

    
    // Per-frame before drawing terrain:
    Vector2 worldMinXZ  = { gTreeShadowMask.worldXZBounds.x, gTreeShadowMask.worldXZBounds.y };
    Vector2 worldSizeXZ = { gTreeShadowMask.worldXZBounds.width, gTreeShadowMask.worldXZBounds.height };

    SetShaderValue(terrainShader, locWorldMinXZ,  &worldMinXZ,  SHADER_UNIFORM_VEC2);
    SetShaderValue(terrainShader, locWorldSizeXZ, &worldSizeXZ, SHADER_UNIFORM_VEC2);
    SetShaderValueTexture(terrainShader, locShadow, gTreeShadowMask.rt.texture);

    //distance fog

    float nightDarkness = ShaderSetup::gSky.skyTransition;

    SetShaderValue(terrainShader,GetShaderLocation(terrainShader, "u_TerrainNightDarkness"),&nightDarkness,SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, locCam_Terrain, &camPos, SHADER_UNIFORM_VEC3);

}

void ResourceManager::UnloadRenderTextures()
{
    for (auto& [name, ptr] : _renderTextures)
    {
        if (ptr && ptr->id != 0)
            UnloadRenderTexture(*ptr);
    }
    _renderTextures.clear();
}



void ResourceManager::UnloadAll() {
    UnloadContainer(_textures,        ::UnloadTexture);
    UnloadContainer(_models,          ::UnloadModel);
    UnloadContainer(_shaders,         ::UnloadShader);
    R.UnloadRenderTextures();
    UnloadAllFonts();
    if (_fallbackTex.id) { UnloadTexture(_fallbackTex); _fallbackTex = {}; }
    _fallbackReady = false;
}

ResourceManager::~ResourceManager() {
    UnloadAll();
}

