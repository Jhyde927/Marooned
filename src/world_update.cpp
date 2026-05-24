#include "world_update.h"
#include "raylib.h"
#include "world.h"
#include "sound_manager.h"
#include "collisions.h"
#include "lighting.h"
#include "NPC.h"
#include "boat.h"
#include "dungeonGeneration.h"
#include "ui.h"
#include "spiderEgg.h"
#include "input.h"
#include "render_pipeline.h"
#include "shaderSetup.h"
#include "game_settings.h"



void UpdateLevelMusic(){
    if (CurrentLevelIs("Ship"))
    {
        UpdateMusicStream(SoundManager::GetInstance().GetMusic("oceanAmbience"));
    }
    else
    {
        UpdateMusicStream(
            SoundManager::GetInstance().GetMusic(
                isDungeon ? "dungeonAir" : "jungleAmbience"
            )
        );
    }

}

void UpdateMenuState(Camera3D& camera, Player& player, float deltaTime, float elapsedTime)
{
    CameraSystem::Get().Update(deltaTime);

    R.UpdateShaders(camera);
    
    UpdateShadersPerFrame(deltaTime, elapsedTime, camera);

    ShaderSetup::UpdateSkyTransition(deltaTime);

    //ship is a dungeon. 
    if (CurrentLevelIs("Ship") || !isDungeon){
        ShaderSetup::UpdateSkyCycle(deltaTime);
    }


    drawCeiling = false;

    UpdateMenu(camera, deltaTime);

    UpdateLevelMusic();

    if (currentGameState == GameState::Quit)
        return;

    RenderMenuFrame(camera, player, deltaTime);
}


bool HandleFadeLevelSwap(Camera3D& camera)
{
    if (gFadePhase != FadePhase::Swapping)
        return false;

    InitLevel(levels[pendingLevelIndex], camera);
    pendingLevelIndex = -1;

    currentGameState = GameState::Playing;
    gFadePhase = FadePhase::FadingIn;

    return true;
}

void UpdateStateTransitions()
{
    static GameState prevState = currentGameState;

    bool enteredMenu =
        currentGameState == GameState::Menu &&
        prevState != GameState::Menu;

    prevState = currentGameState;

    if (enteredMenu)
    {
        EnterMenu();
    }
}

static void UpdateGameplaySystems(Camera3D& camera, Player& player, float dt)
{
    miniMap.Update(dt, player.position);
    PortalSystem::Update(player.position, player.radius, dt);

    UpdateEnemies(dt);
    UpdateCannons(dt);
    UpdateKraken(dt);
    UpdateNPCs(dt);

    UpdateSlashEffects(dt);
    UpdateBullets(camera, dt);
    GatherFrameLights();
    EraseBullets();

    UpdateDecals(dt);
    UpdateMuzzleFlashes(dt);

    UpdateBoat(player_boat, dt);
    UpdateCollectables(dt);
    UpdateCollectableWeapons(dt);
    UpdatePowerUps(player, dt);

    UpdateLauncherTraps(dt);
    UpdateMonsterDoors(dt);
    SpawnManager::Update(dt);
    UpdateDungeonChests();
    raft.Update(dt);
    UpdateDoorDelayedActions(dt);
    UpdateSpiderEggs(dt, player.position);
    UpdateDungeonTileFlags(player, dt);
    ApplyEnemyLavaDPS();

    if (GameSettings::showTutorial)
        UpdateHintManager(dt);

    UpdateInteractionNPC();
}

static void UpdateGameplayPresentation(Camera3D& camera, Player& player, float dt)
{
    HandleWeaponTints();

    if (isDungeon)
    {
        // if (!debugInfo)
        //     drawCeiling = levels[levelIndex].hasCeiling;

        HandleDungeonTints();
    }

    GatherTransparentDrawRequests(camera, dt);

    controlPlayer = CameraSystem::Get().IsPlayerMode();

    UpdateWorldFrame(dt, player);
    UpdatePlayer(player, dt, camera);

    if (!isLoadingLevel && isDungeon)
        BuildDynamicLightmapFromFrameLights(frameLights);
}

static void UpdateGameplayCollisions(Camera3D& camera)
{
    UpdateCollisions(camera);
    HandleDoorInteraction(camera);
    eraseCharacters();
}

void UpdatePlayingFrame(Camera3D& camera, Player& player, float dt, float elapsedTime)
{
    if (IsKeyPressed(KEY_ESCAPE) && gFadePhase == FadePhase::Idle)
        currentGameState = GameState::Menu;

    //UpdateLevelMusic();

    CameraSystem::Get().Update(dt);
    SoundManager::GetInstance().Update(dt); //update speech.

    UpdateLevelMusic();

    player.godMode = false;
    if (player.dying || CameraSystem::Get().GetMode() == CamMode::Free)
        player.godMode = true;

    UpdateWeaponBarLayoutOnResize();
    debugControls(camera, dt);

    R.UpdateShaders(camera);
    ShaderSetup::UpdateSkyTransition(dt);
    if (!isDungeon) ShaderSetup::UpdateSkyCycle(dt); //day night cycle on island maps only. 
    
    UpdateShadersPerFrame(dt, elapsedTime, camera);

    UpdateGameplaySystems(camera, player, dt);
    UpdateGameplayCollisions(camera);
    UpdateGameplayPresentation(camera, player, dt);
}