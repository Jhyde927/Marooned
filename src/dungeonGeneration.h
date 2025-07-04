#pragma once

#include "raylib.h"
#include <string>
#include <vector>

enum class DoorType {
    Normal,
    ExitToPrevious,
    GoToNext
};

struct Fire {
    int fireFrame = 0;
    float fireAnimTimer = 0.0f;
    const float fireFrameDuration = 1.0f / 16.0f; // 16 slightly slower fps to match the bootleg aesthetic
};


struct DoorwayInstance {
    Vector3 position;
    float rotationY;
    bool isOpen = false;
    bool isLocked = false;
    Color tint = WHITE;
    int tileX;
    int tileY;
    int linkedLevelIndex = -1;
    std::vector<BoundingBox> sideColliders;

};

struct Door {
    Vector3 position;
    float rotationY;
    BoundingBox collider;
    std::vector<BoundingBox> sideColliders;
    bool isOpen = false;
    bool isLocked = false;
    Texture2D* doorTexture;
    Vector3 scale = {100.0f, 200.0f, 1.0f}; // width, height, unused
    Color tint = WHITE;
    int tileX;
    int tileY;
    DoorType doorType = DoorType::Normal;

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
};

struct PillarInstance {
    Vector3 position;
    float rotation;
    BoundingBox bounds;
};

struct LightSource {
    Vector3 position;
    float intensity = 0.7;  // maybe 1.0 = full bright, 0.5 = dim, etc.
    float range = 1500;
    float lifeTime = 1.0f;
    float age;

};



struct WallInstance {
    Vector3 position;
    float rotationY;
    BoundingBox bounds;
    Color tint = WHITE;  // Default to no tinting
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
    Model floorTile;

};

struct CeilingTile {
    Vector3 position;
    Color tint;
};


extern std::vector<PillarInstance> pillars;
extern std::vector<Fire> fires;
extern std::vector<LightSource> dungeonLights;
extern std::vector<LightSource> bulletLights;
extern std::vector<WallRun> wallRunColliders;
extern std::vector<WallInstance> wallinstances;
extern std::vector<BarrelInstance> barrelInstances;
extern std::vector<ChestInstance> chestInstances;
extern std::vector<DoorwayInstance> doorways;
extern std::vector<Door> doors;



extern Image dungeonImg;
extern Color* dungeonPixels;
extern int dungeonWidth;
extern int dungeonHeight;

void InitChests();
void UpdateDungeonChests();
void OpenChest(ChestInstance& chest); 
void LoadDungeonLayout(const std::string& imagePath); // Just loads and caches image
void GenerateFloorTiles(float baseY);
void GenerateWallTiles(float baseY);
void GenerateCeilingTiles(float ceilingOffsetY);
void GenerateBarrels(float baseY);
void GenerateChests(float baseY); 
void GenerateLightSources(float baseY);
void GenerateDoorways( float baseY, int currentLevelIndex);
void GenerateDoorsFromArchways();
void GeneratePotions(float baseY);
void GenerateKeys(float baseY);
void DrawDungeonFloor();
void DrawDungeonWalls(Model wallSegmentModel);
void DrawDungeonFloor(Model floorModel);
void DrawDungeonBarrels();
void DrawDungeonChests(); 
void DrawDungeonPillars(float deltaTime, Camera3D camera);
void DrawDungeonDoorways(Model archwayModel);
void DrawFlatDoor(const Door& door);
//void DrawDungeonCeiling(Model ceilingTileModel, float ceilingOffsetY);
void DrawDungeonCeiling(Model ceilingTileModel);

void UpdateWallTints(Vector3 playerPos);
void UpdateFloorTints(Vector3 playerPos);
void UpdateCeilingTints(Vector3 playerPos);
void UpdateBarrelTints(Vector3 playerPos);
void UpdateChestTints(Vector3 playerPos);
void UpdateDoorTints(Vector3 playerPos);
void UpdateDoorwayTints(Vector3 playerPos);
bool IsDoorOpenAt(int x, int y);
BoundingBox MakeDoorBoundingBox(Vector3 position, float rotationY, float halfWidth, float height, float depth); 
int GetDungeonImageX(float worldX, float tileSize, int dungeonWidth);
int GetDungeonImageY(float worldZ, float tileSize, int dungeonHeight);
bool IsDungeonFloorTile(int x, int y); 
Vector3 GetDungeonWorldPos(int x, int y, float tileSize, float baseY);
Vector3 FindSpawnPoint(Color* pixels, int width, int height, float tileSize, float baseY);
void GenerateRaptorsFromImage(float baseY);
void GenerateSkeletonsFromImage(float baseY);
void GeneratePiratesFromImage(float baseY);
void ClearDungeon();
