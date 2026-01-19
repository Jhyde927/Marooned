#include "world.h"
#include "player.h"
#include "raymath.h"
#include "resourceManager.h"
#include "vegetation.h"
#include "dungeonGeneration.h"
#include "boat.h"
#include "algorithm"
#include "sound_manager.h"
#include <iostream>
#include "pathfinding.h"
#include "camera_system.h"
#include "list"
#include "assert.h"
#include "lighting.h"
#include "ui.h"
#include "terrainChunking.h"
#include "spiderEgg.h"
#include "miniMap.h"
#include "heightmapPathfinding.h"
#include "shaderSetup.h"
#include "dialogManager.h"



GameState currentGameState = GameState::Menu;
MainMenu::State gMenu;
//global variables, clean these up somehow. 

Image heightmap;
Vector3 terrainScale = {16000.0f, 200.0f, 16000.0f}; //very large x and z, 
HeightmapNavGrid gIslandNav;

TreeShadowMask gTreeShadowMask;
int levelIndex = 0; //current level, levels[levelIndex]
int previousLevelIndex = 0;
bool first = true; //for first player start position
bool controlPlayer = false;
bool isDungeon = false;
unsigned char* heightmapPixels = nullptr;
Player player = {};
Vector3 startPosition = {5475.0f, 300.0f, -5665.0f}; //middle island start pos
Vector3 boatPosition = {-3727.55f,-20.0f, 5860.31f}; //set in level not here

Vector3 playerSpawnPoint = {0,0,0};
Vector3 waterPos = {0, 0, 0};
int pendingLevelIndex = -1; //wait to switch level until faded out. UpdateFade() needs to know the next level index. 
bool unlockEntrances = false; // unlock entrances after levelindex 4
bool drawCeiling = true;
bool levelLoaded = false;
float waterHeightY = 60;
Vector3 bottomPos = {0, waterHeightY - 100, 0};
float dungeonPlayerHeight = 100;
float ceilingHeight = 480;
float fadeToBlack = 0.0f;
float vignetteIntensity = 0.0f;
float vignetteFade = 0.0f;
int vignetteMode = 0;
float vignetteStrengthValue = 0.2;
float bloomStrengthValue = 0.0;

float fadeSpeed = 1.0f; // units per second

float tileSize = 200;
bool switchFromMenu = false;
int selectedOption = 0;
float floorHeight = 100; //100
float wallHeight = 270;
float dungeonEnemyHeight = 165;
float ElapsedTime = 0.0f;
bool debugInfo = false;
bool isLoadingLevel = false;
float weaponDarkness = 0.0f;
bool playerInit = false;
bool hasStaff = false;
bool hasBlunderbuss = false;
bool hasCrossbow = false;
bool hasHarpoon = false;
float fade = 0.0f;
bool isFullscreen = true;
bool hasIslandNav = false;
int gEnemyCounter = 0;
float lavaOffsetY = 250.0f;
bool enteredDungeon1 = false;

int gCurrentLevelIndex = -1;

FadePhase gFadePhase = FadePhase::Idle;

//std::vector<Bullet> activeBullets;
std::list<Bullet> activeBullets; // instead of std::vector

std::vector<Decal> decals;
std::vector<MuzzleFlash> activeMuzzleFlashes;
std::vector<Collectable> collectables;
std::vector<CollectableWeapon> worldWeapons; //weapon pickups
std::vector<Character> enemies;  
std::vector<Character*> enemyPtrs;
std::vector<NPC> gNPCs;
std::vector<DungeonEntrance> dungeonEntrances;
std::vector<PreviewInfo> levelPreviews;

MiniMap miniMap;

void EnterMenu() {
    EnableCursor();
    CinematicDesc cd{};
    cd.snapOnStart   = true;
    cd.orbitSpeedDeg = 1.0f;      // very slow
    cd.height        = 2000.0f;
    cd.lookSmooth    = 6.0f;
    cd.posSmooth     = 1.5f;

    if (isDungeon) {
        // 32 tiles * 200 = 6400. Center is (3200,0,3200)
        float worldSize = dungeonWidth * tileSize;   // or just 6400.0f if fixed
        cd.focus  = { worldSize * 0.5f, 0.0f, worldSize * 0.5f };
        cd.radius = worldSize * 0.7f;                // bigger than half (half=3200)
        cd.startAngleDeg = 180.0f;                   // starts on -Z side
    } else {
        cd.focus  = { 0.0f, 0.0f, 0.0f };            // set to your island center
        cd.radius = 12000.0f;
        cd.height = 3500.0f;
        cd.startAngleDeg = 180.0f;
    }

    CameraSystem::Get().StartCinematic(cd);
}

void InitMenuLevel(LevelData& level){
    ClearLevel();
    isDungeon = false;
    InitShaders();
    heightmap = LoadImage(level.heightmapPath.c_str());
    ImageFormat(&heightmap, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    terrain = BuildTerrainGridFromHeightmap(heightmap, terrainScale, 193, true); //193 bigger chunks less draw calls.
    GenerateEntrances();
    generateVegetation(); //vegetation checks entrance positions. generate after assinging entrances.

    InitBoat(player_boat,Vector3{0.0, -75, 0.0});
    R.SetShaderValues();
    //R.SetBloomShaderValues();
    ShaderSetup::InitBloomShader(R.GetShader("bloomShader"), ShaderSetup::gBloom);
    if (!isDungeon) R.SetTerrainShaderValues();

    CinematicDesc cd;
    cd.focus = { 0, 0, 0 };        // whatever looks good on your island
    cd.radius = 12000.0f;
    cd.height = 3500.0f;
    cd.orbitSpeedDeg = 2.0f;       // slow
    cd.posSmooth = 1.5f;
    cd.lookSmooth = 6.0f;
    
    CameraSystem::Get().StartCinematic(cd);
}

void InitShaders(){

    ShaderSetup::InitPortalShader(R.GetShader("portalShader"), ShaderSetup::gPortal);
    ShaderSetup::InitWaterShader(R.GetShader("waterShader"), ShaderSetup::gWater, terrainScale);
    ShaderSetup::InitLavaShader(R.GetShader("lavaShader"), ShaderSetup::gLava, R.GetModel("lavaTile"));
    ShaderSetup::InitBloomShader(R.GetShader("bloomShader"), ShaderSetup::gBloom);

    ShaderSetup::SetBloomTonemap(ShaderSetup::gBloom, isDungeon, lightConfig.islandExposure, lightConfig.dungeonExposure);
    ShaderSetup::SetBloomStrength(ShaderSetup::gBloom, 0.0f);

    Model& treeModel      = R.GetModel("palmTree");
    Model& smallTreeModel = R.GetModel("palm2");
    Model& bushModel      = R.GetModel("bush");
    Model& doorwayModel   = R.GetModel("doorWayGray");
    Model& swampTree      = R.GetModel("swampTree");
    Model& skyModel       = R.GetModel("skyModel");

    
    ShaderSetup::InitSkyShader(R.GetShader("skyShader"), ShaderSetup::gSky, skyModel, isDungeon);

    
    if (!isDungeon) ShaderSetup::InitTreeShader(R.GetShader("treeShader"), ShaderSetup::gTree, {
        &treeModel,
        &smallTreeModel,
        &bushModel,
        &doorwayModel,
        &swampTree
    });
}

void UpdateShadersPerFrame(float deltaTime,float ElapsedTime, Camera& camera){
    ShaderSetup::UpdateLavaShaderPerFrame(ShaderSetup::gLava, ElapsedTime, isLoadingLevel);
    ShaderSetup::UpdatePortalShader(ShaderSetup::gPortal, ElapsedTime);
    ShaderSetup::UpdateWaterShaderPerFrame(ShaderSetup::gWater, ElapsedTime, camera);
    ShaderSetup::UpdateTreeShader(ShaderSetup::gTree, camera);
    ShaderSetup::UpdateSkyShaderPerFrame(ShaderSetup::gSky, ElapsedTime);
}


void InitLevel(LevelData& level, Camera& camera) {
    //Make sure we end texture mode, was causing problems with terrain.
    EndTextureMode();
    DisableCursor();
    isLoadingLevel = true;
    isDungeon = false;
    
    //Called when starting game and changing level. init the level you pass it. the level is chosen by menu or door's linkedLevelIndex. 
    ClearLevel();//clears everything.

    CameraSystem::Get().StopCinematic();
    CameraSystem::Get().SetMode(CamMode::Player);
    //CameraSystem::Get().SnapAllToPlayer(); //put freecam at player pos
    
    //camera.position = player.position; //start as player, not freecam.
    levelIndex = level.levelIndex; //update current level index to new level. 
    gCurrentLevelIndex = levelIndex; //save current level globally so we can tell if we are changing levels or resuming. 


    if (level.heightmapPath.empty() && level.dungeonPath.empty()) {
        TraceLog(LOG_INFO, "Skipping placeholder level index %d", levelIndex);
        level = levels[2];
    }



    heightmap = LoadImage(level.heightmapPath.c_str());
    ImageFormat(&heightmap, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    terrain = BuildTerrainGridFromHeightmap(heightmap, terrainScale, 193, true); //193 bigger chunks less draw calls. 

    hasIslandNav = false;

    if (heightmap.data != NULL && heightmap.width > 0)
    {
        float navSeaLevel = 60/255.0f; // match your actual sea level

        gIslandNav = HeightmapPathfinding::BuildNavGridFromHeightmap(
            heightmap,
            256, 256,
            navSeaLevel,
            terrainScale.x,
            terrainScale.z
        );

        if (gIslandNav.gridWidth > 0 && !gIslandNav.walkable.empty())
            hasIslandNav = true;
    }



    dungeonEntrances = level.entrances; //get level entrances from level data
    generateVegetation(); //vegetation checks entrance positions. generate after assinging entrances. 

    generateRaptors(level.raptorCount, level.raptorSpawnCenter, 6000.0f);

    if (levelIndex == 1 || levelIndex == 23){
       generateDactyls(5, level.raptorSpawnCenter, 6000.0f);     
    }

    if (level.name == "Dungeon1"){
        enteredDungeon1 = true;
    }

    if (levelIndex == 0){
        InitNPCs();
    }


    if (level.name == "River") generateTrex(1, level.raptorSpawnCenter, 10000.0f); //generate 1 t-rex on river level. 
    GenerateEntrances();

    InitBoat(player_boat,Vector3{0.0, -75, 0.0});
    InitOverworldWeapons();
    TutorialSetup();
    InitDialogs();

    if (level.isDungeon){
        isDungeon = true;
        drawCeiling = level.hasCeiling;
        LoadDungeonLayout(level.dungeonPath);
        ConvertImageToWalkableGrid(dungeonImg);
        GenerateLightSources(floorHeight);
        GenerateFloorTiles(floorHeight);
        GenerateWallTiles(wallHeight); //model is 400 tall with origin at it's center, so wallHeight is floorHeight + model height/2. 270
        GenerateSecrets(wallHeight);
        BindSecretWallsToRuns(); //assign wallrun index, 

        GenerateInvisibleWalls(floorHeight);
        GenerateDoorways(floorHeight - 20, levelIndex); //calls generate doors from archways
        GenerateLavaSkirtsFromMask(floorHeight);
        GenerateCeilingTiles(ceilingHeight);//400
        GenerateBarrels(floorHeight);
        GenerateLaunchers(floorHeight);
        GenerateSpiderWebs(floorHeight);
        GenerateChests(floorHeight);
        GenerateSwitches(floorHeight);
        GeneratePotions(floorHeight);
        GenerateHarpoon(floorHeight);
        GenerateKeys(floorHeight);
        GenerateWeapons(200);
        GenerateGrapplePoints(floorHeight);

        OpenSecrets();   // set wallRuns[idx] enabled = false, player doesn't collide with disabled wallruns. 
        //generate enemies.
        GenerateSkeletonsFromImage(dungeonEnemyHeight); //165
        GeneratePiratesFromImage(dungeonEnemyHeight);
        GenerateWizardsFromImage(dungeonEnemyHeight);
        GenerateSpiderFromImage(dungeonEnemyHeight);
        GenerateBatsFromImage(dungeonEnemyHeight);
        GenerateGhostsFromImage(dungeonEnemyHeight);
        GenerateGiantSpiderFromImage(dungeonEnemyHeight);
        GenerateSpiderEggFromImage(dungeonEnemyHeight);

        if (levelIndex == 4) levels[0].startPosition = {-5484.34, 180, -5910.67}; //exit dungeon 3 to dungeon enterance 2 position.

        //R.SetLavaShaderValues();
        

        //XZ dynamic lightmap + shader lighting with occlusion
        InitDungeonLights();
        
        miniMap.Initialize(4);
        miniMap.SetDrawSize(288.0f);
    }

    isLoadingLevel = false;
    //R.SetPortalShaderValues();

    R.SetShaderValues();
    InitShaders();
    //R.SetBloomShaderValues();
    if (!isDungeon) R.SetTerrainShaderValues();

    Vector3 resolvedSpawn = ResolveSpawnPoint(level, isDungeon, first, floorHeight);
    InitPlayer(player, resolvedSpawn); //start at green pixel if there is one. otherwise level.startPos or first startPos
    InitWeaponBar();

    hasCrossbow = true;
    player.previousWeapon = WeaponType::Sword;
    player.activeWeapon = WeaponType::Crossbow;
    player.currentWeaponIndex = 0;

    StartFadeInFromBlack();
    levelLoaded = true;


}

static float fadeValue = 0.0;   // 0 = clear, 1 = black
static int queuedLevel = -1;

inline float FadeDt() {
    // Use unpaused time, but cap it to avoid spikes
    float dt = GetFrameTime();               // or your unscaled dt source
    if (dt > 0.05f) dt = 0.05f;              // cap to 50 ms (20 fps) for fades
    return dt;
}


void StartFadeOutFromMenu(int LevelIndex) {
    queuedLevel = levelIndex;
    gFadePhase = FadePhase::FadingOut;
    // don't rely on previous value; clamp explicitly
    fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);   
}


void StartFadeOutToLevel(int levelIndex) {
    queuedLevel = levelIndex;
    gFadePhase = FadePhase::FadingOut;
    // don't rely on previous value; clamp explicitly
    fadeValue = std::clamp(fadeValue, 0.0f, 1.0f);
}

void StartFadeInFromBlack() {
    gFadePhase = FadePhase::FadingIn;
    fadeValue = 1.0f; // start fully black, then tick down
}


// Called every frame before any world/menu update or rendering
void UpdateFade(Camera& camera) {

    const float dt = FadeDt();
    switch (gFadePhase) {
    case FadePhase::FadingOut:
        if (player.position.y > 0) player.canMove = false; //player can still fall into voids, so canMove remains true if below ground level. 

        if (currentGameState == GameState::Menu){
            //it didn't print 0.99 it never reaches 1, clamp? 
            if (fadeValue >= 0.98) {
                pendingLevelIndex = levelIndex;
                gFadePhase = FadePhase::Swapping;
                break;
            }
        }


        if (pendingLevelIndex != -1){ //fade out to next level

            fadeValue = fminf(1.0f, fadeValue + fadeSpeed * dt);
            if (fadeValue >= 1.0f) {
                gFadePhase = FadePhase::Swapping;   // <-- stop here; main loop will do the swap
            }

        }else{ //fade out to spawn point
            fadeValue = fminf(1.0f, fadeValue + fadeSpeed * dt);
            if (fadeValue >= 1.0f) StartFadeInFromBlack();
        }

        break;

    case FadePhase::FadingIn:
        fadeValue = fmaxf(0.0f, fadeValue - fadeSpeed * dt);
        if (fadeValue <= 0.0f){
            gFadePhase = FadePhase::Idle;
            player.canMove = true;

        } 

        break;

    case FadePhase::Swapping:
    case FadePhase::Idle:
        // no-op
        break;
    }

    // Only touch the shader when we are not swapping
    if (gFadePhase != FadePhase::Swapping) {
        Shader& fogShader = R.GetShader("fogShader");
        int loc = GetShaderLocation(fogShader, "fadeToBlack");
        SetShaderValue(fogShader, loc, &fadeValue, SHADER_UNIFORM_FLOAT);
    }
}

void InitOverworldWeapons(){
    Vector3 bpos = {4045.69, 260, -4191.75};
    //worldWeapons.push_back(CollectableWeapon(WeaponType::Crossbow, bpos, R.GetModel("crossbow")));
}


void InitDungeonLights(){
    
    InitDynamicLightmap(dungeonWidth * 4); //128 for 32 pixel map. keep same ratio if bigger map. 
    BuildStaticLightmapOnce(dungeonLights);
    BuildDynamicLightmapFromFrameLights(frameLights); // build dynamic light map once for good luck.

    ResourceManager::Get().SetLightingShaderValues();

}



void DrawEnemyShadows() {
    Model& shadowModel = R.GetModel("shadowQuad");

    // Don’t write to depth, but still test against it
    rlEnableDepthTest();
    rlDisableDepthMask();

    Shader shadowSh = R.GetShader("shadowShader"); // your quad shadow shader
    int locStrength = GetShaderLocation(shadowSh, "shadowStrength");

    float enemyStrength = 0.6f;
    SetShaderValue(shadowSh, locStrength, &enemyStrength, SHADER_UNIFORM_FLOAT);

    for (NPC& npc : gNPCs){
        Vector3 groundPos = {npc.position.x, npc.GetFeetPosY() + 1.0f, npc.position.z};
        DrawModelEx(shadowModel, groundPos, {0,1,0}, 0.0f, {100,100,100}, BLACK);

    }

    for (Character& enemy : enemies) {
        if (enemy.type == CharacterType::Bat) continue; //dont draw shadows for bats

        // Ideally, raycast to ground to get exact Y; add tiny epsilon to avoid z-fighting
        Vector3 groundPos;
        if (isDungeon) {
            groundPos = { enemy.position.x, enemy.position.y - 40.0f, enemy.position.z };
            
        }else{
            float ny = GetHeightAtWorldPosition(enemy.position, heightmap, terrainScale) + 10.0f;
            groundPos = { enemy.position.x, ny, enemy.position.z };
        }
        
        if (enemy.type == CharacterType::Trex) groundPos.y -= 100; //half the frame height? 
        DrawModelEx(shadowModel, groundPos, {0,1,0}, 0.0f, {100,100,100}, BLACK);
    }

    rlEnableDepthMask();
}

void MovePlayerToFreeCam(){
    //Press P to teleport player entitiy to free cam position, and enter player mode. 
    if (CameraSystem::Get().GetMode() == CamMode::Free){
        player.position = CameraSystem::Get().Active().position;
        CameraSystem::Get().SetMode(CamMode::Player);
        CameraSystem::Get().SnapAllToPlayer();
    }

}



void HandleWaves(Camera& camera){
    //water
    // update position (keep your existing waterModel)
    Vector3 waterCenter = { camera.position.x, waterHeightY, camera.position.z };
    Matrix xform = MatrixTranslate(waterCenter.x, waterCenter.y + sinf(GetTime()*0.9f)*0.9f, waterCenter.z);
    R.GetModel("waterModel").transform = xform;


}

void OpenEventLockedDoors(){
    SoundManager::GetInstance().Play("unlock"); //play unlock sound, non positionally so you can here the door unlock.

    for (size_t i = 0; i < doors.size(); i++){
        if (doors[i].eventLocked){
            doors[i].eventLocked = false;
            doors[i].isLocked = false;
            doors[i].isOpen = true; //let the monsters out

            //unlock all evenlocked doors. temporary solution. 
        }
    }
}

void removeAllCharacters(){
    enemies.clear();
    enemyPtrs.clear();

}

// Darkness factor should be in [0.0, 1.0]
// 0.0 = fully dark, 1.0 = fully lit
float CalculateDarknessFactor(Vector3 playerPos, const std::vector<LightSource>& lights) {
    float maxLight = 0.0f;
    const float lightRange = 700.0f;


    for (const LightSource& l : lights) {
        float dist = Vector3Distance(playerPos, l.position);
        float contribution = Clamp(1.0f - dist / l.range, 0.0f, 1.0f) * l.intensity;
        maxLight = fmax(maxLight, contribution);
    }

    for (const LightSample& ls : frameLights){
        float dist = Vector3Distance(playerPos, ls.pos);
        float contribution = Clamp(1.0f - dist / lightRange, 0.0f, 1.0);
        maxLight = fmax(maxLight, contribution);
        
    }

    // Invert to get "darkness"
    float darkness = 1.0f - Clamp(maxLight, 0.0f, 1.0f);
    return darkness;
}



void HandleWeaponTints(){
    if (!isDungeon){
        weaponDarkness = 0.0f;
    }else{
        weaponDarkness = CalculateDarknessFactor(player.position, dungeonLights);
    }


}

void GenerateEntrances() {
    doors.clear();
    doorways.clear();

    for (size_t i = 0; i < dungeonEntrances.size(); ++i) {
        const DungeonEntrance& e = dungeonEntrances[i];

        Door d{};
        d.position = e.position;
        d.rotationY = 0.0f;
        d.doorTexture = R.GetTexture("doorTexture");
        d.isOpen = false;
        d.isLocked = e.isLocked;
        if (i == 2) d.isLocked = !unlockEntrances; //entrance 3 unlocks
        if (i == 0 && levelIndex == 0) d.isLocked = unlockEntrances; //lock entrance 1, prevent player from replaying first 3 levels.
        if (e.position.x == -5484.0f) d.isLocked = true; //entrance 2 always remains locked, make damn sure. 

        d.scale = {300, 365, 1};
        d.tint = WHITE;
        d.linkedLevelIndex = e.linkedLevelIndex;
        d.doorType = DoorType::GoToNext;

        float halfWidth = 200.0f;
        float height    = 365.0f;
        float depth     = 20.0f;
        d.collider = MakeDoorBoundingBox(d.position, d.rotationY, halfWidth, height, depth);
        doors.push_back(d);

        DoorwayInstance dw{};
        dw.position = e.position;
        dw.rotationY = 90.0f * DEG2RAD;
        dw.isOpen = false;
        //dw.isLocked = d.isLocked;
        dw.tint = WHITE;
        doorways.push_back(dw);
    }


}


void generateTrex(int amount, Vector3 centerPos, float radius) {

    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = 1000;
    //try 1000 times to spawn all the raptors either above 80 on heightmap or on empty floor tiles in dungeon. 
    while (spawned < amount && attempts < maxAttempts) {
        ++attempts;

        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float distance = GetRandomValue(500, (int)radius);
        float x = centerPos.x + cosf(angle) * distance;
        float z = centerPos.z + sinf(angle) * distance;

        Vector3 spawnPos = { x, 0.0f, z }; //random x, z  get height diferrently for dungeon
        
        if (isDungeon) {
            // Convert world x,z to dungeon tile coordinates
            const float tileSize = 200.0f; 
            int gridX = (int)(x / tileSize);
            int gridY = (int)(z / tileSize);

            if (!IsDungeonFloorTile(gridX, gridY)) continue; //try again

            float dh = 85.0f;
            float spriteHeight = 200 * 0.5f;
            spawnPos.y = dh + spriteHeight / 2.0f;

        } else {
            float terrainHeight = GetHeightAtWorldPosition(spawnPos, heightmap, terrainScale);
            if (terrainHeight <= 80.0f) continue; //try again

            float spriteHeight = 200 * 0.5f;
            spawnPos.y = terrainHeight + spriteHeight / 2.0f;
        }

        //std::cout << "generated T-Rex\n";
        Character Trex(spawnPos, R.GetTexture("trexSheet"), 300, 300, 1, 0.5f, 1.0, 0, CharacterType::Trex);
        Trex.maxHealth = 2000;
        Trex.currentHealth = Trex.maxHealth;
        enemies.push_back(Trex);
        enemyPtrs.push_back(&enemies.back()); 
        ++spawned;
    }


}

void generateDactyls(int amount, Vector3 centerPos, float radius) {
    //Dactyls on spawn on land. Just like raptors
    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = 1000;
    //try 1000 times to spawn all the raptors either above 80 on heightmap or on empty floor tiles in dungeon. 
    while (spawned < amount && attempts < maxAttempts) {
        ++attempts;

        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float distance = GetRandomValue(500, (int)radius);
        float x = centerPos.x + cosf(angle) * distance;
        float z = centerPos.z + sinf(angle) * distance;

        Vector3 spawnPos = { x, 0.0f, z }; //random x, z  get height diferrently for dungeon
        
        float terrainHeight = GetHeightAtWorldPosition(spawnPos, heightmap, terrainScale);
        //if (terrainHeight <= 80.0f) continue; //try again
        //dactyls can spawn over water

        float spriteHeight = 200 * 0.5f;
        spawnPos.y = terrainHeight + spriteHeight / 2.0f;
    

        Character dactyl(spawnPos, R.GetTexture("dactylSheet"), 512, 512, 1, 0.5f, 0.5f, 0, CharacterType::Pterodactyl);
        dactyl.scale = 0.4;
        dactyl.maxHealth = 100; //one harpoon shot will kill it. otherwise we would have to figure out 3d grapple pull
        dactyl.currentHealth = dactyl.maxHealth;
        dactyl.state == CharacterState::Idle;
        enemies.push_back(dactyl);
        enemyPtrs.push_back(&enemies.back()); 
        ++spawned;
    }

}


void generateRaptors(int amount, Vector3 centerPos, float radius) {

    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = 1000;
    //try 1000 times to spawn all the raptors either above 80 on heightmap or on empty floor tiles in dungeon. 
    while (spawned < amount && attempts < maxAttempts) {
        ++attempts;

        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float distance = GetRandomValue(500, (int)radius);
        float x = centerPos.x + cosf(angle) * distance;
        float z = centerPos.z + sinf(angle) * distance;

        Vector3 spawnPos = { x, 0.0f, z }; //random x, z  get height diferrently for dungeon
        
        if (isDungeon) {
            // Convert world x,z to dungeon tile coordinates
            const float tileSize = 200.0f; 
            int gridX = (int)(x / tileSize);
            int gridY = (int)(z / tileSize);

            if (!IsDungeonFloorTile(gridX, gridY)) continue; //try again

            float dh = 85.0f;
            float spriteHeight = 200 * 0.5f;
            spawnPos.y = dh + spriteHeight / 2.0f;

        } else {
            float terrainHeight = GetHeightAtWorldPosition(spawnPos, heightmap, terrainScale);
            if (terrainHeight <= 80.0f) continue; //try again

            float spriteHeight = 200 * 0.5f;
            spawnPos.y = terrainHeight + spriteHeight / 2.0f;
        }

        //std::cout << "generated raptor\n";

        Character raptor(spawnPos, R.GetTexture("raptorTexture"), 512, 512, 1, 0.5f, 0.5f, 0, CharacterType::Raptor);
        raptor.scale = 0.18;
        raptor.maxHealth = 150;
        raptor.currentHealth = raptor.maxHealth;
        raptor.id++;
        enemies.push_back(raptor);
        enemyPtrs.push_back(&enemies.back()); 
        ++spawned;
    }


}

Character* FindEnemyById(int id)
{
    for (Character* e : enemyPtrs)
    {
        if (!e) continue;
        if (e->id == id)
            return e;
    }
    return nullptr;
}

void UpdateNPCs(float deltaTime){
    for (NPC& npc : gNPCs){
        npc.Update(deltaTime);
        npc.UpdateAnim(deltaTime);
    }

}

void UpdateEnemies(float deltaTime) {
    if (isLoadingLevel) return;
    for (Character& e : enemies){
        e.Update(deltaTime, player);
    }
}

void UpdateMuzzleFlashes(float deltaTime) {
    for (auto& flash : activeMuzzleFlashes) {
        flash.age += deltaTime;
    }

    // Remove expired flashes
    activeMuzzleFlashes.erase(
        std::remove_if(activeMuzzleFlashes.begin(), activeMuzzleFlashes.end(),
                       [](const MuzzleFlash& flash) { return flash.age >= flash.lifetime; }),
        activeMuzzleFlashes.end());

    // Light while any flash is active
    lightConfig.playerIntensity = activeMuzzleFlashes.empty() ? 0.5f : 0.25f;
    lightConfig.playerRadius = activeMuzzleFlashes.empty() ? 400.0f : 1600.0f;
}

void UpdateBullets(Camera& camera, float dt) {
    for (Bullet& b : activeBullets) {
        b.Update(camera, dt);
        if (b.exploded){
            // First frame of death: convert to glow if requested
            if (b.light.active && b.light.detachOnDeath && !b.light.detached) {
                b.light.detached = true;
                b.light.age = 0.f;
                b.light.posWhenDetached = b.GetPosition();  // freeze at death position

            }
            if (b.light.detached) {
                b.light.age += dt;
            }
        }
    }
}

void EraseBullets() {
    activeBullets.erase(
        std::remove_if(activeBullets.begin(), activeBullets.end(),
            [](Bullet& b) { return b.IsDone();}),
        activeBullets.end()
    );
}

// void EraseBullets(){
//     activeBullets.erase( //erase dead bullets. 
//         std::remove_if(activeBullets.begin(), activeBullets.end(),
//                     [](const Bullet& b) { return !b.IsAlive(); }),
//         activeBullets.end());
// }

void UpdateCollectables(float deltaTime) { 
    for (size_t i = 0; i < collectables.size(); i++) {
        collectables[i].Update(deltaTime, player.position);

        // Pickup logic
        if (collectables[i].CheckPickup(player.position, 180.0f)) { //180 radius
            if (collectables[i].type == CollectableType::HealthPotion) {
                player.inventory.AddItem("HealthPotion");
                SoundManager::GetInstance().Play("clink");
            }
            else if (collectables[i].type == CollectableType::GoldKey) {
                player.hasGoldKey = true;
                SoundManager::GetInstance().Play("key");

            } else if (collectables[i].type == CollectableType::SilverKey){
                player.hasSilverKey = true;
                SoundManager::GetInstance().Play("key");
                
            }else if (collectables[i].type == CollectableType::SkeletonKey){
                player.hasSkeletonKey = true;
                SoundManager::GetInstance().Play("key");

            }else if (collectables[i].type == CollectableType::Gold) {
                player.gold += collectables[i].value;
                SoundManager::GetInstance().Play("key");

            } else if (collectables[i].type == CollectableType::ManaPotion) {
                player.inventory.AddItem("ManaPotion");
                SoundManager::GetInstance().Play("clink");
            } else if (collectables[i].type == CollectableType::Harpoon) {
                hasHarpoon = true;
                SoundManager::GetInstance().Play("ratchet");
            }

            collectables.erase(collectables.begin() + i);
            i--;
        }
    }
}

void PlayerSwipeDecal(Camera& camera){
    //swipe decal on melee attack
    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, player.position));
    Vector3 up  = {0.0f, 1.0f, 0.0f};
    // Right = fwd × up  (raylib uses right-handed coords; this gives +X when fwd = (0,0,-1))
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, up));
    // Safety: if looking straight up/down, cross can be tiny — fall back to +X
    if (Vector3Length(right) < 1e-4f) right = {1.0f, 0.0f, 0.0f};
    Vector3 basePos   = Vector3Add(camera.position, Vector3Scale(fwd, 125.0f));  // in front
    Vector3 offsetPos = Vector3Add(basePos,        Vector3Scale(right, 25.0f)); // to the right
    offsetPos.y -= 25;
    Decal decal = {offsetPos, DecalType::MeleeSwipe, R.GetTexture("playerSlashSheet"), 7, 0.35f, 0.05f, 100.0f};

    Vector3 vel = Vector3Add(Vector3Scale(fwd, Vector3Length(player.velocity)), Vector3Scale(right, 0.0f)); //melee swipe decals move forward 
    decal.velocity = vel;
    
    decals.emplace_back(decal);
}


void UpdateDecals(float deltaTime){
    //update decal animation timers
    for (auto& d : decals) {
        d.Update(deltaTime);
        
    }
    decals.erase(std::remove_if(decals.begin(), decals.end(),
                [](const Decal& d) { return !d.alive; }),
                decals.end());
}


void DrawBloodParticles(Camera& camera){
    for (Character* enemy : enemyPtrs) { //draw enemy blood, blood is 3d so draw before billboards. 
            enemy->bloodEmitter.Draw(camera);
    }

    for (SpiderEgg& egg : eggs){
        egg.gooEmitter.Draw(camera);
    }
}

void DrawBullets(Camera& camera) {
    for (const Bullet& b : activeBullets) {
        b.Draw(camera);
    }

}

void DrawOverworldProps() {
    for (const auto& p : levels[levelIndex].overworldProps) {
        
        const char* modelKey =
            (p.type == PropType::Barrel) ? "barrelModel" :
            (p.type == PropType::FirePit)? "campFire": "barrelModel";

        //std::cout << modelKey << "\n";
        Vector3 propPos = p.position;
        float propY = GetHeightAtWorldPosition(propPos, heightmap, terrainScale);
        propPos.y = propY;
        DrawModelEx(R.GetModel(modelKey), propPos,
                    {0,1,0}, p.yawDeg, {p.scale,p.scale,p.scale}, WHITE);
    }
}



Vector3 ResolveSpawnPoint(const LevelData& level, bool isDungeon, bool first, float floorHeight) 
{
    Vector3 resolvedSpawn = level.startPosition; // default fallback

    if (first) {
        resolvedSpawn = {5475.0f, 300.0f, -5665.0f}; // hardcoded spawn for first level
    }

    if (isDungeon) {
        Vector3 pixelSpawn = FindSpawnPoint(dungeonPixels, dungeonWidth, dungeonHeight, tileSize, floorHeight + player.height/2);
        if (pixelSpawn.x != 0 || pixelSpawn.z != 0) {
            resolvedSpawn = pixelSpawn; // green pixel overrides everything
        }
    }

    return resolvedSpawn;
}

void InitNPCs()
{
    gNPCs.clear();
    
    NPC hermit;
    hermit.type = NPCType::Hermit;
    Vector3 hermitStart = {4851.0f, 318.0f, -5552.0f};
    Vector3 hermitFarIsland = {-5815.0f, 304.0f, 6359.0f};

    Vector3 newHermitPos = enteredDungeon1 ? hermitFarIsland : hermitStart;

    hermit.position = newHermitPos;

    hermit.Init(
        R.GetTexture("hermitSheet"), // or hermitTex
        400,                    // frame width
        400,                    // frame height
        0.4f                   // scale (tweak until it feels right)
    );

    // Idle pose (single frame)
    hermit.ResetAnim(
        /*row*/ 0,
        /*start*/ 0,
        /*count*/ 1,
        /*speed*/ 0.05f
    );

    // Interaction
    hermit.interactRadius = 400.0f;
    hermit.dialogId = enteredDungeon1 ? "hermit_2" : "hermit_intro";

    hermit.tint = { 220, 220, 220, 255 }; //darker when not interacting.
    hermit.isInteractable = true;
    hermit.position.y = hermit.GetFeetPosY() + (hermit.frameHeight/2) * hermit.scale;
    gNPCs.push_back(hermit);
}



float GetHeightAtWorldPosition(Vector3 position, Image& heightmap, Vector3 terrainScale) {
    //read heightmap pixels and return the height in world space. 
    int width = heightmap.width;
    int height = heightmap.height;
    unsigned char* pixels = (unsigned char*)heightmap.data;

    // Convert world X/Z into heightmap image coordinates
    float xPercent = (position.x + terrainScale.x / 2.0f) / terrainScale.x;
    float zPercent = (position.z + terrainScale.z / 2.0f) / terrainScale.z;

    // Clamp to valid range
    xPercent = Clamp(xPercent, 0.0f, 1.0f);
    zPercent = Clamp(zPercent, 0.0f, 1.0f);

    // Convert to pixel indices
    int x = (int)(xPercent * (width - 1));
    int z = (int)(zPercent * (height - 1));
    int index = z * width + x;

    // Get grayscale pixel and scale to world height
    float heightValue = (float)pixels[index] / 255.0f;
    return heightValue * terrainScale.y;
}

void DrawReticle(WeaponType& weaponType){
    if (weaponType == WeaponType::Blunderbuss){
        Texture2D tex = R.GetTexture("shotgunReticle");

        float minScale = 0.25f;
        float maxScale = 1.0f;


        player.spreadDegrees = Lerp(player.spreadMinDeg, player.spreadMaxDeg, player.crosshairBloom);

        float scale = Lerp(minScale, maxScale, player.crosshairBloom);



        // ---- Tunables ----
        //const float scale      = 0.33f;                 // change this freely
        const float xBias      = 0.01f;                // your 0.51f -> +1% width bias
        const float yOffsetPct = 0.10f;                // 10% down
        Color tint = { 255, 0, 0, 50 };                // semi-transparent red
        Color islandTint = {255, 0, 0, 75};           //brighter reticle outside. 
        Color minTint = {255, 0, 0, 150};
        // ---- Position (screen space) ----
        float x = GetScreenWidth()  * (0.5f + xBias);
        float y = GetScreenHeight() * 0.5f + GetScreenHeight() * yOffsetPct;

        // ---- Source & destination rects ----
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };

        // Destination rect is centered on (x, y) via origin
        Rectangle dst = { x, y, tex.width * scale, tex.height * scale };
        Vector2 origin = { dst.width * 0.5f, dst.height * 0.5f };

        Color finalTint = (isDungeon) ? tint : islandTint;
        if (scale == minScale) finalTint = minTint;
        DrawTexturePro(tex, src, dst, origin, 0.0f, finalTint);

    }else if (weaponType == WeaponType::Crossbow){
        float yOffset = 100.0f;
        Vector2 ret = {
            GetScreenWidth()  * 0.51f,
            GetScreenHeight() * 0.5f + yOffset //half the screen height
        };
        
        DrawLineV({ret.x - 6, ret.y}, {ret.x + 6, ret.y}, RAYWHITE);
        DrawLineV({ret.x, ret.y - 6}, {ret.x, ret.y + 6}, RAYWHITE);
    }

    
}


void UpdateWorldFrame(float dt, Player& player) {
    // Toggle mode
    if (IsKeyPressed(KEY_TAB) && debugInfo) {
    
        auto m = CameraSystem::Get().GetMode();
        CameraSystem::Get().SetMode(m == CamMode::Player ? CamMode::Free : CamMode::Player);
        CameraSystem::Get().SnapAllToPlayer();
        
    }

    PlayerView pv{
        player.position,
        player.rotation.y,
        player.rotation.x,
        player.isSwimming,
        player.onBoard,
        player_boat.position
        
    };
    CameraSystem::Get().SyncFromPlayer(pv); //synce to players rotation

    // Update camera (handles free vs player internally)
    CameraSystem::Get().Update(dt);

}

void ClearLevel() {
    removeAllCharacters();
    ClearDungeon();
    RemoveAllVegetation();
    activeBullets.clear();
    billboardRequests.clear();
    bulletLights.clear();
    dungeonEntrances.clear();
    
    // if (terrainMesh.vertexCount > 0) UnloadMesh(terrainMesh); //unload mesh and heightmap when switching levels. if they exist
    if (heightmap.data != nullptr) UnloadImage(heightmap); 
    UnloadTerrainGrid(terrain);

    isDungeon = false;
    player.hasGoldKey = false;
    player.hasSilverKey = false;
    player.hasSkeletonKey = false;

}



