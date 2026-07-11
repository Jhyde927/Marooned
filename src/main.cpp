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
#include "spawn_manager.h"
#include "world_update.h"
#include "game_settings.h"

//As above, so below.

int main() { 
    //square res and show tutorial are now in game settings. 

    if (GameSettings::useVsync) SetConfigFlags(FLAG_VSYNC_HINT); //disable for uncapped frame rate
    
    int screenWidth = GameSettings::squareRes ? 1024 : 1600; //square resolution for youtube shorts...
    int screenHeight = GameSettings::squareRes ? 1024 : 900;
    //normally start 1600x900 window, toggle fullscreen to fit to monitor.

    InitWindow(screenWidth, screenHeight, "Marooned");

    // Force the first visible frame to be black
    Font titleFont = LoadFont("assets/fonts/PiecesOfEight.ttf");
    BeginDrawing();
    ClearBackground(BLACK);
    Vector2 textPos = Vector2{(float)screenWidth/2 - MeasureText("Loading...", 20)/2, (float)screenHeight/2};
    DrawTextEx(titleFont, "Loading...", textPos, 64, 4, WHITE);
    EndDrawing();

    InitAudioDevice();
    //SetTargetFPS(60); //using vsync instead.

    //linux icon
    Image icon = LoadImage("assets/icon.png");
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(icon);
    UnloadImage(icon);

    SetExitKey(KEY_NULL); //Escape brings up menu, not quit
    ResourceManager::Get().LoadAllResources();
    SoundManager::GetInstance().LoadSounds();
    SoundManager::GetInstance().InitMusic();
    controlPlayer = true; //start as player //hit ~ for debug mode, hit Tab for freecam in debug mode. 

    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float fovy   = (aspect < (16.0f/9.0f)) ? 50.0f : 45.0f; //bump up FOV if it's narrower than 16x9
    Vector3 camPos = {startPosition.x, startPosition.y + 1000, startPosition.z};

    CameraSystem::Get().Init(camPos); 
    CameraSystem::Get().SetFOV(fovy);

    MainMenu::gLevelPreviews = BuildLevelPreviews(true);
    InitMenuLevel(levels[0]);
    levelIndex = LoadLastLevel();

    if (LoadLastLevel() > 0){//dont show preview for middle island. 
        MainMenu::InitLevelPreviewFromSavedLevel();
    }


    //main game loop
    while (!WindowShouldClose()) {
        float rawDt = GetFrameTime();
        ElapsedTime += rawDt;

        float deltaTime = rawDt;
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        Camera3D& camera = CameraSystem::Get().Active();

        UpdateFade(camera, deltaTime);

        if (HandleFadeLevelSwap(camera)) {
            continue;
        }

        UpdateStateTransitions();

        if (currentGameState == GameState::Menu) {
            UpdateMenuState(camera, deltaTime, ElapsedTime);

            if (currentGameState == GameState::Quit)
                break;

            continue;
        }

        if (currentGameState == GameState::Playing) {
            UpdatePlayingFrame(camera, player, deltaTime, ElapsedTime);
            RenderFrame(camera, player, deltaTime);

        }

        if (currentGameState == GameState::Quit)
            break;
    }


    SaveLastLevel(gCurrentLevelIndex); //last level that was actually loaded.

    // Cleanup
    ClearLevel();
    ResourceManager::Get().UnloadAll();
    SoundManager::GetInstance().UnloadAll();
    CloseAudioDevice();
    CloseWindow();

    //system("pause"); // ← waits for keypress
    return 0;
}
