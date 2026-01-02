// world.h
#pragma once
#include "raylib.h"
#include "player.h"
#include <vector>
#include "character.h"
#include "bullet.h"
#include "vegetation.h"
#include "decal.h"
#include "level.h"
#include "collectable.h"
#include "collectableWeapon.h"
#include "list"
#include "dungeonGeneration.h"
#include "shadows.h"
#include "miniMap.h"
#include "heightmapPathfinding.h"
#include "main_menu.h"

enum class GameState {
    Menu,
    Playing,
    LoadingLevel,
    Quit
};

// Globals or in a FadeController:
enum class FadePhase { Idle, FadingOut, Swapping, FadingIn };

extern Image heightmap;

extern Vector3 terrainScale;

//gobal vars
extern Player player;
extern Vector3 boatPosition;
extern Vector3 startPosition;
extern Vector3 playerSpawnPoint;

extern Vector3 waterPos;
extern Vector3 bottomPos;

extern MiniMap miniMap;

extern bool controlPlayer;
extern bool isDungeon;
extern float dungeonPlayerHeight; 
extern float floorHeight;
extern float wallHeight;
extern unsigned char* heightmapPixels;
extern float vignetteStrengthValue;
extern float bloomStrengthValue;
extern int selectedOption; // 0 = Start, 1 = Quit
extern int levelIndex;
extern int previousLevelIndex;
extern int pendingLevelIndex;
extern float fadeToBlack;
extern float vignetteIntensity;
extern float vignetteFade;
extern int  vignetteMode;
extern float boatSpeed;
extern float waterHeightY;
extern float ceilingHeight;
extern bool switchFromMenu;
extern float tileSize;

extern bool first;
extern float dungeonEnemyHeight;
extern float ElapsedTime;
extern bool debugInfo;
extern bool isLoadingLevel;
extern float weaponDarkness;
extern bool drawCeiling;
extern bool unlockEntrances;
extern bool playerInit;
extern float fade;
extern bool hasStaff;
extern bool hasBlunderbuss;
extern bool hasCrossbow;
extern bool hasHarpoon;
extern bool drawCeiling;
extern bool levelLoaded;
extern bool isFullscreen;
extern bool hasIslandNav;
extern int gEnemyCounter;
extern int gCurrentLevelIndex; //for resuming game
extern bool quitQued;
//extern float muzzleFlashTimer;



extern GameState currentGameState;
extern FadePhase gFadePhase;
extern MainMenu::State gMenu;
extern HeightmapNavGrid gIslandNav;
extern TreeShadowMask gTreeShadowMask;
extern std::vector<DungeonEntrance> dungeonEntrances;
//extern std::vector<Bullet> activeBullets;
extern std::list<Bullet> activeBullets; // instead of std::vector
extern std::vector<Decal> decals;
extern std::vector<Collectable> collectables;
extern std::vector<MuzzleFlash> activeMuzzleFlashes;
extern std::vector<PreviewInfo> levelPreviews;
extern std::vector<CollectableWeapon> worldWeapons;

extern std::vector<Character> enemies;  
extern std::vector<Character*> enemyPtrs;
Character* FindEnemyById(int id);
void ClearLevel();
void InitLevel(LevelData& level, Camera& camera);
void InitDungeonLights();
void UpdateFade(Camera& camera);
void removeAllCharacters();
void generateRaptors(int amount, Vector3 centerPos, float radius);
void generateDactyls(int amount, Vector3 centerPos, float radius);
void generateTrex(int amount, Vector3 centerPos, float radius);
//void BeginCustom3D(Camera3D camera, float farClip);
void GenerateEntrances();
void HandleWaves(Camera& camera);
void UpdateEnemies(float deltaTime);
void DrawEnemyShadows();
void UpdateMuzzleFlashes(float deltaTime);
void UpdateBullets(Camera& camera, float deltaTime);
void EraseBullets();
float CalculateDarknessFactor(Vector3 playerPos, const std::vector<LightSource>& lights);
void ApplyWeaponTint(Model& weapon, float darkness);
void HandleWeaponTints();
void UpdateDecals(float deltaTime);
void UpdateCollectables(float deltaTime);
void DrawBullets(Camera& camera);
void DrawBloodParticles(Camera& camera);
void DrawOverworldProps();
void DrawReticle(WeaponType& weaponType);
Vector3 ResolveSpawnPoint(const LevelData& level, bool isDungeon, bool first, float floorHeight);
float GetHeightAtWorldPosition(Vector3 position, Image& heightmap, Vector3 terrainScale);
void PlayerSwipeDecal(Camera& camera);
void UpdateWorldFrame(float dt, Player& player);
void StartFadeOutToLevel(int levelIndex);
void StartFadeInFromBlack(); 
void OpenEventLockedDoors();
void InitOverworldWeapons();
void InitMenuLevel(LevelData& level);

void EnterMenu();


