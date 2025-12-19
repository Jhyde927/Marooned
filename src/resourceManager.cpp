
#include "resourceManager.h"
#include "world.h"
#include "lighting.h"
#include "level.h"
//#include "shadows.h"
#include "vegetation.h"
#include "terrainChunking.h"
#include "rlgl.h"
#include "utilities.h"


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

Model& ResourceManager::LoadModelFromMesh(const std::string& name, const Mesh& mesh) {
    auto it = _models.find(name);
    if (it != _models.end()) return it->second;
    Model m = ::LoadModelFromMesh(mesh);
    _models.emplace(name, m);
    return _models[name];
}
Model& ResourceManager::GetModel(const std::string& name) const {
    auto it = _models.find(name);
    if (it == _models.end()) throw std::runtime_error("Model not found: " + name);
    return const_cast<Model&>(it->second);
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
RenderTexture2D& ResourceManager::LoadRenderTexture(const std::string& name, int w, int h) { 
    auto it = _renderTextures.find(name); if (it != _renderTextures.end()) return it->second; 
    RenderTexture2D rt = ::LoadRenderTexture(w, h); _renderTextures.emplace(name, rt); 
    return _renderTextures[name]; 
}

RenderTexture2D& ResourceManager::GetRenderTexture(const std::string& name) const {
    auto it = _renderTextures.find(name);
    if (it == _renderTextures.end()) throw std::runtime_error("RenderTexture not found: " + name);
    return const_cast<RenderTexture2D&>(it->second);
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
static int gLastW = -1, gLastH = -1;

void ResourceManager::EnsureScreenSizedRTs() {
    int w = GetScreenWidth(), h = GetScreenHeight();
    if (w == gLastW && h == gLastH) return;

    // Recreate sceneTexture
    if (_renderTextures.count("sceneTexture")) {
        UnloadRenderTexture(_renderTextures["sceneTexture"]);
    }
    _renderTextures["sceneTexture"] = LoadRenderTexture("sceneTexture", w, h);
    SetTextureWrap(_renderTextures["sceneTexture"].texture, TEXTURE_WRAP_CLAMP);

    // Recreate postProcessTexture
    if (_renderTextures.count("postProcessTexture")) {
        UnloadRenderTexture(_renderTextures["postProcessTexture"]);
    }
    _renderTextures["postProcessTexture"] = LoadRenderTexture("postProcessTexture", w, h);
    SetTextureWrap(_renderTextures["postProcessTexture"].texture, TEXTURE_WRAP_CLAMP);

    gLastW = w; gLastH = h;
}



void ResourceManager::LoadAllResources() {
    Vector2 screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    // ResourceManager init once

    //render textures
    RenderTexture& scene = R.LoadRenderTexture("sceneTexture", screenResolution.x, screenResolution.y);
    SetTextureFilter(scene.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(scene.texture, TEXTURE_WRAP_CLAMP);

    RenderTexture& post = R.LoadRenderTexture("postProcessTexture", screenResolution.x, screenResolution.y);
    SetTextureFilter(post.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(post.texture, TEXTURE_WRAP_CLAMP);

    SetTextureWrap(R.GetRenderTexture("sceneTexture").texture, TEXTURE_WRAP_CLAMP);
    SetTextureWrap(R.GetRenderTexture("postProcessTexture").texture, TEXTURE_WRAP_CLAMP);

    R.LoadFont("Pieces", "assets/fonts/PiecesOfEight.ttf", 128, 1);
    R.LoadFont("Kingthings", "assets/fonts/KingthingsPetrock.ttf", 128, 1);

    //Resources are saved to unordered maps, with a string key. Get a resource by calling R.GetModel("blunderbuss") for example. 
    R.LoadTexture("raptorTexture",    "assets/sprites/bigRaptorSheet.png");
    R.LoadTexture("skeletonSheet",    "assets/sprites/skeletonSheet.png");
    R.LoadTexture("muzzleFlash",      "assets/sprites/muzzleFlash.png");
    R.LoadTexture("backDrop",         "assets/screenshots/dungeon1.png");
    R.LoadTexture("smokeSheet",       "assets/sprites/smokeSheet.png");
    R.LoadTexture("bloodSheet",       "assets/sprites/bloodDecalSheet.png");
    R.LoadTexture("doorTexture",      "assets/sprites/Door.png");
    R.LoadTexture("healthPotTexture", "assets/sprites/Healthpot.png");
    R.LoadTexture("keyTexture",       "assets/sprites/key.png");
    R.LoadTexture("swordBloody",      "assets/textures/swordBloody.png");
    R.LoadTexture("swordClean",       "assets/textures/swordClean.png");
    R.LoadTexture("fireSheet",        "assets/sprites/fireSheet.png");
    R.LoadTexture("pirateSheet",      "assets/sprites/pirateSheet.png");
    R.LoadTexture("coinTexture",      "assets/sprites/coin.png");
    R.LoadTexture("spiderSheet",      "assets/sprites/spiderSheet.png");
    R.LoadTexture("spiderWebTexture", "assets/sprites/spiderWeb.png");
    R.LoadTexture("brokeWebTexture",  "assets/sprites/brokeWeb.png");
    R.LoadTexture("explosionSheet",   "assets/sprites/explosionSheet.png");
    R.LoadTexture("manaPotion",       "assets/sprites/manaPotion.png");
    R.LoadTexture("fireIcon",         "assets/sprites/fireIcon.png");
    R.LoadTexture("iceIcon",          "assets/sprites/iceIcon.png");
    R.LoadTexture("shadowTex",        "assets/textures/shadow_decal.png");
    R.LoadTexture("ghostSheet",       "assets/sprites/ghostSheet.png");
    R.LoadTexture("magicAttackSheet", "assets/sprites/magicAttackSheet.png");
    R.LoadTexture("treeShadow",       "assets/textures/treeShadow.png");
    R.LoadTexture("grassTexture",     "assets/textures/grass2.png");
    R.LoadTexture("sandTexture",      "assets/textures/sand.png");
    R.LoadTexture("trexSheet",        "assets/sprites/trexSheet.png");
    R.LoadTexture("blockSheet",       "assets/sprites/blockSheet2.png");
    R.LoadTexture("playerSlashSheet", "assets/sprites/playerSlashSheet.png");
    R.LoadTexture("slashSheet",       "assets/sprites/slashSheet.png");
    R.LoadTexture("slashSheetLeft",   "assets/sprites/slashSheetLeft.png");
    R.LoadTexture("biteSheet",        "assets/sprites/biteSheet.png");
    R.LoadTexture("bulletHoleSheet",  "assets/sprites/bulletHoleSheet.png");
    R.LoadTexture("GiantSpiderSheet", "assets/sprites/giantSpiderSheet.png");
    R.LoadTexture("spiderEggSheet",   "assets/sprites/spiderEggSheet.png");
    R.LoadTexture("blank",            "assets/textures/blank.png");
    R.LoadTexture("silverKey",        "assets/sprites/silverKey.png");
    R.LoadTexture("harpoon",          "assets/sprites/harpoon.png");
    R.LoadTexture("menuButton",       "assets/sprites/menuButton.png");


    // Models (registering with string keys)
    R.LoadModel("palmTree",               "assets/Models/bigPalmTree.glb");
    R.LoadModel("palm2",                  "assets/Models/smallPalmTree.glb");
    R.LoadModel("bush",                   "assets/Models/grass(stripped).glb");
    R.LoadModel("boatModel",              "assets/Models/boat.glb");
    R.LoadModel("blunderbuss",            "assets/Models/blunderbus.glb");
    R.LoadModel("floorTileGray",          "assets/Models/floorTileGray.glb");
    R.LoadModel("doorWayGray",            "assets/Models/doorWayGray.glb");
    R.LoadModel("wallSegment",            "assets/Models/wallSegment.glb");
    R.LoadModel("barrelModel",            "assets/Models/barrel.glb");
    R.LoadModel("swordModel",             "assets/Models/sword.glb");
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




    //generated models
    // Fins: a bit wider and flatter, slightly shorter
    R.LoadModelFromMesh("boltFinsModel", GenMeshCube(3.0f, 3.0f, 5.0f));

    R.LoadModelFromMesh("squareBolt", GenMeshCube(2.0f, 2.0f, 20.0f));

    R.LoadModelFromMesh("skyModel", GenMeshCube(1.0f, 1.0f, 1.0f));
    R.LoadModelFromMesh("waterModel",GenMeshPlane(16000, 160000, 1, 1));
    R.LoadModelFromMesh("shadowQuad",GenMeshPlane(1.0f, 1.0f, 1, 1)); //still used for enemy shadows
    R.LoadModelFromMesh("bottomPlane",GenMeshPlane(16000, 160000, 1, 1));
    
    //shaders
    R.LoadShader("terrainShader",  "assets/shaders/height_color.vs",       "assets/shaders/height_color.fs");
    R.LoadShader("fogShader",      /*vsPath=*/"",                          "assets/shaders/fog_postprocess.fs");
    R.LoadShader("shadowShader",   "assets/shaders/shadow_decal.vs",       "assets/shaders/shadow_decal.fs");
    R.LoadShader("skyShader",      "assets/shaders/skybox.vs",             "assets/shaders/skybox.fs");
    R.LoadShader("waterShader",    "assets/shaders/water.vs",              "assets/shaders/water.fs");
    R.LoadShader("bloomShader",    /*vsPath=*/"",                          "assets/shaders/bloom.fs");
    R.LoadShader("cutoutShader",   /*vsPath=*/"",                          "assets/shaders/leaf_cutout.fs");
    R.LoadShader("lightingShader", "assets/shaders/lighting_baked_xz.vs",  "assets/shaders/lighting_baked_xz.fs");
    R.LoadShader("lavaShader",     "assets/shaders/lava_world.vs",         "assets/shaders/lava_world.fs");
    R.LoadShader("treeShader",     "assets/shaders/treeShader.vs",         "assets/shaders/treeShader.fs");
    R.LoadShader("portalShader",   "assets/shaders/portal.vs",             "assets/shaders/portal.fs");



}

void ResourceManager::SetShaderValues(){

    Vector2 screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
    // set shaders values
    Shader& fogShader = R.GetShader("fogShader");
    Shader& shadowShader = R.GetShader("shadowShader");
    Shader& waterShader = R.GetShader("waterShader");

    //regular black vignette
    vignetteStrengthValue = isDungeon ? 0.8 : 0.25f; //less of vignette outdoors.
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "baseVignetteStrength"), &vignetteStrengthValue, SHADER_UNIFORM_FLOAT);

    // Sky
    //apply skyShader to sky model
    Shader& sky = R.GetShader("skyShader");
    R.GetModel("skyModel").materials[0].shader = sky;


    // Cache uniform locations
    int locTime      = GetShaderLocation(sky, "time");
    int locIsDungeon = GetShaderLocation(sky, "isDungeon");

    int isDung = isDungeon ? 1 : 0; 
    SetShaderValue(sky, locIsDungeon, &isDung, SHADER_UNIFORM_INT);


    // Shadow //Shadow decals beneath trees. Also shadows beneath enemies. 
    Model& shadowQuad = R.GetModel("shadowQuad");
    shadowQuad.materials[0].shader = shadowShader;
    SetMaterialTexture(&shadowQuad.materials[0], MATERIAL_MAP_DIFFUSE, R.GetTexture("shadowTex"));

    // Water
    
    SetShaderValue(waterShader, GetShaderLocation(waterShader, "waterLevel"), &waterHeightY, SHADER_UNIFORM_FLOAT);
    R.GetModel("waterModel").materials[0].shader = waterShader;

    Model& boltModel = R.GetModel("bolt");

    for (int i = 0; i < boltModel.materialCount; ++i) {
        Material& mat = boltModel.materials[i];

        // Use default shader (ignore dungeon lighting / custom stuff)
        // mat = LoadMaterialDefault();
        // mat.shader = GetShaderDefault();

        // In raylib 4/5, usually:
        // - MATERIAL_MAP_ALBEDO is the main color map
        // - MATERIAL_MAP_DIFFUSE is also often there for compatibility


            // Create an image with white pixels
        Image whiteImage = GenImageColor(100, 100, WHITE);  // Generates a 100x100 white image 

        // Convert the image to a texture
        Texture2D whiteTexture = LoadTextureFromImage(whiteImage);
        Texture2D whiteTex = whiteTexture;

        mat.maps[MATERIAL_MAP_ALBEDO].texture = whiteTex;
        mat.maps[MATERIAL_MAP_ALBEDO].color   = (Color){ 255, 100, 100, 255 }; // neon cyan

        // mat.maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
        // mat.maps[MATERIAL_MAP_DIFFUSE].color   = (Color){ 255, 255, 255, 255 };
    }

}

void ResourceManager::SetPortalShaderValues(){
    Shader portal = R.GetShader("portalShader");

    int loc_speed         = GetShaderLocation(portal, "u_speed");
    int loc_swirlStrength = GetShaderLocation(portal, "u_swirlStrength");
    int loc_swirlScale    = GetShaderLocation(portal, "u_swirlScale");
    int loc_colorA        = GetShaderLocation(portal, "u_colorA");
    int loc_colorB        = GetShaderLocation(portal, "u_colorB");
    int loc_edgeFeather   = GetShaderLocation(portal, "u_edgeFeather");
    int loc_rings         = GetShaderLocation(portal, "u_rings");
    int loc_glowBoost     = GetShaderLocation(portal, "u_glowBoost");

    float loc_speed_ptr[] {1.4f};
    float loc_swirlStrength_ptr[] {1.2f};
    float loc_swirlScale_ptr[] {12.0f};
    float loc_edgeFeather_ptr[] {0.08f};
    float loc_rings_ptr[] {0.7f};
    float loc_glowBoost_ptr[] {0.8f};

    // One-time defaults
    SetShaderValue(portal, loc_speed,         loc_speed_ptr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portal, loc_swirlStrength, loc_swirlStrength_ptr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portal, loc_swirlScale,    loc_swirlScale_ptr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portal, loc_edgeFeather,   loc_edgeFeather_ptr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portal, loc_rings,         loc_rings_ptr, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portal, loc_glowBoost,     loc_glowBoost_ptr, SHADER_UNIFORM_FLOAT);

    // Colors 
    Vector3 cA = {0.0f, 0.25f, 1.0f};
    Vector3 cB = {0.5f, 0.2f, 1.0f};
    SetShaderValue(portal, loc_colorA, &cA, SHADER_UNIFORM_VEC3);
    SetShaderValue(portal, loc_colorB, &cB, SHADER_UNIFORM_VEC3);

}

void ResourceManager::SetWaterShaderValues(Camera& camera) {
    //runs every frame
    Shader water = R.GetShader("waterShader");

    Vector2 worldMinXZ  = { -terrainScale.x * 0.5f, -terrainScale.z * 0.5f };
    Vector2 worldSizeXZ = {  terrainScale.x,          terrainScale.z       };


    float halfSize   = 8000.0f;   
    float fadeStart  = 10000.0f;
    float fadeEnd    = 16000.0f;


    Vector2 worldMin = worldMinXZ;              // (minX, minZ)
    Vector2 worldMax = worldMin + worldSizeXZ;  // (maxX, maxZ)

    float patchHalf = 8000.0f;     // same as u_PatchHalfSize
    float feather   = 600.0f;      // ~the width of the edge fade band

    // Clamp the center so the patch edge + feather never crosses the world edge
    float minX = worldMin.x + (patchHalf - feather);
    float maxX = worldMax.x - (patchHalf - feather);
    float minZ = worldMin.y + (patchHalf - feather);
    float maxZ = worldMax.y - (patchHalf - feather);

    Vector2 centerXZ = {
        Clamp(camera.position.x, minX, maxX),
        Clamp(camera.position.z, minZ, maxZ)
    };

    SetShaderValue(water, GetShaderLocation(water, "u_WaterCenterXZ"), &centerXZ, SHADER_UNIFORM_VEC2);
    SetShaderValue(water, GetShaderLocation(water, "u_PatchHalfSize"), &halfSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(water, GetShaderLocation(water, "u_FadeStart"),     &fadeStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(water, GetShaderLocation(water, "u_FadeEnd"),       &fadeEnd,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(water, GetShaderLocation(water, "cameraPos"),       &camera.position, SHADER_UNIFORM_VEC3);

}


void ResourceManager::SetTerrainShaderValues(){ //plus palm tree shader
    // Load textures (tileable, power-of-two helps mips)
    Shader& terrainShader = R.GetShader("terrainShader");

    Shader& sh = R.GetShader("terrainShader");

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
    int locGrassTile   = GetShaderLocation(terrainShader, "grassTiling");
    int locSandTile    = GetShaderLocation(terrainShader, "sandTiling");

    Vector2 t_worldMinXZ  = { -terrainScale.x * 0.5f, -terrainScale.z * 0.5f };
    Vector2 t_worldSizeXZ = {  terrainScale.x,          terrainScale.z       };
    SetShaderValue(terrainShader, locWorldMinXZ,  &t_worldMinXZ,  SHADER_UNIFORM_VEC2);
    SetShaderValue(terrainShader, locWorldSizeXZ, &t_worldSizeXZ, SHADER_UNIFORM_VEC2);



    // --- Fog and sky
    Vector3 skyTop  = {0.55f, 0.75f, 1.00f};
    Vector3 skyHorz = {0.60f, 0.80f, 0.95f};
    float fogStart  = 100.0f;
    float fogEnd    = 18000.0f;
    float seaLevel  = 400.0f;
    float falloff   = 0.002f;

    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SkyColorTop"),      &skyTop,   SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SkyColorHorizon"),  &skyHorz,  SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogStart"),         &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogEnd"),           &fogEnd,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_SeaLevel"),         &seaLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(terrainShader, GetShaderLocation(terrainShader, "u_FogHeightFalloff"), &falloff,  SHADER_UNIFORM_FLOAT);

    // Load tree shader
    Shader treeShader = R.GetShader("treeShader");

    Model& treeModel = R.GetModel("palmTree");
    Model& smallTreeModel = R.GetModel("palm2");
    Model& bushModel = R.GetModel("bush");
    Model& doorwayModel = R.GetModel("doorWayGray");

    // Hook ALBEDO to our sampler name
    treeShader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(treeShader, "textureDiffuse");

    // (optional) hook diffuse tint if you plan to use it; otherwise raylib will handle it
    treeShader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(treeShader, "colDiffuse");

    // Set shared fog uniforms once (reuse the same values as terrain)
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_SkyColorTop"),      &skyTop,   SHADER_UNIFORM_VEC3);
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_SkyColorHorizon"),  &skyHorz,  SHADER_UNIFORM_VEC3);
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_FogStart"),         &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_FogEnd"),           &fogEnd,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_SeaLevel"),         &seaLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"u_FogHeightFalloff"), &falloff,  SHADER_UNIFORM_FLOAT);




    // Alpha cutoff (tweak per asset)
    float alphaCut = 0.30f;
    SetShaderValue(treeShader, GetShaderLocation(treeShader,"alphaCutoff"), &alphaCut, SHADER_UNIFORM_FLOAT);


    for (int i = 0; i < doorwayModel.materialCount; ++i) {
        doorwayModel.materials[i].shader = treeShader;
        doorwayModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    }


    for (int i = 0; i < bushModel.materialCount; ++i) {
        bushModel.materials[i].shader = treeShader;
        bushModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    }

    for (int i = 0; i < treeModel.materialCount; ++i) {
        treeModel.materials[i].shader = treeShader;
        smallTreeModel.materials[i].shader = treeShader; //smallTree matrials as well. 
        
        
        // IMPORTANT: keep map tint white so it doesn’t darken
        treeModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        smallTreeModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    }



}

void ResourceManager::SetBloomShaderValues(){
    //bloom post process. 
    Shader& bloomShader = R.GetShader("bloomShader");
    Vector2 screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    bloomStrengthValue = 0.0f; //No bloom
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "bloomStrength"), &bloomStrengthValue, SHADER_UNIFORM_FLOAT);

    //tonemap
    float exposure = isDungeon ? lightConfig.dungeonExposure : lightConfig.islandExposure; // needed for both dungeons and outdoor level
    int toneOp = isDungeon ? 1 : 0;
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "uExposure"), &exposure, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "uToneMapOperator"), &toneOp, SHADER_UNIFORM_INT);

    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "resolution"), &screenResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "bloomStrength"), &bloomStrengthValue, SHADER_UNIFORM_FLOAT);
    
}

void ResourceManager::SetLavaShaderValues(){
    Shader lavaShader = R.GetShader("lavaShader");
    Model& lavaTile = R.GetModel("lavaTile");

    for (int i=0; i < lavaTile.materialCount; i++){
        lavaTile.materials[i].shader = lavaShader;
    }

    // Hook locations once
    int locTime   = GetShaderLocation(lavaShader, "uTime");
    int locDir    = GetShaderLocation(lavaShader, "uScrollDir");
    int locSpeed  = GetShaderLocation(lavaShader, "uSpeed");
    int locOff    = GetShaderLocation(lavaShader, "uWorldOffset");
    int locScale  = GetShaderLocation(lavaShader, "uUVScale");
    int locFreq   = GetShaderLocation(lavaShader, "uDistortFreq");
    int locAmp    = GetShaderLocation(lavaShader, "uDistortAmp");
    int locEmis   = GetShaderLocation(lavaShader, "uEmissive");
    int locGain   = GetShaderLocation(lavaShader, "uEmissiveGain");

    // Parameters
    Vector2 scroll = { 0.07f, 0.0f };
    float speed = 0.5f, freq = 6.0f, amp = 0.02f;
    float gain = 0.1f;
    Vector3 emis = { 3.0f, 1.0f, 0.08f };

    // World-to-UV scale:
    //   If your tile is 'tileSize' world units and you want 1 repeat per *tile*,
    //   then uUVScale = 1.0f / tileSize.
    //   If you want 2 repeats per tile: 2.0f / tileSize, etc.
    float uvsPerWorldUnit = 0.5 / tileSize;

    // Dungeon/world origin 
    Vector2 worldOffset = {0,0};

    // Set static params once
    SetShaderValue(lavaShader, locDir,   &scroll,       SHADER_UNIFORM_VEC2);
    SetShaderValue(lavaShader, locSpeed, &speed,        SHADER_UNIFORM_FLOAT);
    SetShaderValue(lavaShader, locFreq,  &freq,         SHADER_UNIFORM_FLOAT);
    SetShaderValue(lavaShader, locAmp,   &amp,          SHADER_UNIFORM_FLOAT);
    SetShaderValue(lavaShader, locEmis,  &emis,         SHADER_UNIFORM_VEC3);
    SetShaderValue(lavaShader, locGain,  &gain,         SHADER_UNIFORM_FLOAT);
    SetShaderValue(lavaShader, locOff,   &worldOffset,  SHADER_UNIFORM_VEC2);
    SetShaderValue(lavaShader, locScale, &uvsPerWorldUnit, SHADER_UNIFORM_FLOAT);
}



void ResourceManager::SetLightingShaderValues() {

    Shader& lightingShader = R.GetShader("lightingShader");

    Model& floorModel    = R.GetModel("floorTileGray");
    Model& wallModel     = R.GetModel("wallSegment");
    Model& doorwayModel  = R.GetModel("doorWayGray");
    Model& launcherModel = R.GetModel("stonePillar");
    Model& barrelModel   = R.GetModel("barrelModel");
    Model& brokeModel    = R.GetModel("brokeBarrel");

    // assign shader to all dungeon materials
    for (int i = 0; i < wallModel.materialCount; i++)      wallModel.materials[i].shader   = lightingShader;
    for (int i = 0; i < doorwayModel.materialCount; i++)  doorwayModel.materials[i].shader= lightingShader;
    for (int i = 0; i < floorModel.materialCount; ++i)    floorModel.materials[i].shader  = lightingShader;
    for (int i = 0; i < launcherModel.materialCount; ++i) launcherModel.materials[i].shader = lightingShader;
    for (int i = 0; i < barrelModel.materialCount; ++i)   barrelModel.materials[i].shader = lightingShader;
    for (int i = 0; i < brokeModel.materialCount; ++i)    brokeModel.materials[i].shader  = lightingShader;

    // IMPORTANT: map EMISSION -> texture4 sampler in shader
    lightingShader.locs[SHADER_LOC_MAP_EMISSION] =
        GetShaderLocation(lightingShader, "texture4");

    // bind the lightmap to EMISSION slot for every model
    auto setLightmap = [&](Model& m){
        for (int i = 0; i < m.materialCount; ++i){
            m.materials[i].maps[MATERIAL_MAP_EMISSION].texture = gDynamic.tex;
        }
    };
    setLightmap(floorModel);
    setLightmap(wallModel);
    setLightmap(doorwayModel);
    setLightmap(launcherModel);
    setLightmap(barrelModel);
    setLightmap(brokeModel);

    Shader use = floorModel.materials[0].shader;

    int locGrid   = GetShaderLocation(use, "gridBounds");
    int locDynStr = GetShaderLocation(use, "dynStrength");
    int locAmb    = GetShaderLocation(use, "ambientBoost");

    float grid[4] = {
        gDynamic.minX, gDynamic.minZ,
        gDynamic.sizeX ? 1.0f / gDynamic.sizeX : 0.0f,
        gDynamic.sizeZ ? 1.0f / gDynamic.sizeZ : 0.0f
    };

    if (locGrid >= 0) SetShaderValue(use, locGrid, grid, SHADER_UNIFORM_VEC4);

    float dynStrength  = lightConfig.dynStrength;
    float ambientBoost = lightConfig.ambient;
    if (locDynStr >= 0) SetShaderValue(use, locDynStr, &dynStrength,  SHADER_UNIFORM_FLOAT);
    if (locAmb    >= 0) SetShaderValue(use, locAmb,    &ambientBoost, SHADER_UNIFORM_FLOAT);
}

void ResourceManager::UpdateShaders(Camera& camera){
    SetWaterShaderValues(camera); //update water every frame
    //runs every frame, updates all shaders
    Vector2 screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
    Shader& waterShader = R.GetShader("waterShader");
    Shader& skyShader = R.GetShader("skyShader");
    Shader& terrainShader = R.GetShader("terrainShader");
    Shader& fogShader = R.GetShader("fogShader");
    Shader& treeShader = R.GetShader("treeShader");

    Vector3 camPos = camera.position;

    float t = GetTime();
    int dungeonFlag = isDungeon ? 1 : 0;
    int isDungeonLoc = GetShaderLocation(skyShader, "isDungeon");
    int camLoc = GetShaderLocation(waterShader, "cameraPos");
    int camPosLoc = GetShaderLocation(terrainShader, "cameraPos");

    //water shader needs cameraPos for reasons. 
    SetShaderValue(waterShader, camLoc, &camPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(waterShader, GetShaderLocation(waterShader, "time"), &t, SHADER_UNIFORM_FLOAT);
    
    //distance based desaturation on terrain needs camera pos
    SetShaderValue(terrainShader, camPosLoc, &camPos, SHADER_UNIFORM_VEC3);



    //animate sky needs elapsed time
    SetShaderValue(skyShader, GetShaderLocation(skyShader, "time"), &t, SHADER_UNIFORM_FLOAT);
    SetShaderValue(skyShader, isDungeonLoc, &dungeonFlag, SHADER_UNIFORM_INT);

    //red vignette intensity over time
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "vignetteIntensity"), &vignetteIntensity, SHADER_UNIFORM_FLOAT);

    //dungeonDarkness //is there a reason we need to set these every frame? 
    float dungeonDarkness = -0.1f;//it darkens the gun model as well, so go easy. negative number brightens it. 
    float dungeonContrast = 1.00f; //makes darks darker. 

    int isDungeonVal = isDungeon ? 1 : 0; 
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "resolution"), &screenResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "isDungeon"), &isDungeonVal, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "dungeonDarkness"), &dungeonDarkness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "dungeonContrast"), &dungeonContrast, SHADER_UNIFORM_FLOAT);
    if (!isLoadingLevel){
        Shader lavaShader = R.GetShader("lavaShader");
        int locTime        = GetShaderLocation(lavaShader, "uTime");
        SetShaderValue(R.GetShader("lavaShader"), locTime, &t, SHADER_UNIFORM_FLOAT);

    }
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
    int locCam_Terrain = GetShaderLocation(terrainShader, "cameraPos");
    int locCam_Trees   = GetShaderLocation(treeShader,   "cameraPos");

    SetShaderValue(terrainShader, locCam_Terrain, &camPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(treeShader,   locCam_Trees,   &camPos, SHADER_UNIFORM_VEC3);

    int loc_time_p = GetShaderLocation(R.GetShader("portalShader"), "u_time");
    //portal
    SetShaderValue(R.GetShader("portalShader"), loc_time_p, &t, SHADER_UNIFORM_FLOAT);


}


void ResourceManager::UnloadAll() {
    UnloadContainer(_textures,        ::UnloadTexture);
    UnloadContainer(_models,          ::UnloadModel);
    UnloadContainer(_shaders,         ::UnloadShader);
    UnloadContainer(_renderTextures,  ::UnloadRenderTexture);
    UnloadAllFonts();
    if (_fallbackTex.id) { UnloadTexture(_fallbackTex); _fallbackTex = {}; }
    _fallbackReady = false;
}

ResourceManager::~ResourceManager() {
    UnloadAll();
}

