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


//As above, so below.

int main() { 
    squareRes = false; // set true for 1280x1024, false for widescreen
    showTutorial = true;

    int screenWidth = squareRes ? 1024 : 1600;
    int screenHeight = squareRes ? 1024 : 900;
    //normally start 1600x900 window, toggle fullscreen to fit to monitor.

    //we stopped targeting 60 FPS, so frame rate is uncapped. 
    //Before making a new build, enable vsync so it maches the users monitor.
    SetConfigFlags(FLAG_VSYNC_HINT);

    InitWindow(screenWidth, screenHeight, "Marooned");

    InitAudioDevice();
    //SetTargetFPS(60); //testing

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
    enemies.reserve(100); 



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
            UpdateMenuState(camera, player, deltaTime, ElapsedTime);

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

    // Cleanup
    ClearLevel();
    ResourceManager::Get().UnloadAll();
    SoundManager::GetInstance().UnloadAll();
    CloseAudioDevice();
    CloseWindow();

    //system("pause"); // ← waits for keypress
    return 0;
}
