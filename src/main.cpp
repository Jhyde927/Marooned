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

bool squareRes = false; // set true for 1280x1024, false for widescreen

int main() { 
    int screenWidth = squareRes ? 1280 : 1600;
    int screenHeight = squareRes ? 1024 : 900;
    //normally start 1600x900 window, toggle fullscreen to fit to monitor. 

    // Simply do not generate logs on Linux, everything is visible in the terminal
    // and this code causes a segfault. Probably because of the C way to open files
    // instead of using <fstream> header or buffer overflow
#ifndef linux
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback([](int logLevel, const char* text, va_list args){
        static FILE* f = fopen("marooned_log.txt", "w"); // or "a" to append
        if (!f) return;
        char buf[4096];
        vsnprintf(buf, sizeof(buf), text, args);
        fputs(buf, f);
        fputc('\n', f);
        fflush(f);
    });
#endif



    drawCeiling = true; //debug no ceiling mode. drawCeiling is set by levelData so we can have some dungeons with and without ceilings. 

    InitWindow(screenWidth, screenHeight, "Marooned");

    InitAudioDevice();
    SetTargetFPS(60);
    DisableCursor();
    SetExitKey(KEY_NULL); //Escape brings up menu, not quit
    ResourceManager::Get().LoadAllResources();
    SoundManager::GetInstance().LoadSounds();
    
    SoundManager::GetInstance().PlayMusic("dungeonAir");
    SoundManager::GetInstance().PlayMusic("jungleAmbience");
    SetMusicVolume(SoundManager::GetInstance().GetMusic("jungleAmbience"), 0.5f);

    controlPlayer = true; //start as player //hit ~ for debug mode, hit Tab for freecam in debug mode. 

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float fovy   = (aspect < (16.0f/9.0f)) ? 50.0f : 45.0f; //bump up FOV if it's narrower than 16x9

    CameraSystem::Get().Init(startPosition);
    CameraSystem::Get().SetFOV(fovy);


    
    //main game loop
    while (!WindowShouldClose()) {
        ElapsedTime += GetFrameTime();
        float deltaTime = GetFrameTime();
        
       // Use the active camera everywhere:
        Camera3D& camera = CameraSystem::Get().Active();

        UpdateFade(camera); //always update fade

        //Main Menu - level select 
        if (currentGameState == GameState::Menu) {

            UpdateMenu(camera);
            
            UpdateMusicStream(SoundManager::GetInstance().GetMusic("jungleAmbience"));
            if (switchFromMenu){ //HACK//// make lighting work on level load from door. When game state is menu, only menu code runs,
            //enabling us to cleanly switch levels and lightmaps. 

                InitLevel(levels[pendingLevelIndex], camera);
                pendingLevelIndex = -1;
                
                switchFromMenu = false;
                currentGameState = GameState::Playing;
            } 


      
            //dont draw menu when doing the menu switching hack
            BeginDrawing();

            DrawMenu(selectedOption, levelIndex);

            EndDrawing();


            if (currentGameState == GameState::Quit) break;
            

            continue; // skip the rest of the game loop
        }

        if (IsKeyPressed(KEY_ESCAPE) && currentGameState != GameState::Menu) currentGameState = GameState::Menu;
        UpdateMusicStream(SoundManager::GetInstance().GetMusic(isDungeon ? "dungeonAir" : "jungleAmbience"));

        //update context

        debugControls(camera, deltaTime); 
        R.UpdateShaders(camera);
        UpdateEnemies(deltaTime);
        UpdateBullets(camera, deltaTime);
        GatherFrameLights();
        EraseBullets();
        UpdateDecals(deltaTime);
        UpdateMuzzleFlashes(deltaTime);
        UpdateBoat(player_boat, deltaTime);
        UpdateCollectables(deltaTime); 
        UpdateLauncherTraps(deltaTime);
        UpdateDungeonChests();
        ApplyLavaDPS(player, deltaTime, 1);
        HandleWaves();
        UpdateHintManager(deltaTime);
        
        //collisions
        UpdateCollisions(camera);

        HandleDoorInteraction(camera);
        HandleWeaponTints();
        if (isDungeon){
            
            HandleDungeonTints();
        }

        //gather up everything 2d and put it into a vector of struct drawRequests, then we sort and draw every billboard/quad in the game.
        GatherTransparentDrawRequests(camera, deltaTime);

        controlPlayer = CameraSystem::Get().IsPlayerMode();

        // Update camera based on player
        UpdateWorldFrame(deltaTime, player);
        UpdatePlayer(player, deltaTime, camera);
        
        if (!isLoadingLevel && isDungeon) BuildDynamicLightmapFromFrameLights(frameLights);

        RenderFrame(camera, player, deltaTime); //draw everything
        
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
