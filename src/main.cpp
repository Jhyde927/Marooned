#include "raylib.h"
#include <iostream>
#include "world.h"
#include "input.h"
#include "boat.h"
#include "sound_manager.h"
#include "dungeonGeneration.h"
#include "collisions.h"
#include "ui.h"
#include "resourceManager.h"
#include "render_pipeline.h"
#include "camera_system.h"
#include "lighting.h"
#include "hintManager.h"
#include "rlgl.h"
#include "spiderEgg.h"
#include "miniMap.h"
#include "shaderSetup.h"
#include "portal.h"


//As above, so below.

bool squareRes = false; // set true for 1280x1024, false for widescreen
bool showTutorial = false;

int main() { 
    int screenWidth = squareRes ? 1280 : 1600;
    int screenHeight = squareRes ? 1024 : 900;
    //normally start 1600x900 window, toggle fullscreen to fit to monitor.

    InitWindow(screenWidth, screenHeight, "Marooned");

    InitAudioDevice();
    SetTargetFPS(60);
    //DisableCursor();
    SetExitKey(KEY_NULL); //Escape brings up menu, not quit
    ResourceManager::Get().LoadAllResources();
    SoundManager::GetInstance().LoadSounds();
    
    SoundManager::GetInstance().PlayMusic("dungeonAir");
    SoundManager::GetInstance().PlayMusic("jungleAmbience");
    SetMusicVolume(SoundManager::GetInstance().GetMusic("jungleAmbience"), 0.5f);

    controlPlayer = true; //start as player //hit ~ for debug mode, hit Tab for freecam in debug mode. 

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float fovy   = (aspect < (16.0f/9.0f)) ? 50.0f : 45.0f; //bump up FOV if it's narrower than 16x9
    Vector3 camPos = {startPosition.x, startPosition.y + 1000, startPosition.z};

    CameraSystem::Get().Init(camPos); //init camera to player pos, setting it to cinematic overwrites this. but we still need to init with something. so set it to cinematic then. 
    CameraSystem::Get().SetFOV(fovy);

    MainMenu::gLevelPreviews = BuildLevelPreviews(true);
    InitMenuLevel(levels[0]);

    //main game loop
    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        ElapsedTime += rawDt; //use the same frameTime for both dt and et. 
        float deltaTime = rawDt; // Clamp dt so physics never sees a huge value on a hitch
        if (deltaTime > 0.05f) deltaTime = 0.05f;   
        // Use the active camera everywhere:
        Camera3D& camera = CameraSystem::Get().Active();
        UpdateFade(camera, deltaTime); //always update fade regardless of state



        //Switch Levels
        //UpdateFade switches FadePhase to swapping after fading out completely. while swapping, for 1 frame, switch levels then fade in
        if (gFadePhase == FadePhase::Swapping) {

            InitLevel(levels[pendingLevelIndex], camera);
            pendingLevelIndex = -1;

            currentGameState = GameState::Playing;
            gFadePhase = FadePhase::FadingIn;

            continue; // Block anything from touching old resources this frame. skip the rest of game loop. 
        }

        static GameState prevState = currentGameState;
        bool enteredMenu = (currentGameState == GameState::Menu && prevState != GameState::Menu); //first frame of menu state
        prevState = currentGameState;

        if (enteredMenu) //runs once
        {
            EnterMenu(); //setup cinematic camera for menu view
        }

        //Main Menu - level select - cinematic camera backdrop with menu overlay
        if (currentGameState == GameState::Menu) {
            
            CameraSystem::Get().Update(deltaTime); //update orbit

            //ATTENTION THIS IS THE MENU 
            R.UpdateShaders(camera);
            UpdateShadersPerFrame(deltaTime, ElapsedTime, camera);

            drawCeiling = false; 
            UpdateMenu(camera, deltaTime);//lives in UI.cpp calls main_menu

            UpdateMusicStream(SoundManager::GetInstance().GetMusic(isDungeon ? "dungeonAir" : "jungleAmbience")); //menu level can be dungeon
            if (currentGameState == GameState::Quit) break; //break before we render the next frame. for reasons
            RenderMenuFrame(camera, player, deltaTime); //Render world with menu on top

        
            continue; // skip the rest of the game loop
        }
        if (currentGameState == GameState::Playing){
            //play the game
            if (IsKeyPressed(KEY_ESCAPE) && currentGameState != GameState::Menu) currentGameState = GameState::Menu;
            UpdateMusicStream(SoundManager::GetInstance().GetMusic(isDungeon ? "dungeonAir" : "jungleAmbience"));
            CameraSystem::Get().Update(deltaTime);
            SoundManager::GetInstance().Update(deltaTime); //update hermit speech
            player.godMode = (CameraSystem::Get().GetMode() == CamMode::Free) ? true : false; //player is invincible in freecam 
            //update context
            UpdateWeaponBarLayoutOnResize();
            debugControls(camera, deltaTime); 

            R.UpdateShaders(camera);
            UpdateShadersPerFrame(deltaTime, ElapsedTime, camera);

            miniMap.Update(deltaTime, player.position);
            PortalSystem::Update(player.position, player.radius, deltaTime);
            UpdateEnemies(deltaTime);
            UpdateNPCs(deltaTime);
            UpdateBullets(camera, deltaTime);
            GatherFrameLights();
            EraseBullets();
            UpdateDecals(deltaTime);
            UpdateMuzzleFlashes(deltaTime);
            UpdateBoat(player_boat, deltaTime);
            UpdateCollectables(deltaTime); 
            UpdateLauncherTraps(deltaTime);
            UpdateMonsterDoors(deltaTime);
            UpdateDungeonChests();
            UpdateDoorDelayedActions(deltaTime);
            UpdateSpiderEggs(deltaTime, player.position);
            UpdateDungeonTileFlags(player, deltaTime);
            ApplyEnemyLavaDPS();
            if (showTutorial) UpdateHintManager(deltaTime);
            UpdateInteractionNPC();
            //collisions
            UpdateCollisions(camera);
            HandleDoorInteraction(camera);
            eraseCharacters(); //clean up dead enemies
            HandleWeaponTints();
            if (isDungeon){
                if (!debugInfo) drawCeiling = levels[levelIndex].hasCeiling;
                HandleDungeonTints();
            }

            //gather up everything 2d and put it into a vector of struct drawRequests, then we sort and draw every billboard/quad in the game.
            //sorting is now done by saving to depthmask automatically. alpha cutoff shader handles occlusion problem. legacy code remains. 
            GatherTransparentDrawRequests(camera, deltaTime);
            controlPlayer = CameraSystem::Get().IsPlayerMode();
            // Update camera based on player
            UpdateWorldFrame(deltaTime, player);
            UpdatePlayer(player, deltaTime, camera);

            if (!isLoadingLevel && isDungeon) BuildDynamicLightmapFromFrameLights(frameLights); //update dynamic lights
            RenderFrame(camera, player, deltaTime); //draw everything
            
        }
    }
    // Cleanup
    ClearLevel();
    ResourceManager::Get().UnloadAll();
    SoundManager::GetInstance().UnloadAll();
    CloseAudioDevice();
    CloseWindow();

    //system("pause"); // ← waits for keypress
    return 0;
}
