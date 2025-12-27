#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include "transparentDraw.h"
#include "bullet.h"
#include "player.h"
#include <cstdint>


enum class KeyType { None, Gold, Silver };

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

struct LightSource {
    Vector3 position;
    Vector3 colorTint;   // 0..1
    float   intensity;
    float   range;

    float   lifeTime;   // dynamic only
    float   age;

    LightType type;
};


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

struct GrapplePoint {
    Vector3 position;        // world-space anchor position
    BoundingBox box;     // bullet collision box
    Texture2D tex;
    float scale;
    // --- Gameplay rules ---
    float   snapRadius;      // how close the aim ray must be to lock on
    float   maxRange;        // max distance player can grapple from
    float   stopDistance;    // how close player stops from anchor
    float   pullSpeed;       // units per second toward anchor
    bool    enabled;         // can this point currently be used

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
    KeyType requiredKey = KeyType::None;

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
    float debugDoorOpenAngleDeg = 0.0f;
    int tileX;
    int tileY;
    DoorType doorType = DoorType::Normal;
    bool eventLocked = false;
    KeyType requiredKey = KeyType::None;

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


struct WallInstance {
    Vector3 position;
    float rotationY;
    BoundingBox bounds;
    Color tint = WHITE;  // Default to no tinting
    Vector3 scale;
};

struct WallRun {
    Vector3 startPos;
    Vector3 endPos;
    float rotationY;
    BoundingBox bounds; // Precomputed, for fast collision checking
    bool enabled = true;
};

struct WindowCollider
{
    Vector3 a;
    Vector3 b;
    float   rotationY;
    BoundingBox bounds;
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

struct SecretWall {
    int x = 0;
    int y = 0;
    Vector3 position;
    float   rotationY = 0.0f;
    BoundingBox tileBounds;      // AABB for that tile
    std::vector<int> wallRunIndices;  // indices into wallRunColliders / wallInstances
    bool    opened   = true;
    bool    discovered = false;
    int wallRunIndex = -1;
    
};

struct InvisibleWall {
    int x = 0;
    int y = 0;
    Vector3 position;
    BoundingBox tileBounds;      // AABB for that tile
    bool enabled = true;

};

extern std::vector<uint8_t> lavaMask;
extern std::vector<uint8_t> voidMask;
extern std::vector<LightSample> frameLights;
extern std::vector<LauncherTrap> launchers;
extern std::vector<PillarInstance> pillars;
extern std::vector<Fire> fires;
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
extern std::vector<SecretWall> secretWalls;
extern std::vector<InvisibleWall> invisibleWalls;
extern std::vector<GrapplePoint> grapplePoints;

extern std::vector<WallInstance>   windowWallInstances;
extern std::vector<WindowCollider> windowColliders;



extern Image dungeonImg;
extern Color* dungeonPixels;
extern int dungeonWidth;
extern int dungeonHeight;
extern float playerLightRange;
extern float playerLightIntensity;

extern size_t gStaticLightCount;  // global or dungeon-level

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
void GenerateDoorways( float baseY, int currentLevelIndex);
void GenerateDoorsFromArchways();
void GeneratePotions(float baseY);
void GenerateHarpoon(float baseY);
void GenerateKeys(float baseY);
void GenerateLavaSkirtsFromMask(float baseY);
void GenerateSecrets(float baseY);
void GenerateInvisibleWalls(float baseY);
void GenerateWindows(float baseY);
void GenerateGrapplePoints(float baseY);
void DebugDrawGrappleBox();
void DrawDungeonBarrels();
void DrawLaunchers();
int Idx(int x, int y); 
void ApplyLavaDPS(Player& player, float dt, float lavaDps);
void UpdateDungeonTileFlags(Player& player, float dt);
void ApplyEnemyLavaDPS();
void DrawDungeonGeometry(Camera& camera, float maxDrawDist);
void DrawSecrets();
void BindSecretWallsToRuns();
void OpenSecrets();
void DrawDungeonChests(); 
void DrawDungeonPillars();
//void DrawFlatDoor(Texture2D tex, Vector3 pos, float width, float height, float rotY, Color tint);
void DrawFlatWeb(Texture2D texture, Vector3 position, float width, float height, float rotationY, Color tint);
void GenerateWeapons(float Height);
//void DrawDungeonCeiling(Model ceilingTileModel, float ceilingOffsetY);
void HandleDungeonTints();
void UpdateBarrelTints(Vector3 playerPos);
void UpdateChestTints(Vector3 playerPos);
void UpdateDoorTints(Vector3 playerPos);

void UpdateLauncherTraps(float deltaTime);
bool IsDoorOpenAt(int x, int y);
BoundingBox MakeDoorBoundingBox(Vector3 position, float rotationY, float halfWidth, float height, float depth); 
BoundingBox MakeWallBoundingBox(const Vector3& start, const Vector3& end,float thickness, float height);
int GetDungeonImageX(float worldX, float tileSize, int dungeonWidth);
int GetDungeonImageY(float worldZ, float tileSize, int dungeonHeight);
bool IsDungeonFloorTile(int x, int y); 
Vector3 GetDungeonWorldPos(int x, int y, float tileSize, float baseY);
Vector3 FindSpawnPoint(Color* pixels, int width, int height, float tileSize, float baseY);

void GenerateSkeletonsFromImage(float baseY);
void GeneratePiratesFromImage(float baseY);
void GenerateSpiderFromImage(float baseY);
void GenerateGhostsFromImage(float baseY);
void GenerateGiantSpiderFromImage(float baseY);
void GenerateSpiderEggFromImage(float baseY);
void SpawnSpiderFromEgg(Vector3 spawnPos);
Vector3 ColorToNormalized(Color color);
float ColorAverage(Color c);

void ClearDungeon();

void DrawFlatDoor(Texture2D tex, Vector3 hinge,float width,float height, float rotYClosed,bool isOpen, Color tint);