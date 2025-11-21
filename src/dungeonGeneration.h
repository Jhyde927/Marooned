#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include "transparentDraw.h"
#include "bullet.h"
#include "player.h"
#include <cstdint>



enum class FloorType {
    Normal,
    Lava,
};

enum class DoorType {
    Normal,
    ExitToPrevious,
    GoToNext,
};

enum class LightType {
    StaticFire,
    Fireball,
    Iceball,
    Other // fallback or future expansion
};

enum class TrapType {
    fireball,
    iceball,

};

struct GridCoord { int x; int y; }; // image-space coords used for worldToGrid

struct SimpleLight { //foward shader light
    Vector3 pos;
    float   radius;
    Color   color;      // 0–255
    float   intensity;  // 0..1
};

struct Fire {
    int fireFrame = 0;
    float fireAnimTimer = 0.0f;
    const float fireFrameDuration = 1.0f / 16.0f; // 16 slightly slower fps to match the bootleg aesthetic
};

struct LightSample {
    Vector3 pos;
    Vector3 color;   // 0..1
    float   range;
    float   intensity;
};


struct DoorwayInstance {
    Vector3 position;
    float rotationY = 0.0f;
    bool isOpen = false;
    bool isLocked = false;
    bool isPortal = false;
    Color tint = GRAY;
    int tileX = 0;
    int tileY = 0;
    Color bakedTint = WHITE;
    float bakedBrightness = 0.0f;
    int linkedLevelIndex = -1;
    std::vector<BoundingBox> sideColliders{};
    bool eventLocked = false;

};

struct Door {
    Vector3 position;
    float rotationY;
    BoundingBox collider;
    std::vector<BoundingBox> sideColliders;
    bool isOpen = false;
    bool isLocked = false;
    bool isPortal = false;
    Texture2D doorTexture;
    Vector3 scale = {100.0f, 200.0f, 1.0f}; // width, height, unused
    Color tint = WHITE;

    int tileX;
    int tileY;
    DoorType doorType = DoorType::Normal;
    bool eventLocked = false;

    int linkedLevelIndex = -1; // -1 means no linked level
};


struct ChestInstance {
    Model model;
    ModelAnimation *animations;
    int animCount;
    Vector3 position;
    Color tint;
    BoundingBox bounds;

    bool open = false;
    bool animPlaying = false;
    float animFrame = 0.0f;
    bool canDrop = true;
};


struct BarrelInstance {
    Vector3 position;
    Color tint = WHITE;
    BoundingBox bounds;
    bool destroyed = false;
    bool containsPotion = false;
    bool containsGold = false;
    bool containsMana = false;
};

struct SpiderWebInstance {
    Vector3 position;
    Color tint = WHITE;
    BoundingBox bounds;
    bool destroyed = false;
    float rotationY;


};

struct PillarInstance {
    Vector3 position;
    float rotation;
    BoundingBox bounds;
};

struct LauncherTrap {
    TrapType type;
    Vector3 position;
    Vector3  direction;   // world-space unit vector on XZ
    float fireIntervalSec;
    float   cooldown;     // time until next shot
    BoundingBox bounds;
};

struct LightSource {
    Vector3 position;
    float intensity = 0.9f;  // 1.0 = full bright, 0.5 = dim, etc.
    float fireballIntensity = 0.8f;
    float range = 1500.0f;
    float lifeTime = 1.0f;
    float age;
    float fireballRange = 600.0f;
    
    Vector3 colorTint = {1.0f, 0.85f, 0.8f}; // default warm
    LightType type = LightType::Other;
};



struct WallInstance {
    Vector3 position;
    float rotationY;
    BoundingBox bounds;
    Color tint = WHITE;  // Default to no tinting
    Color bakedTint;    // Precomputed static lighting
    float bakedBrightness; // NEW! Used only during baking
    Vector3 scale;
};

struct WallRun {
    Vector3 startPos;
    Vector3 endPos;
    float rotationY;
    BoundingBox bounds; // Precomputed, for fast collision checking
};

struct FloorTile {
    Vector3 position;
    Color tint;
    FloorType floorType;

};

struct CeilingTile {
    Vector3 position;
    Color tint;
    Color bakedTint;
    float bakedBrightness;
};

//edge of lava pit
struct Edge {
    Vector3 a, b;   // world-space endpoints ON THE RIM (axis-aligned)
    float   rotY;   // 90 for X edges, 0 for Z edges
    Vector3 inward; // unit normal pointing into the pit
};

extern std::vector<uint8_t> lavaMask;
extern std::vector<LightSample> frameLights;
extern std::vector<LauncherTrap> launchers;
extern std::vector<PillarInstance> pillars;
extern std::vector<Fire> fires;
extern std::vector<SimpleLight> gDungeonLightsForward;
extern std::vector<LightSource> dungeonLights;
extern std::vector<LightSource> bulletLights;
extern std::vector<WallRun> wallRunColliders;
extern std::vector<WallInstance> wallinstances;
extern std::vector<BarrelInstance> barrelInstances;
extern std::vector<SpiderWebInstance> spiderWebs;
extern std::vector<ChestInstance> chestInstances;
extern std::vector<DoorwayInstance> doorways;
extern std::vector<BillboardDrawRequest> billboardRequests;
extern std::vector<Door> doors;



extern Image dungeonImg;
extern Color* dungeonPixels;
extern int dungeonWidth;
extern int dungeonHeight;
extern float playerLightRange;
extern float playerLightIntensity;

void UpdateDungeonChests();
void LoadDungeonLayout(const std::string& imagePath); // Just loads and caches image
void GenerateFloorTiles(float baseY);
void GenerateWallTiles(float baseY);
void GenerateCeilingTiles(float ceilingOffsetY);
void GenerateBarrels(float baseY);
void GenerateSpiderWebs(float baseY);
void GenerateChests(float baseY); 
void GenerateLaunchers(float baseY);
void GenerateLightSources(float baseY);
void GenerateLightSourcesForward(float baseY);
void GenerateDoorways( float baseY, int currentLevelIndex);
void GenerateDoorsFromArchways();
void GeneratePotions(float baseY);
void GenerateKeys(float baseY);
void GenerateLavaSkirtsFromMask(float baseY);
void DrawDungeonFloor();
void DrawDungeonWalls();
void DrawDungeonFloor();
void DrawDungeonBarrels();
void DrawLaunchers();
int Idx(int x, int y); 
void ApplyLavaDPS(Player& player, float dt, float lavaDps);

//void DrawSpiderWebs(Camera& camera);
void DrawDungeonChests(); 
void DrawDungeonPillars();
void DrawDungeonDoorways();
void DrawFlatDoor(Texture2D tex, Vector3 pos, float width, float height, float rotY, Color tint);
void DrawFlatWeb(Texture2D texture, Vector3 position, float width, float height, float rotationY, Color tint);
void GenerateWeapons(float Height);
//void DrawDungeonCeiling(Model ceilingTileModel, float ceilingOffsetY);
void HandleDungeonTints();
void DrawDungeonCeiling();

void UpdateBarrelTints(Vector3 playerPos);
void UpdateChestTints(Vector3 playerPos);
void UpdateDoorTints(Vector3 playerPos);

void UpdateLauncherTraps(float deltaTime);
bool IsDoorOpenAt(int x, int y);
BoundingBox MakeDoorBoundingBox(Vector3 position, float rotationY, float halfWidth, float height, float depth); 
int GetDungeonImageX(float worldX, float tileSize, int dungeonWidth);
int GetDungeonImageY(float worldZ, float tileSize, int dungeonHeight);
bool IsDungeonFloorTile(int x, int y); 
Vector3 GetDungeonWorldPos(int x, int y, float tileSize, float baseY);
Vector3 FindSpawnPoint(Color* pixels, int width, int height, float tileSize, float baseY);
//void GenerateRaptorsFromImage(float baseY);
void GenerateSkeletonsFromImage(float baseY);
void GeneratePiratesFromImage(float baseY);
void GenerateSpiderFromImage(float baseY);
void GenerateGhostsFromImage(float baseY);
void GenerateGiantSpiderFromImage(float baseY);
void GenerateSpiderEggFromImage(float baseY);
void SpawnSpiderFromEgg(Vector3 spawnPos);
Vector3 ColorToNormalized(Color color);
float ColorAverage(Color c);
void GatherFrameLights();
void ClearDungeon();
