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



GameState currentGameState = GameState::Menu;

//global variables, clean these up somehow. 

Image heightmap;
Vector3 terrainScale = {16000.0f, 200.0f, 16000.0f}; //very large x and z, 

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
float vignetteStrengthValue = 0.2;
float bloomStrengthValue = 0.0;

float fadeSpeed = 1.0f; // units per second

float tileSize = 200;
bool switchFromMenu = false;
int selectedOption = 0;
float floorHeight = 100;
float wallHeight = 270;
float dungeonEnemyHeight = 165;
float ElapsedTime = 0.0f;
bool debugInfo = false;
bool isLoadingLevel = false;
float weaponDarkness = 0.0f;
bool playerInit = false;
bool hasStaff = false;
float fade = 0.0f;
bool isFullscreen = true;

FadePhase gFadePhase = FadePhase::Idle;

//std::vector<Bullet> activeBullets;
std::list<Bullet> activeBullets; // instead of std::vector

std::vector<Decal> decals;
std::vector<MuzzleFlash> activeMuzzleFlashes;
std::vector<Collectable> collectables;
std::vector<CollectableWeapon> worldWeapons; //weapon pickups
std::vector<Character> enemies;  
std::vector<Character*> enemyPtrs;
std::vector<DungeonEntrance> dungeonEntrances;


void InitLevel(LevelData& level, Camera& camera) {
    //Make sure we end texture mode, was causing problems with terrain.
    EndTextureMode();

    isLoadingLevel = true;
    isDungeon = false;
    
    //Called when starting game and changing level. init the level you pass it. the level is chosen by menu or door's linkedLevelIndex. 
    ClearLevel();//clears everything.
    
    camera.position = player.position; //start as player, not freecam.
    levelIndex = level.levelIndex; //update current level index to new level. 

    if (level.heightmapPath.empty() && level.dungeonPath.empty()) {
        TraceLog(LOG_INFO, "Skipping placeholder level index %d", levelIndex);
        level = levels[2];
    }



    heightmap = LoadImage(level.heightmapPath.c_str());
    ImageFormat(&heightmap, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    terrain = BuildTerrainGridFromHeightmap(heightmap, terrainScale, 193, true); //193 bigger chunks less draw calls. 

    dungeonEntrances = level.entrances; //get level entrances from level data
    generateVegetation(); //vegetation checks entrance positions. generate after assinging entrances. 

    generateRaptors(level.raptorCount, level.raptorSpawnCenter, 6000.0f);
    if (level.name == "River") generateTrex(1, level.raptorSpawnCenter, 10000.0f); //generate 1 t-rex on river level. 
    GenerateEntrances();

    InitBoat(player_boat,Vector3{0.0, -75, 0.0});
    TutorialSetup();

    if (level.isDungeon){
        isDungeon = true;
        drawCeiling = level.hasCeiling;
        LoadDungeonLayout(level.dungeonPath);
        ConvertImageToWalkableGrid(dungeonImg);
        GenerateLightSources(floorHeight);
        GenerateFloorTiles(floorHeight);//80
        GenerateWallTiles(wallHeight); //model is 400 tall with origin at it's center, so wallHeight is floorHeight + model height/2. 270
        GenerateDoorways(floorHeight - 20, levelIndex); //calls generate doors from archways
        GenerateLavaSkirtsFromMask(floorHeight);
        GenerateCeilingTiles(ceilingHeight);//400
        GenerateBarrels(floorHeight);
        GenerateLaunchers(floorHeight);
        GenerateSpiderWebs(floorHeight);
        GenerateChests(floorHeight);
        GeneratePotions(floorHeight);
        GenerateKeys(floorHeight);
        GenerateWeapons(200);
        
        
        //generate enemies.
        GenerateSkeletonsFromImage(dungeonEnemyHeight); //165
        GeneratePiratesFromImage(dungeonEnemyHeight);
        GenerateSpiderFromImage(dungeonEnemyHeight);
        GenerateGhostsFromImage(dungeonEnemyHeight);
        GenerateGiantSpiderFromImage(dungeonEnemyHeight);
        GenerateSpiderEggFromImage(dungeonEnemyHeight);

        if (levelIndex == 4) levels[0].startPosition = {-5653, 200, 6073}; //exit dungeon 3 to dungeon enterance 2 position.

        R.SetLavaShaderValues();
        R.SetBloomShaderValues();

        //XZ dynamic lightmap + shader lighting with occlusion
        InitDungeonLights();
    }

    //ResourceManager::Get().SetLightingShaderValues();
    ResourceManager::Get().SetPortalShaderValues();
    isLoadingLevel = false;


    R.SetShaderValues();
    if (!isDungeon) R.SetTerrainShaderValues();
    Vector3 resolvedSpawn = ResolveSpawnPoint(level, isDungeon, first, floorHeight);
    InitPlayer(player, resolvedSpawn); //start at green pixel if there is one. otherwise level.startPos or first startPos

    CameraSystem::Get().SnapAllToPlayer(); //put freecam at player pos

    //start with blunderbus and sword in that order

    player.collectedWeapons = {WeaponType::Blunderbuss, WeaponType::Sword}; 
    if (hasStaff) player.collectedWeapons.push_back(WeaponType::MagicStaff); //once you pick up the staff in world you have it forever. 

    player.activeWeapon = WeaponType::Blunderbuss;
    player.currentWeaponIndex = 0;

    StartFadeInFromBlack();


}

static float fadeValue = 0.0;   // 0 = clear, 1 = black
static int queuedLevel = -1;

inline float FadeDt() {
    // Use unpaused time, but cap it to avoid spikes
    float dt = GetFrameTime();               // or your unscaled dt source
    if (dt > 0.05f) dt = 0.05f;              // cap to 50 ms (20 fps) for fades
    return dt;
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
        player.canMove = false;
        if (pendingLevelIndex != -1){ //fade out to next level
            player.canMove = false;
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

void RebindDynamicLightmapForFrame() {
    // const int DYN_UNIT = 1;
    // rlActiveTextureSlot(DYN_UNIT);
    // rlSetTexture(gDynamic.tex.id);

    Shader lightingShader = R.GetShader("lightingShader");
    int locDynTex = GetShaderLocation(lightingShader, "dynamicGridTex");
    if (locDynTex >= 0) {
        SetShaderValueTexture(lightingShader, locDynTex, gDynamic.tex);
    }
}

void InitDungeonLights(){
    // Reserve texture unit 0 for 2D/UI so the dungeon lightmap
    // will consistently be assigned to a higher texture slot by raylib.
    Texture2D dummyTex = R.GetTexture("blank"); 
    DrawTexture(dummyTex, 0, 0, WHITE);

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

    for (Character& enemy : enemies) {
        // Ideally, raycast to ground to get exact Y; add tiny epsilon to avoid z-fighting
        Vector3 groundPos = { enemy.position.x, enemy.position.y - 40.1f, enemy.position.z };
        if (enemy.type == CharacterType::Trex) groundPos.y -= 100; //half the frame height? 
        DrawModelEx(shadowModel, groundPos, {0,1,0}, 0.0f, {100,100,100}, BLACK);
    }

    rlEnableDepthMask();
}


void HandleWaves(Camera& camera){
    //water
    // update position (keep your existing waterModel)
    Vector3 waterCenter = { camera.position.x, waterHeightY, camera.position.z };
    Matrix xform = MatrixTranslate(waterCenter.x, waterCenter.y + sinf(GetTime()*0.9f)*0.9f, waterCenter.z);
    R.GetModel("waterModel").transform = xform;


}

void OpenEventLockedDoors(){
    //opens the door to next level on spider boss room. we can reuse this to open 1 event locked door per dungeon. 
    //maybe make an event system. save eventLocked doors to a vector? or a map with an id. an event id or something. 
    for (size_t i = 0; i < doors.size(); i++){
        if (doors[i].eventLocked){
            doors[i].eventLocked = false;
            doors[i].isLocked = false;
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
        if (i == 2) d.isLocked = !unlockEntrances;

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

        Character raptor(spawnPos, R.GetTexture("raptorTexture"), 200, 200, 1, 0.5f, 0.5f, 0, CharacterType::Raptor);
        raptor.maxHealth = 200;
        raptor.currentHealth = raptor.maxHealth;
        enemies.push_back(raptor);
        enemyPtrs.push_back(&enemies.back()); 
        ++spawned;
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
    player.lightIntensity = activeMuzzleFlashes.empty() ? 0.5f : 0.25f;
    player.lightRange = activeMuzzleFlashes.empty() ? 400.0f : 1600.0f;
}

void UpdateBullets(Camera& camera, float dt) {
    for (Bullet& b : activeBullets) {
        if (b.IsAlive()) {
            b.Update(camera, dt);
            // (optional) animate b.light.intensity/b.light.range while flying
        } else {
            // First frame of death: convert to glow if requested
            if (b.light.active && b.light.detachOnDeath && !b.light.detached) {
                b.light.detached = true;
                b.light.age = 0.f;
                b.light.posWhenDetached = b.GetPosition();  // freeze at death position
            }
            if (b.light.detached) {
                b.light.age += dt;
                float t = 1.0f - (b.light.age / b.light.lifeTime);
                if (t <= 0.f) {
                    b.light.active = false;      // glow ended
                } else {
                    // optional: b.light.intensity = baseIntensity * t;
                }
            }
        }
    }
}


void EraseBullets(){
    activeBullets.erase( //erase dead bullets. 
        std::remove_if(activeBullets.begin(), activeBullets.end(),
                    [](const Bullet& b) { return !b.IsAlive(); }),
        activeBullets.end());
}

void UpdateCollectables(float deltaTime) { 
    for (size_t i = 0; i < collectables.size(); i++) {
        collectables[i].Update(deltaTime);

        // Pickup logic
        if (collectables[i].CheckPickup(player.position, 180.0f)) { //180 radius
            if (collectables[i].type == CollectableType::HealthPotion) {
                player.inventory.AddItem("HealthPotion");
                SoundManager::GetInstance().Play("clink");
            }
            else if (collectables[i].type == CollectableType::Key) {
                player.inventory.AddItem("GoldKey");
                SoundManager::GetInstance().Play("key");
            }
            else if (collectables[i].type == CollectableType::Gold) {
                player.gold += collectables[i].value;
                SoundManager::GetInstance().Play("key");
            } else if (collectables[i].type == CollectableType::ManaPotion) {
                player.inventory.AddItem("ManaPotion");
                SoundManager::GetInstance().Play("clink");
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
    Vector3 basePos   = Vector3Add(camera.position, Vector3Scale(fwd, 130.0f));  // in front
    Vector3 offsetPos = Vector3Add(basePos,        Vector3Scale(right, 0.0f)); // to the right
    offsetPos.y -= 50;
    Decal decal = {offsetPos, DecalType::MeleeSwipe, R.GetTexture("playerSlashSheet"), 7, 0.35f, 0.05f, 100.0f};

    Vector3 vel = Vector3Add(Vector3Scale(fwd, 0.0f), Vector3Scale(right, 0.0f)); //melee swipe decals move forward 
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
        if (b.IsAlive()){
             b.Draw(camera);
        }
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
        Vector3 pixelSpawn = FindSpawnPoint(dungeonPixels, dungeonWidth, dungeonHeight, tileSize, floorHeight);
        if (pixelSpawn.x != 0 || pixelSpawn.z != 0) {
            resolvedSpawn = pixelSpawn; // green pixel overrides everything
        }
    }

    return resolvedSpawn;
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
    billboardRequests.clear();
    removeAllCharacters();\
    activeBullets.clear();
    ClearDungeon();
    bulletLights.clear();
    dungeonEntrances.clear();

    RemoveAllVegetation();

    // if (terrainMesh.vertexCount > 0) UnloadMesh(terrainMesh); //unload mesh and heightmap when switching levels. if they exist
    if (heightmap.data != nullptr) UnloadImage(heightmap); 
    UnloadTerrainGrid(terrain);
    //UnloadRenderTexture(gTreeShadowMask.rt);

    isDungeon = false;

}



