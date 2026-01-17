#include "dungeonGeneration.h"
#include "raymath.h"
#include <iostream>
#include "player.h"
#include "vector"
#include "world.h"
#include "rlgl.h"
#include "sound_manager.h"
#include "transparentDraw.h"
#include "pathfinding.h"
#include "resourceManager.h"
#include "utilities.h"
#include "dungeonColors.h"
#include "lighting.h"
#include <cstdint>
#include "spiderEgg.h"
#include "viewCone.h"
#include <algorithm>


std::vector<uint8_t> voidMask;
std::vector<uint8_t> lavaMask; // width*height, 1 = lava, 0 = not

std::vector<LightSample> frameLights;
std::vector<LauncherTrap> launchers;
std::vector<FloorTile> floorTiles;
std::vector<FloorTile> lavaTiles;
std::vector<WallInstance> wallInstances;
std::vector<CeilingTile> ceilingTiles;
std::vector<BarrelInstance> barrelInstances;
std::vector<SpiderWebInstance> spiderWebs;
std::vector<ChestInstance> chestInstances;
std::vector<DoorwayInstance> doorways;
std::vector<Door> doors;
std::vector<SecretWall> secretWalls;
std::vector<PillarInstance> pillars;
std::vector<WallRun> wallRunColliders;
std::vector<InvisibleWall> invisibleWalls;
std::vector<LightSource> dungeonLights; //static lights.
std::vector<GrapplePoint> grapplePoints;
std::vector<WindowCollider> windowColliders;
std::vector<SwitchTile> switches;

std::vector<LightSource> bulletLights; //fireball/iceball
std::vector<Fire> fires;

Image dungeonImg; //save the dungeon info globaly
Color* dungeonPixels = nullptr;
int dungeonWidth = 0;
int dungeonHeight = 0;

float lavaTimer = 0.0f;
float tickDamage = 0.5;

size_t gStaticLightCount = 0; 

using namespace dungeon;

//Dungeon Legend

//   Transparent = Void (no floor tiles)

//⬛ Black = Wall

//⬜ White = Floor (the whole dungeon is filled with floor not just white pixels)

//🟩 Green = Player Start

//🔵 Blue = Barrel

//🟥 Red = Skeleton

//🟨 Yellow = Light

//🟪 Purple = Doorway (128, 0, 128)

//   Teal = Dungeon Exit (0, 128, 128)

//🟧 Orange = Next Level (255, 128, 0)

//   Aqua = locked door (0, 255, 255)

//   Magenta = Pirate (255, 0, 255)

//💗 Pink = health pot (255, 105, 180)

//⭐ Gold = key (255, 200, 0)

//  Sky Blue = Chest (0, 128, 255)

// ⚫ Dark Gray = Spider (64, 64, 64)

//    light gray = spiderWeb (128, 128, 128)

//    Dark Red = magicStaff (128, 0, 0)

//    very light grey = ghost (200, 200, 200)

//    Vermillion = launcherTrap (255, 66, 52)

//    yellowish = direction pixel (200, 200, 0)

//    medium yellow = timing pixel (200, 50, 0)/ (200, 100, 0)/ (200, 150, 0)

// Epsilon for float compares
static inline bool NearlyEq(float a, float b, float eps = 1e-4f) { return fabsf(a - b) < eps; }

Vector3 ColorToNormalized(Color color) {
    return (Vector3){
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f
    };
}

float ColorAverage(Color c) {
    return ((c.r + c.g + c.b) / 3.0f) / 255.0f;
}

void GenerateWeapons(float Height){
    worldWeapons.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current,ColorOf(Code::MagicStaffDarkRed))) { // Dark red staff
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, Height);
                worldWeapons.push_back(CollectableWeapon(WeaponType::MagicStaff, pos, R.GetModel("staffModel")));

            }

            if (EqualsRGB(current, ColorOf(Code::Blunderbuss))) {
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, Height);
                worldWeapons.push_back(CollectableWeapon(WeaponType::Blunderbuss, pos, R.GetModel("blunderbuss")));
            }
        }
    }
}

void GenerateSecrets(float baseY)
{
    secretWalls.clear();

    for (int y = 0; y < dungeonHeight; ++y) {
        for (int x = 0; x < dungeonWidth; ++x) {

            Color c = dungeonPixels[y * dungeonWidth + x];
            if (!EqualsRGB(c, ColorOf(Code::SecretDoor))) continue;

            SecretWall sw;
            sw.position = GetDungeonWorldPos(x, y, tileSize, baseY);

            // Neighbor checks
            bool left  = (x > 0) &&
                IsWallColor(dungeonPixels[y * dungeonWidth + (x - 1)]);
            bool right = (x < dungeonWidth - 1) &&
                IsWallColor(dungeonPixels[y * dungeonWidth + (x + 1)]);
            bool up    = (y > 0) &&
                IsWallColor(dungeonPixels[(y - 1) * dungeonWidth + x]);
            bool down  = (y < dungeonHeight - 1) &&
                IsWallColor(dungeonPixels[(y + 1) * dungeonWidth + x]);

            // Infer orientation
            if ((left || right) && !(up || down)) {
                sw.rotationY = 90.0f;   // horizontal wall run
            }
            else if ((up || down) && !(left || right)) {
                sw.rotationY = 0.0f;    // vertical wall run
            }
            else {
                // Corner or ambiguous — pick one (you like the squeeze anyway)
                sw.rotationY = 0.0f;
            }

            sw.wallRunIndex = -1;
            sw.opened = true; 

            secretWalls.push_back(sw);
        }
    }
}




void OpenSecrets(){
    for (SecretWall& sw : secretWalls){
        if (sw.wallRunIndex >= 0 && sw.wallRunIndex < (int)wallRunColliders.size()) {
            if (sw.opened){
                wallRunColliders[sw.wallRunIndex].enabled = false;

            }
        }
    }
}


void UpdateDungeonChests() {
    
    const int OPEN_END_FRAME = 10;

    for (ChestInstance& chest : chestInstances) {
        float distToPlayer = Vector3Distance(player.position, chest.position);
        if (distToPlayer < 300 && IsKeyPressed(KEY_E) && !chest.open){
            chest.animPlaying = true;
            chest.animFrame = 0.0f;


            SoundManager::GetInstance().Play("chestOpen");
        }

        if (chest.animPlaying) {
            chest.animFrame += GetFrameTime() * 50.0f;

            if (chest.animFrame > OPEN_END_FRAME) {
                chest.animFrame = OPEN_END_FRAME;
                chest.animPlaying = false;
                chest.open = true;
            }

            UpdateModelAnimation(chest.model, chest.animations[0], (int)chest.animFrame);

            
        }
        else if (chest.open && chest.canDrop) { //wait until the animation is finished before dropping the item. 
            chest.canDrop = false;
            UpdateModelAnimation(chest.model, chest.animations[0], OPEN_END_FRAME);
            Vector3 pos = {chest.position.x, chest.position.y + 100, chest.position.z};
            Collectable key(CollectableType::GoldKey, pos, R.GetTexture("keyTexture"), 100);
            
            collectables.push_back(key);
            
        }

    }

}



BoundingBox MakeWallBoundingBox(const Vector3& start, const Vector3& end,
                                float thickness, float height)
{
    Vector3 minv = Vector3Min(start, end);
    Vector3 maxv = Vector3Max(start, end);

    const float halfT = thickness * 0.5f;

    if (NearlyEq(start.x, end.x)) {
        // Edge runs along Z, expand thickness in X
        minv.x -= halfT; maxv.x += halfT;
    } else if (NearlyEq(start.z, end.z)) {
        // Edge runs along X, expand thickness in Z
        minv.z -= halfT; maxv.z += halfT;
    } else {
        // Diagonal edge (shouldn't happen for skirts) — expand in both just in case
        minv.x -= halfT; maxv.x += halfT;
        minv.z -= halfT; maxv.z += halfT;
    }

    maxv.y += height; // if you use this path elsewhere; for pits we'll override Ys below
    return { minv, maxv };
}


bool TileNearSolid(int tx, int tz)
{
    // Check 8-neighborhood around this tile
    for (int dz = -1; dz <= 1; ++dz)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dz == 0) continue;

            int nx = tx + dx;
            int nz = tz + dz;

            // Out of bounds counts as solid (prevents edge leaks)
            if (nx < 0 || nz < 0 || nx >= dungeonWidth || nz >= dungeonHeight){
                return true;
            }


            Color c = dungeonPixels[nz * dungeonWidth + nx];

            // Treat any wall / closed door / opaque tile as solid
            if (EqualsRGB(c, ColorOf(Code::Wall))){
                return true;
            }


            // If you encode doors separately:
            if (EqualsRGB(c, ColorOf(Code::Doorway)))
            {
                int doorIndex = GetDoorIndexAtTile(nx, nz);
                if (doorIndex >= 0 && !doors[doorIndex].isOpen)
                    return true;
            }
        }
    }

    return false;
}



void LoadDungeonLayout(const std::string& imagePath) {
    if (dungeonPixels) {
        UnloadImageColors(dungeonPixels); //erase the previous pixels 
    }

    dungeonImg = LoadImage(imagePath.c_str());
    dungeonPixels = LoadImageColors(dungeonImg);
    dungeonWidth = dungeonImg.width;
    dungeonHeight = dungeonImg.height;


}

Vector3 FindSpawnPoint(Color* pixels, int width, int height, float tileSize, float baseY) {
    //look for the green pixel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color current = pixels[y * width + x];
            if (EqualsRGB(current, ColorOf(Code::PlayerStart))) { //pure green
                return GetDungeonWorldPos(x, y, tileSize, baseY);
            }
        }
    }
    return Vector3{ 0, baseY, 0 }; // fallback if none found
}


int Idx(int x, int y) { return y * dungeonWidth + x; }

bool IsLava(int gx, int gy) {
    return lavaMask[Idx(gx,gy)] != 0;
}

bool IsVoid(int gx, int gy) {
    return voidMask[Idx(gx,gy)] != 0;
}


void GenerateCeilingTiles(float ceilingOffsetY) {
    ceilingTiles.clear();

    //fill the whole dungeon with ceiling tiles. 
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {

            Color pixel = GetImageColor(dungeonImg, x, y);
            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, ceilingHeight);

            // Only skip transparent pixels
            if (pixel.a == 0) continue;

            CeilingTile ceiling;
            ceiling.position = pos;
            ceiling.tint = GRAY;
            ceilingTiles.push_back(ceiling);

        }

    }
}

void GenerateFloorTiles(float baseY) {
    floorTiles.clear();
    lavaTiles.clear();

    voidMask.assign(dungeonWidth * dungeonHeight, 0);
    lavaMask.assign(dungeonWidth * dungeonHeight, 0);

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color pixel = GetImageColor(dungeonImg, x, y);
            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

            // Void Tiles Build Void Mask
            if (pixel.a == 0){
                voidMask[Idx(x,y)] = 1;  //mark as lava on void mask
                continue; 
            } 

            if (pixel.r == 200 && pixel.g == 0 && pixel.b == 0){
                //generate lava tile instead, add to vector of lava tiles. 
                FloorTile lavaTile;
                Vector3 offset = {0, -lavaOffsetY, 0};
                lavaTile.position = pos + offset;
                lavaTile.tint = BLACK;
                lavaTile.floorType = FloorType::Lava;
                lavaTiles.push_back(lavaTile);
                lavaMask[Idx(x,y)] = 1;  //mark as lava on lava mask
                continue;
            }

            

            FloorTile tile;
            tile.position = pos;
            tile.tint = WHITE; 
            tile.floorType = FloorType::Normal;
            floorTiles.push_back(tile);
        }
    }
}

static Vector3 CenterOfBounds(const BoundingBox& bb)
{
    return {
        (bb.min.x + bb.max.x) * 0.5f,
        (bb.min.y + bb.max.y) * 0.5f,
        (bb.min.z + bb.max.z) * 0.5f,
    };
}

void BindSecretWallsToRuns()
{
    for (auto& sw : secretWalls) {
        float bestDist2 = 99999.9;
        int   bestIdx   = -1;

        for (int i = 0; i < (int)wallRunColliders.size(); ++i) {
            auto& run = wallRunColliders[i];

            // Only consider runs with matching orientation
            if (fabsf(run.rotationY - sw.rotationY) > 0.1f) continue;

            Vector3 mid = CenterOfBounds(run.bounds);

            float dx = mid.x - sw.position.x;
            float dz = mid.z - sw.position.z;
            float dist2 = dx*dx + dz*dz;

            if (dist2 < bestDist2) {
                bestDist2 = dist2;
                bestIdx   = i;
            }
        }

        sw.wallRunIndex = bestIdx;
    }
}

int GetDoorIndexAtTile(int nx, int nz){
    for (int i = 0; i < (int)doors.size(); i++){
        if (doors[i].tileX == nx && doors[i].tileY == nz){
            return i;
        }
    }
    return -1;
}



void GenerateInvisibleWalls(float baseY)
{
    invisibleWalls.clear();

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            Color c = dungeonPixels[y * dungeonWidth + x];

            if (!EqualsRGB(c, ColorOf(Code::InvisibleWall)))
                continue;

            InvisibleWall iw;
            iw.x = x;
            iw.y = y;

            // Center of the tile in world space
            iw.position = GetDungeonWorldPos(x, y, tileSize, baseY);

            // Full-tile AABB
            const float half = tileSize * 0.5f;

            iw.tileBounds.min = {
                iw.position.x - half,
                iw.position.y,
                iw.position.z - half
            };

            iw.tileBounds.max = {
                iw.position.x + half,
                iw.position.y + wallHeight * 2,   // same height as normal walls
                iw.position.z + half
            };

            iw.enabled = true;

            invisibleWalls.push_back(iw);
        }
    }
}

static Vector3 CenterOfBoundsXZ(const BoundingBox& b)
{
    return {
        (b.min.x + b.max.x) * 0.5f,
        0.0f,
        (b.min.z + b.max.z) * 0.5f
    };
}



void GenerateWallTiles(float baseY) {
    wallInstances.clear();
    wallRunColliders.clear();

    float wallThickness = 50.0f;
    float wallHeight = 400.0f;

    auto IsSolidWall = [&](Color c) {
        if (c.a == 0) return false;          // transparent → no wall
        if (!IsWallColor(c)) return false;   // not a wall color
        if (IsBarrelColor(c)) return false;  // barrels carve holes out of walls
        
        return true;
    };

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // If this tile is void or not a wall, don’t start any wall segments from it
            if (!IsSolidWall(current)) continue;

            // === Horizontal Pair ===
            if (x < dungeonWidth - 1) {
                Color right = dungeonPixels[y * dungeonWidth + (x + 1)];

                if (IsSolidWall(right)) {
                    Vector3 a = GetDungeonWorldPos(x,     y, tileSize, baseY);
                    Vector3 b = GetDungeonWorldPos(x + 1, y, tileSize, baseY);

                    Vector3 mid = Vector3Lerp(a, b, 0.5f);
                    mid.y = baseY;

                    WallInstance wall;
                    wall.position  = mid;
                    wall.rotationY = 90.0f;
                    wall.tint      = WHITE;
                    wallInstances.push_back(wall);

                    a.y -= 190.0f;
                    b.y -= 190.0f;

                    BoundingBox bounds = MakeWallBoundingBox(a, b, wallThickness, wallHeight);
                    wallRunColliders.push_back({ a, b, 90.0f, bounds });
                }
            }

            // === Vertical Pair ===
            if (y < dungeonHeight - 1) {
                Color down = dungeonPixels[(y + 1) * dungeonWidth + x];

                if (IsSolidWall(down)) {
                    Vector3 a = GetDungeonWorldPos(x, y,     tileSize, baseY);
                    Vector3 b = GetDungeonWorldPos(x, y + 1, tileSize, baseY);

                    Vector3 mid = Vector3Lerp(a, b, 0.5f);
                    mid.y = baseY;

                    WallInstance wall;
                    wall.position  = mid;
                    wall.rotationY = 0.0f;
                    wall.tint      = WHITE;
                    wallInstances.push_back(wall);

                    a.y -= 190.0f;
                    b.y -= 190.0f;

                    BoundingBox bounds = MakeWallBoundingBox(a, b, wallThickness, wallHeight);
                    wallRunColliders.push_back({ a, b, 0.0f, bounds });
                }
            }
        }
    }
}


void GenerateSideColliders(Vector3 pos, float rotationY, DoorwayInstance& archway){
    // 1️⃣ Define local side offset in door-local space
    Vector3 localOffset = { 150, 0, 0 };

    // 2️⃣ Rotate local offset by (rotationY + 90°) so it's perpendicular to the door's opening
    float sideRotation = rotationY + 90.0f * DEG2RAD;
    float cosR = cosf(sideRotation);
    float sinR = sinf(sideRotation);

    Vector3 rotatedOffset = {
        localOffset.x * cosR - localOffset.z * sinR,
        0,
        localOffset.x * sinR + localOffset.z * cosR
    };

    // 3️⃣ Compute world positions for side colliders
    Vector3 leftPos = {
        pos.x - rotatedOffset.x,
        pos.y,
        pos.z - rotatedOffset.z
    };

    Vector3 rightPos = {
        pos.x + rotatedOffset.x,
        pos.y,
        pos.z + rotatedOffset.z
    };

    // 4️⃣ Dimensions for the side walls
    float sideWidth = 50.0f;
    float sideHeight = 400.0f;
    float sideDepth = 50.0f;

    // 5️⃣ Create bounding boxes
    BoundingBox leftBox = MakeDoorBoundingBox(
        leftPos,
        rotationY,
        sideWidth * 0.5f,
        sideHeight,
        sideDepth
    );

    BoundingBox rightBox = MakeDoorBoundingBox(
        rightPos,
        rotationY,
        sideWidth * 0.5f,
        sideHeight,
        sideDepth
    );

    // 6️⃣ Store them
    archway.sideColliders.push_back(leftBox);
    archway.sideColliders.push_back(rightBox);

}

void GenerateDoorways(float baseY, int currentLevelIndex) {
    doorways.clear();

    for (int y = 1; y < dungeonHeight - 1; y++) {
        for (int x = 1; x < dungeonWidth - 1; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            bool isDoor = EqualsRGB(current, ColorOf(Code::Doorway));   // purple
            bool isExit     = EqualsRGB(current, ColorOf(Code::ExitTeal));   // teal
            bool nextLevel =  EqualsRGB(current, ColorOf(Code::NextLevelOrange)); //orange
            bool lockedDoor = EqualsRGB(current, ColorOf(Code::LockedDoorAqua)); //CYAN
            bool silverDoor = EqualsRGB(current, ColorOf(Code::SilverDoor)); //Dark Cyan
            bool skeletonDoor = EqualsRGB(current, ColorOf(Code::SkeletonDoor)); //aged ivory
            bool portal = EqualsRGB(current, ColorOf(Code::DoorPortal)); //portal
            bool window = EqualsRGB(current, ColorOf(Code::WindowedWall));
            bool monsterDoor = EqualsRGB(current, ColorOf(Code::MonsterDoor));
            bool eventLocked = EqualsRGB(current, ColorOf(Code::EventLocked)); //spring-green

            if (!isDoor && !isExit && !nextLevel && !lockedDoor && !portal && !eventLocked && !silverDoor 
                && !window && !monsterDoor && !skeletonDoor) continue;

            // Check surrounding walls to determine door orientation
            Color left = dungeonPixels[y * dungeonWidth + (x - 1)];
            Color right = dungeonPixels[y * dungeonWidth + (x + 1)];
            Color up = dungeonPixels[(y - 1) * dungeonWidth + x];
            Color down = dungeonPixels[(y + 1) * dungeonWidth + x];

            bool wallLeft = left.r == 0 && left.g == 0 && left.b == 0;
            bool wallRight = right.r == 0 && right.g == 0 && right.b == 0;
            bool wallUp = up.r == 0 && up.g == 0 && up.b == 0;
            bool wallDown = down.r == 0 && down.g == 0 && down.b == 0;

            float rotationY = 0.0f;
            if (wallLeft && wallRight) {
                rotationY = 90.0f * DEG2RAD;
            } else if (wallUp && wallDown) {
                rotationY = 0.0f;
            } else {
                continue; // not a valid doorway
            }

            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);
            DoorwayInstance archway = { pos, rotationY, false, false, false, WHITE };
            archway.tileX = x;
            archway.tileY = y;
            if (window) archway.window = true;
            GenerateSideColliders(pos, rotationY, archway);


            if (portal){
                archway.isPortal = true;
                nextLevel = true; //why not portals to the same level
            }

            if (isExit) { //teal
                archway.linkedLevelIndex = previousLevelIndex; //go back outside. 
            }else if (nextLevel){ //orange
                archway.linkedLevelIndex = levels[currentLevelIndex].nextLevel; //door to next level
            }else if (lockedDoor){ //GoldKey door = default locked door
                archway.isLocked = true;
                archway.requiredKey = KeyType::Gold; 

            }else if (silverDoor){ //Dark CYAN
                archway.isLocked = true;
                archway.requiredKey = KeyType::Silver; 

            }else if (skeletonDoor) {
                archway.isLocked = true;
                archway.requiredKey = KeyType::Skeleton;

            } else if (eventLocked) {
                archway.eventLocked = true; //unlock on giant spider death ect..

            }else if (monsterDoor){
                archway.isLocked = true;
                archway.monster = true;

            } else { //purple
                archway.linkedLevelIndex = -1; //regular door
            }
        
            doorways.push_back(archway);
        }
    }

    GenerateDoorsFromArchways();
}

void UpdateMonsterDoors(float deltaTime){
    //update monster door timer
    for (Door& door : doors){

        if (door.isOpen) continue;

        if (door.monsterTriggered){
            door.monsterTimer -= deltaTime;

            if (door.monsterTimer <= 0.0f){
                door.isOpen = true;
                door.isLocked = false;
                door.monsterTimer = 0.0f;
            }
        }


    }
}


void GenerateDoorsFromArchways() {
    doors.clear();

    float halfWidth = 200.0f;   // Half of the 400-unit wide doorway
    float height = 365.0f;
    float depth = 20.0f;        // Thickness into the doorway (forward axis)

    for (DoorwayInstance& dw : doorways) {
        if (dw.isOpen) continue; // skip if this archway should remain open
        if (dw.window){
            //rotate the bounding box by 90
            BoundingBox bb = MakeDoorBoundingBox(dw.position, dw.rotationY + DEG2RAD * 90.0f, halfWidth, height, depth);
            WindowCollider wc = {dw.position, dw.rotationY, bb};
            windowColliders.push_back(wc);
            Door door{}; //dummy door to give side colliders to windows 
            door.sideColliders = dw.sideColliders;
            door.isOpen = false; //side colliders are only active if the door is open. 
            doors.push_back(door);
            
            continue; 
        }
        // Match position/rotation of archway
        Door door{};
        door.position = dw.position;
        door.rotationY = dw.rotationY + DEG2RAD * 90.0f; //offset why? 
        door.isOpen = false;
        door.isPortal = dw.isPortal;
        door.eventLocked = dw.eventLocked;
        door.isLocked = dw.isLocked;
        door.requiredKey = dw.requiredKey;
        door.window = dw.window;
        door.doorTexture = R.GetTexture("doorTexture");
        door.scale = {300, 365, 1}; //stretch it taller
        door.tileX = dw.tileX;
        door.tileY = dw.tileY;
        door.sideColliders = dw.sideColliders; //side colliders for when door is open


        door.collider = MakeDoorBoundingBox(door.position, door.rotationY, halfWidth, height, depth); //covers the whole archway

        if (dw.linkedLevelIndex == previousLevelIndex) {
            door.doorType = DoorType::ExitToPrevious;
        } else if (dw.linkedLevelIndex == levels[levelIndex].nextLevel) {
            door.doorType = DoorType::GoToNext;
            
        }else if (dw.monster){
            door.doorType = DoorType::Monster;
        } else {
            door.doorType = DoorType::Normal;
}

        door.linkedLevelIndex = dw.linkedLevelIndex; //get the level index from the archway

        doors.push_back(door);

    }
}

static constexpr float WALL_MODEL_H        = 320.0f;   // your mesh height
static constexpr float EDGE_LEN            = 200.0f;   // tile size
static constexpr float SKIRT_VIS_THICKNESS = 50.0f;    // used for mesh placement/scaling
static constexpr float SKIRT_COLL_THICKNESS = 1.0f;   // thinner collider
static constexpr float PUSH_OUT_FACTOR     = 0.5f;     // out of pit

BoundingBox MakeAABBFromSkirt(const WallInstance& s, int dir)
{
    // sizes
    const float halfLen    = EDGE_LEN * 0.5f;
    const float halfCollTh = SKIRT_COLL_THICKNESS * 0.5f;
    const float halfY      = (s.scale.y * WALL_MODEL_H) * 0.6f;

    // center (push fully into pit so it never straddles the rim)
    Vector3 outward = (dir==0)? Vector3{-1,0,0}
                    : (dir==1)? Vector3{+1,0,0}
                    : (dir==2)? Vector3{ 0,0,-1}
                               : Vector3{ 0,0,+1};

    // still use the *visual* thickness to decide how far into the pit it sits
    Vector3 c = Vector3Add(s.position, Vector3Scale(outward, SKIRT_VIS_THICKNESS * PUSH_OUT_FACTOR));

    // decide which axis is the long edge from s.rotationY
    float r = fmodf(fabsf(s.rotationY), 180.0f);
    bool longAlongX = (r < 45.0f || r > 135.0f);   // 0°/180° → along X, 90° → along Z

    BoundingBox bb;
    bb.min.y = c.y - halfY;
    bb.max.y = c.y + halfY;

    // flipped this to make it work.
    if (!longAlongX) {               // length on X, thickness on Z
        bb.min.x = c.x - halfLen;    bb.max.x = c.x + halfLen;
        bb.min.z = c.z - halfCollTh; bb.max.z = c.z + halfCollTh;
    } else {                         // length on Z, thickness on X
        bb.min.x = c.x - halfCollTh; bb.max.x = c.x + halfCollTh;
        bb.min.z = c.z - halfLen;    bb.max.z = c.z + halfLen;
    }

    return bb;
}



// Make one vertical liner on edge between (x,y) and its neighbor in DIR
// dir: 0=+X (east), 1=-X (west), 2=+Z (south), 3=-Z (north)
void AddLavaSkirtEdge(int x, int y, int dir, float baseY) {
    // endpoints on the floor plane at the edge between two tiles
    Vector3 a, b;

    switch (dir) {
        case 0: // east edge: (x,y) -> (x+1,y)
            a = GetDungeonWorldPos(x,     y, tileSize, baseY); a.x +=  tileSize*0.5f; a.z -= tileSize*0.5f;
            b = GetDungeonWorldPos(x + 1, y, tileSize, baseY); b.x -=  tileSize*0.5f; b.z += tileSize*0.5f;

            break;
        case 1: // west edge: (x-1,y) -> (x,y)
            a = GetDungeonWorldPos(x - 1, y, tileSize, baseY); a.x +=  tileSize*0.5f; a.z += tileSize*0.5f;
            b = GetDungeonWorldPos(x,     y, tileSize, baseY); b.x -=  tileSize*0.5f; b.z -= tileSize*0.5f;
            break;
        case 2: // south edge: (x,y) -> (x,y+1)
            a = GetDungeonWorldPos(x, y,     tileSize, baseY); a.x += tileSize*0.5f; a.z +=  tileSize*0.5f;
            b = GetDungeonWorldPos(x, y + 1, tileSize, baseY); b.x -= tileSize*0.5f; b.z -=  tileSize*0.5f;
            break;
        default: // north edge: (x,y-1) -> (x,y)
            a = GetDungeonWorldPos(x, y - 1, tileSize, baseY); a.x -= tileSize*0.5f; a.z +=  tileSize*0.5f;
            b = GetDungeonWorldPos(x, y,     tileSize, baseY); b.x += tileSize*0.5f; b.z -=  tileSize*0.5f;
            break;
    }

    const float topY   = baseY + 20.0f;
    const float lavaY  = baseY - 420;
    //const float bTop   = baseY - 50;
    const float height = (topY - lavaY);
    const float WALL_MODEL_HEIGHT = 400.0f; // visual height of your wall model

    //const float t = 50.0f;
    if (height <= 0.0f) return;

    // Instance positioned at mid of the segment, centered vertically
    Vector3 mid = Vector3Lerp(a, b, 0.5f);
    mid.y = lavaY + 0.5f * height;

    float rotY = (dir < 2) ? 0.0f : 90.0f;  // x-edges face 90°, z-edges face 0°
    //float rotY = (dir > 2) ? 0.0f : 90.0f; //flip
    WallInstance skirt{};
    
    skirt.rotationY = rotY;
    skirt.tint      = WHITE;
    skirt.scale     = {1.0f, height / WALL_MODEL_HEIGHT, 1.0f}; 

    // --- how much to overlap/shift ---
    const float lipOut    = tileSize * 0.125f;   // push skirt outward toward the floor side (0..100+)
    const float extraThick = tileSize * 0.25f;  // add thickness so it eats the shelf from both sides

    // --- figure outward normal based on the edge dir (lava -> neighbor) ---
    Vector3 outward = {0,0,0};
    switch (dir) {
        case 0: outward = { -1, 0, 0 }; break; // east edge
        case 1: outward = { +1, 0, 0 }; break; // west edge
        case 2: outward = {  0, 0,-1 }; break; // south edge
        default:outward = {  0, 0,+1 }; break; // north edge
    }

    // 1) PUSH the skirt toward the floor side so it sits under the wall base/ledge
    mid = Vector3Add(mid, Vector3Scale(outward, lipOut));

    // 2) THICKEN the skirt so it actually overlaps the shelf (symmetrically)
    const float WALL_MODEL_THICKNESS = 50.0f; // <-- the source mesh’s native thickness in its local "thickness" axis (measure once)
    float thicknessScale = (WALL_MODEL_THICKNESS + extraThick) / WALL_MODEL_THICKNESS;

    // rotY is already set above. If dir < 2, the edge runs along Z, so thickness axis is X; else thickness axis is Z.
    if (dir < 2) {
        // along Z, so expand X
        skirt.scale = { thicknessScale, height / WALL_MODEL_HEIGHT, 1.0f };
    } else {
        // along X, so expand Z
        skirt.scale = { 1.0f, height / WALL_MODEL_HEIGHT, thicknessScale };
    }

    skirt.position  = mid;

    wallInstances.push_back(skirt);

    BoundingBox bb = MakeAABBFromSkirt(skirt, dir);

    // Store it (rotY not needed for AABB)
    wallRunColliders.push_back({ /*start*/skirt.position, /*end*/skirt.position, 0.0f, bb }); //add colliders to skirts. 
    
}

// --- main pass: walk mask edges and place skirts ---
// lavaMask[y*w + x] == 1 means lava there (you already fill this)
void GenerateLavaSkirtsFromMask(float baseY) {
    for (int y = 0; y < dungeonHeight; ++y) {
        for (int x = 0; x < dungeonWidth; ++x) {
            if (lavaMask[Idx(x,y)] == 0) continue; // only build from lava cells outward

            // For each of 4 neighbors: if neighbor is NOT lava (i.e., floor/solid), add an edge skirt.
            // This guarantees we create one skirt per boundary edge (no duplicates).
            // East
            if (InBounds(x+1, y, dungeonWidth, dungeonHeight) && lavaMask[Idx(x+1,y)] == 0)
                AddLavaSkirtEdge(x, y, 0, baseY);
            // West
            if (InBounds(x-1, y, dungeonWidth, dungeonHeight) && lavaMask[Idx(x-1,y)] == 0)
                AddLavaSkirtEdge(x, y, 1, baseY);
            // South
            if (InBounds(x, y+1, dungeonWidth, dungeonHeight) && lavaMask[Idx(x,y+1)] == 0)
                AddLavaSkirtEdge(x, y, 2, baseY);
            // North
            if (InBounds(x, y-1, dungeonWidth, dungeonHeight) && lavaMask[Idx(x,y-1)] == 0)
                AddLavaSkirtEdge(x, y, 3, baseY);
        }
    }
}


bool IsDoorOpenAt(int x, int y) {
    for (const Door& door : doors) {
        if (door.tileX == x && door.tileY == y) {
            return door.isOpen;
        }
    }
    return true; // If no door is found, assume it's open (or not a real door)
}





void GenerateSpiderWebs(float baseY)
{
    
    spiderWebs.clear();

    for (int y = 1; y < dungeonHeight - 1; y++) {
        for (int x = 1; x < dungeonWidth - 1; x++) {

            Color current = dungeonPixels[y * dungeonWidth + x];
            bool isWeb = (EqualsRGB(current, ColorOf(Code::SpiderWebLightGray)));  // light gray

            if (!isWeb) continue;

            // Check surrounding walls to determine orientation
            Color left = dungeonPixels[y * dungeonWidth + (x - 1)];
            Color right = dungeonPixels[y * dungeonWidth + (x + 1)];
            Color up = dungeonPixels[(y - 1) * dungeonWidth + x];
            Color down = dungeonPixels[(y + 1) * dungeonWidth + x];

            bool wallLeft  = (left.r == 0 && left.g == 0 && left.b == 0);
            bool wallRight = (right.r == 0 && right.g == 0 && right.b == 0);
            bool wallUp    = (up.r == 0 && up.g == 0 && up.b == 0);
            bool wallDown  = (down.r == 0 && down.g == 0 && down.b == 0);

            float rotationY = 0.0f;
            if (wallLeft && wallRight) {
                rotationY = 0;
            } 
            else if (wallUp && wallDown) {
                rotationY = 90.0f * DEG2RAD;
            } 
            else {
                continue;  // not valid web position
            }

            // World position (treat as center of the web)
            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

            // Center it vertically where you want the web midline to be.
            // Example: raise it a bit off the floor but keep it centered.
            pos.y += 200.0f; // tweak to taste

            // Oriented bounding box approximated by an axis-aligned box using half-extents
            const float width      = 150.0f;
            const float thickness  = 40.0f;
            const float height     = 200.0f;

            const float hw = width * 0.5f;
            const float ht = thickness * 0.5f;
            const float hh = height * 0.5f;

            // rotationY: 0 -> facing Z, 90deg -> facing X (your existing logic is fine)
            BoundingBox box;
            if (rotationY == 0.0f) {
                // plane spans X (wide) and Y (tall), thin in Z
                box.min = { pos.x - hw, pos.y - hh, pos.z - ht };
                box.max = { pos.x + hw, pos.y + hh, pos.z + ht };
            } else {
                // plane spans Z (wide) and Y (tall), thin in X
                box.min = { pos.x - ht, pos.y - hh, pos.z - hw };
                box.max = { pos.x + ht, pos.y + hh, pos.z + hw };
}

            // Add to spiderWebs
            spiderWebs.push_back({
                pos,
                WHITE,
                box,
                false,
                rotationY
            });
        }
    }
}



void GenerateLaunchers(float baseY) {
    launchers.clear();
    //fireball launchers are a 3 pixel line. A launcher pixel (vermilion) a direction pixel (yellowish) and a timing pixel (one of 3 medium oranges)
    //The launcher pixel is in the middle, the direction pixel faces the direction the launcher fires, the timing pixel is always opposit the direction.
    //create the launcher and determine the timing and the direction. 

    // Offsets for 4-neighbor (cardinal) directions: left, right, up, down
    const int dx[4] = { -1, 1, 0, 0 };
    const int dy[4] = {  0, 0,-1, 1 };

    for (int y = 0; y < dungeonHeight; ++y) {
        for (int x = 0; x < dungeonWidth; ++x) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Vermilion trap pixel
            //if (!(current.r == 255 && current.g == 66 && current.b == 52)) continue; //if not vermillion try again. 

            TrapType trapType;
    
            if (EqualsRGB(current, ColorOf(Code::LauncherTrapVermillion))) {
                trapType = TrapType::fireball;
            }//(173, 216, 230)
            else if (EqualsRGB(current, ColorOf(Code::IceLauncher))) {
                trapType = TrapType::iceball;
            }
            else {
                continue;
            }
            
            float fireIntervalSec = 3.0f; // default interval
            Vector3 fireDir = {0, 0, 1}; // default forward

            // Find the yellow direction pixel among the 4 neighbors
            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx < 0 || nx >= dungeonWidth || ny < 0 || ny >= dungeonHeight) continue;

                Color neighbor = dungeonPixels[ny * dungeonWidth + nx];
                if (!IsDirPixel(neighbor)) continue;

                // Compute direction in world space directly
                Vector3 trapWorld = GetDungeonWorldPos(x,  y,  tileSize, baseY);
                Vector3 dirWorld  = GetDungeonWorldPos(nx, ny, tileSize, baseY);
                fireDir = Vector3Normalize({
                    dirWorld.x - trapWorld.x,
                    0.0f,  // ignore vertical difference
                    dirWorld.z - trapWorld.z
                });

                // Timing pixel is on the *opposite* side: (x - dx[i], y - dy[i])
                int tx = x - dx[i], ty = y - dy[i];
                if (tx >= 0 && tx < dungeonWidth && ty >= 0 && ty < dungeonHeight) {
                    Color timing = dungeonPixels[ty * dungeonWidth + tx];
                    if (IsTimingPixel(timing)) {
                        fireIntervalSec = TimingFromPixel(timing);
                    }
                }


                break;
            }

            // Build trap
            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

            float halfSize = 100.0f;
            BoundingBox box;
            box.min = { pos.x - halfSize, pos.y,          pos.z - halfSize };
            box.max = { pos.x + halfSize, pos.y + 100.0f, pos.z + halfSize };

            launchers.push_back({ trapType, pos, fireDir, fireIntervalSec, 0.0f, box });
        }
    }
}




void GenerateBarrels(float baseY) {
    barrelInstances.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::Barrel))) { // Blue = Barrel
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

                // Define bounding box as 100x100x100 cube centered on pos, tileSize is 200 so half tile size centered. 
                float halfSize = 50.0f;
                BoundingBox box;
                box.min = {
                    pos.x - halfSize,
                    pos.y,
                    pos.z - halfSize
                };
                box.max = {
                    pos.x + halfSize,
                    pos.y + 250.0f,
                    pos.z + halfSize
                };
                //Decide what the barrel will drop. 
                int roll = GetRandomValue(0, 99);
                bool willContainPotion = false;
                bool willContainMana = false;
                bool willContainGold = false;
                //barrels only drop one thing at a time. 
                if (roll < 25) {
                    willContainPotion = true;     // 0 - 24 → 25%
                } else if (roll < 35) {
                    willContainMana = true;       // 25 - 34 → 10%
                } else if (roll < 85) {
                    willContainGold = true;       // 35 - 84 → 50%
                }
                // 85 - 99 → 15% chance barrel contains nothing
                
                barrelInstances.push_back({
                    pos,
                    WHITE,
                    box,
                    false,
                    willContainPotion,
                    willContainGold,
                    willContainMana,
                    
                });
                
            }
        }
    }
}

static int CountOrthogonalNeighborsWithColor(
    const Color* pixels,
    int w, int h,
    int x, int y,
    Color target)
{
    auto InBounds = [&](int ix, int iy) -> bool {
        return (ix >= 0 && ix < w && iy >= 0 && iy < h);
    };

    int count = 0;

    // N
    if (InBounds(x, y - 1)) {
        Color c = pixels[(y - 1) * w + x];
        if (EqualsRGB(c, target)) count++;
    }
    // E
    if (InBounds(x + 1, y)) {
        Color c = pixels[y * w + (x + 1)];
        if (EqualsRGB(c, target)) count++;
    }
    // S
    if (InBounds(x, y + 1)) {
        Color c = pixels[(y + 1) * w + x];
        if (EqualsRGB(c, target)) count++;
    }
    // W
    if (InBounds(x - 1, y)) {
        Color c = pixels[y * w + (x - 1)];
        if (EqualsRGB(c, target)) count++;
    }

    return count;
}

static LockType LockTypeFromSwitchNeighborCount(int n)
{
    if (n <= 0) return LockType::Gold;
    if (n == 1) return LockType::Silver;
    if (n == 2) return LockType::Skeleton;
    return LockType::Event; // 3 or 4
}



void GenerateSwitches(float baseY)
{
    switches.clear();

    const Color cSwitch   = ColorOf(Code::Switch);   // Ash Gray
    const Color cSwitchID = ColorOf(Code::switchID); // Payne's Green (0,102,85)

    for (int y = 0; y < dungeonHeight; y++)
    {
        for (int x = 0; x < dungeonWidth; x++)
        {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (!EqualsRGB(current, cSwitch))
                continue;

            // --- decode lock type from adjacent SwitchID pixels ---
            const int idCount = CountOrthogonalNeighborsWithColor(
                dungeonPixels,
                dungeonWidth, dungeonHeight,
                x, y,
                cSwitchID
            );

            SwitchTile st = {};
            st.position  = GetDungeonWorldPos(x, y, tileSize, baseY);
            st.triggered = false;
            st.oneShot   = true;
            st.lockType  = LockTypeFromSwitchNeighborCount(idCount);

            // --- bounding box: 200x200x200 cube centered on pos (y from base to base+200) ---
            const float halfSize = 100.0f;
            st.box.min = { st.position.x - halfSize, st.position.y,         st.position.z - halfSize };
            st.box.max = { st.position.x + halfSize, st.position.y + 200.0f, st.position.z + halfSize };

            switches.push_back(st);
        }
    }
}



// void GenerateSwitches(float baseY) {
//     switches.clear();

//     for (int y = 0; y < dungeonHeight; y++) {
//         for (int x = 0; x < dungeonWidth; x++) {
//             Color current = dungeonPixels[y * dungeonWidth + x];

//             if (EqualsRGB(current, ColorOf(Code::Switch))) { // Ash Gray switch tile
//                 Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

//                 // Define bounding box as 200x200x200 cube centered on pos
//                 float halfSize = 100.0f;
//                 BoundingBox box;
//                 box.min = {
//                     pos.x - halfSize,
//                     pos.y,
//                     pos.z - halfSize
//                 };
//                 box.max = {
//                     pos.x + halfSize,
//                     pos.y + 200.0f,
//                     pos.z + halfSize
//                 };

//                 SwitchTile st = {
//                     pos, 
//                     box,
//                     false,
//                     true
//                 };

//                 switches.push_back(st);


//             }
//         }
//     }
// }

void GenerateChests(float baseY) {
    chestInstances.clear();
    int chestID = 0;
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::ChestSkyBlue))) { // SkyBlue = chest
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

                // Define bounding box as 100x100x100 cube centered on pos
                float halfSize = 10.0f;
                BoundingBox box;
                box.min = {
                    pos.x - halfSize,
                    pos.y,
                    pos.z - halfSize
                };
                box.max = {
                    pos.x + halfSize,
                    pos.y + 100.0f,
                    pos.z + halfSize
                };

                                // create a unique key for this chest model
                std::string key = "chestModel#" + std::to_string(chestID++);

                // load a _separate_ model for this chest
                // (this reads the same GLB but gives you independent skeleton data)
                R.LoadModel(key, "assets/models/chest.glb");
                Model& model = R.GetModel(key);

                int animCount = 0;
                ModelAnimation *anims = LoadModelAnimations("assets/models/chest.glb", &animCount);

                ChestInstance chest = {
                    model,
                    anims,
                    animCount,
                    pos,
                    WHITE,
                    box,
                    false, // open
                    false, // animPlaying
                    0.0f   // animFrame
                };

                chestInstances.push_back(chest);
                
            }
        }
    }
}

void GenerateHarpoon(float baseY){
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::Harpoon))) { // gun metal
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 50); // raised slightly off floor
                Collectable p = {CollectableType::Harpoon, pos, R.GetTexture("harpoon"), 80};
                collectables.push_back(p);
            }
        }
    }

}

static BoundingBox MakeBoxCentered(Vector3 p, Vector3 half)
{
    BoundingBox b;
    b.min = (Vector3){ p.x - half.x, p.y - half.y, p.z - half.z };
    b.max = (Vector3){ p.x + half.x, p.y + half.y, p.z + half.z };
    return b;
}

void GenerateGrapplePoints(float baseY)
{
    grapplePoints.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {

            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::GrapplePoint))) { // steel blue

                Vector3 half = { 30.0f, 30.0f, 30.0f }; // tweak this (width/height/depth)
                Vector3 pos = GetDungeonWorldPos(
                    x,
                    y,
                    tileSize,
                    baseY + 50.0f   // anchor height offset
                );

                GrapplePoint gp;
                gp.position     = pos;
                gp.box          = MakeBoxCentered(pos, half);
                gp.tex          = R.GetTexture("grapplePoint");
                gp.snapRadius   = 120.0f;
                gp.maxRange     = 4000.0f; 
                gp.stopDistance = 100.0f;
                gp.pullSpeed    = 6000.0f;
                gp.enabled      = true;
                gp.scale        = 50.0f;
                grapplePoints.push_back(gp);
            }
        }
    }
}



void GeneratePotions(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::HealthPotPink))) { // pink for potions
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 50); // raised slightly off floor
                Collectable p = {CollectableType::HealthPotion, pos, R.GetTexture("healthPotTexture"), 40};
                collectables.push_back(p);
            }

            if (EqualsRGB(current, ColorOf(Code::ManaPotion))) { // dark blue for mana potions
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 50); // raised slightly off floor
                Collectable p = {CollectableType::ManaPotion, pos, R.GetTexture("manaPotion"), 40};
                collectables.push_back(p);
            }

        }
    }
}

void GenerateKeys(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (EqualsRGB(current, ColorOf(Code::KeyGold))) { // Gold for keys
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 80); // raised slightly off floor
                Collectable key = {CollectableType::GoldKey, pos, R.GetTexture("keyTexture"), 100.0f};
                collectables.push_back(key);
            }

            if (EqualsRGB(current, ColorOf(Code::SilverKey))) { // Cool Silver for silver keys
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 80); // raised slightly off floor
                Collectable key = {CollectableType::SilverKey, pos, R.GetTexture("silverKey"), 100.0f};
                collectables.push_back(key);
            }

            if (EqualsRGB(current, ColorOf(Code::SkeletonKey))) { // aged ivory for bone key
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 80); // raised slightly off floor
                Collectable key = {CollectableType::SkeletonKey, pos, R.GetTexture("skeletonKey"), 100.0f};
                collectables.push_back(key);
            }           
        }
    }
}


void GenerateBatsFromImage(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for pure red pixels (255, 0, 0) → Skeleton spawn
            if (EqualsRGB(current, ColorOf(Code::Bat))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character bat(
                    spawnPos,
                    R.GetTexture("batSheet"), 
                    200, 200,         // frame width, height
                    1,                // max frames
                    0.5f, 0.4f,       // speed, scale 
                    0,                // initial animation frame
                    CharacterType::Bat
                );
                bat.maxHealth = 75;
                bat.currentHealth = 75; //1.5 sword attacks
                bat.id = gEnemyCounter++;
                bat.bobPhase = Rand01() * 2.0f * PI; //random starting offset

                enemies.push_back(bat);
                enemyPtrs.push_back(&enemies.back()); 
            }
        }
    }

}


void GenerateSpiderFromImage(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for pure red pixels (255, 0, 0) → Skeleton spawn
            if (EqualsRGB(current, ColorOf(Code::SpiderDarkGray))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character spider(
                    spawnPos,
                    R.GetTexture("spiderSheet"), 
                    200, 200,         // frame width, height
                    1,                // max frames
                    0.5f, 0.5f,       // scale, speed
                    0,                // initial animation frame
                    CharacterType::Spider
                );
                spider.maxHealth = 100;
                spider.currentHealth = 100; //2 sword attacks
                spider.id = gEnemyCounter++;
                enemies.push_back(spider);
                enemyPtrs.push_back(&enemies.back()); 
            }
        }
    }

}



void SpawnSpiderFromEgg(Vector3 spawnPos){
    Character spider(
        spawnPos,
        R.GetTexture("spiderSheet"), 
        200, 200,         // frame width, height
        1,                // max frames
        0.5f, 0.5f,       // scale, speed
        0,                // initial animation frame
        CharacterType::Spider
    );
    spider.maxHealth = 100;
    spider.currentHealth = 100; //2 sword attacks
    
    enemies.push_back(spider);
    enemyPtrs.push_back(&enemies.back()); 

}

void GenerateSpiderEggFromImage(float baseY) {
    for (int y = 0; y < dungeonHeight; ++y) {
        for (int x = 0; x < dungeonWidth; ++x) {

            Color c = dungeonPixels[y * dungeonWidth + x];

            bool isEgg = (c.r == 128 && c.g == 255 && c.b == 0); // Slime Green

            if (isEgg) {
                Vector3 worldPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                // Get the egg sprite sheet from your ResourceManager
                Texture2D& eggTex = R.GetTexture("spiderEggSheet");

                // These numbers depend on how you lay out the sprite sheet
                int frameW       = 200;
                int frameH       = 200;
                int framesPerRow = 3;
                float scale      = 0.5f;

                SpiderEgg& egg = SpawnSpiderEgg(worldPos, eggTex, frameW, frameH, framesPerRow, scale);

                // Optional tuning per-egg:

                egg.maxHealth    = 100.0f;
                egg.health       = egg.maxHealth;
                egg.rowDormant   = 0;
                egg.rowHatching  = 1;
                egg.rowHusk      = 2;
                egg.rowDestroyed = 3;
            }
        }
    }

}

void GenerateGhostsFromImage(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (current.a == 0) continue;
            
            if (EqualsRGB(current, ColorOf(Code::GhostVeryLightGray))) { //very light gray = ghost. 
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character ghost(
                    spawnPos,
                    R.GetTexture("ghostSheet"), 
                    200, 200,         // frame width, height
                    1,                // max frames
                    0.8f, 0.5f,       // scale, speed
                    0,                // initial animation frame
                    CharacterType::Ghost
                );
                ghost.maxHealth = 200;
                ghost.currentHealth = 200; 

                enemies.push_back(ghost);
                enemyPtrs.push_back(&enemies.back()); 

            }
        }
    }


}

void GenerateGiantSpiderFromImage(float baseY) {

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for pure red pixels (255, 0, 0) → Giant Spider
            if (EqualsRGB(current, ColorOf(Code::GiantSpider))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character giantSpider(
                    spawnPos,
                    R.GetTexture("GiantSpiderSheet"), 
                    300, 300,         // frame width, height //any bigger he would clip walls on 1x1 hallways
                    1,                // max frames
                    1.0f, 0.5f,       // speed, scale //scalling to anything other than 0.5 makes the animation not line up for mysterious reasons. 
                    0,                // initial animation frame
                    CharacterType::GiantSpider
                );
                giantSpider.maxHealth = 2000; //3k was to much, try 2k
                giantSpider.currentHealth = giantSpider.maxHealth; 
                giantSpider.id = gEnemyCounter++;
                enemies.push_back(giantSpider);
                enemyPtrs.push_back(&enemies.back()); 
            }
        }
    }


}

void GenerateSkeletonsFromImage(float baseY) {


    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for pure red pixels (255, 0, 0) → Skeleton spawn
            if (EqualsRGB(current, ColorOf(Code::Skeleton))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);
                
                Character skeleton(
                    spawnPos,
                    R.GetTexture("skeletonSheet"), 
                    200, 200,         // frame width, height
                    1,                // max frames
                    0.8f, 0.5f,       // scale, speed
                    0,                // initial animation frame
                    CharacterType::Skeleton
                );
                skeleton.maxHealth = 200;
                skeleton.currentHealth = 200; //at least 2 shots. 4 sword swings 
                skeleton.id = gEnemyCounter++;
                
                enemies.push_back(skeleton);
                enemyPtrs.push_back(&enemies.back()); 
            }
        }
    }


}

void GeneratePiratesFromImage(float baseY) {


    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for magenta pixels (255, 0, 255) → Pirate spawn
            if (EqualsRGB(current, ColorOf(Code::PirateMagenta))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character pirate(
                    spawnPos,
                    R.GetTexture("pirateSheet"), 
                    200, 200,         // frame width, height 
                    1,                // max frames, set when setting animations
                    0.5f, 0.5f,       // scale, speed
                    0,                // initial animation frame
                    CharacterType::Pirate
                );
                

                pirate.maxHealth = 400; // twice as tough as skeletons, at least 3 shots. 8 slices.
                pirate.currentHealth = 400;
                pirate.id = gEnemyCounter++;
                enemies.push_back(pirate);
                enemyPtrs.push_back(&enemies.back()); 

            }
        }
    }

}

void GenerateWizardsFromImage(float baseY) {


    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for magenta pixels (148, 0, 211) → wizard spawn
            if (EqualsRGB(current, ColorOf(Code::Wizard))) {
                Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

                Character wizard(
                    spawnPos,
                    R.GetTexture("wizardSheet"), 
                    400, 400,         // frame width, height 
                    1,                // max frames, set when setting animations
                    0.25f, 0.35f,       // speed, scale 
                    0,                // initial animation frame
                    CharacterType::Wizard
                );
                

                wizard.maxHealth = 400; // twice as tough as skeletons, at least 3 shots. 8 slices.
                wizard.currentHealth = 400;
                wizard.id = gEnemyCounter++;
                enemies.push_back(wizard);
                enemyPtrs.push_back(&enemies.back()); 

            }
        }
    }

}

void GenerateLightSources(float baseY) {
    dungeonLights.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];
            
            // Check for yellow (pure R + G, no B)
            if (EqualsRGB(current, ColorOf(Code::Light))) {
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);
                LightSource L = MakeStaticTorch(pos);
                dungeonLights.push_back(L);

                // Create a 100x100x100 bounding box centered on pos
                BoundingBox box;
                box.min = Vector3Subtract(pos, Vector3{50.0f, 0.0f, 50.0f});
                box.max = Vector3Add(pos, Vector3{50.0f, 200.0f, 50.0f});

                pillars.push_back({ pos, 1.0f, box });

                Fire newFire;
                newFire.fireFrame = GetRandomValue(0, 59);
                fires.push_back(newFire);
            }

        }
    }
    //Invisible light sources
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];
            
            // Check for light yellow 
            if (EqualsRGB(current,ColorOf(Code::InvisibleLight))) {
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);
                LightSource L = MakeStaticTorch(pos);

                if (levelIndex == 21){ //blue lights on ice level
                    L.colorTint = Vector3{0.0, 0.0, 1.0};
                }
                dungeonLights.push_back(L);
            }

        }
    }
    //Lava glow
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];
            
            // Check for FireBrick red
            if (EqualsRGB(current,ColorOf(Code::LavaGlow))){
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);
                LightSource L = MakeStaticTorch(pos);
                L.colorTint = Vector3 {1, 0, 0}; // 0..1
                dungeonLights.push_back(L);
            }

        }
    }

}


// Returns true if world point is inside the dungeon bounds.
// world → image grid (with your X/Z flips baked in)
inline bool WorldToGrid(const Vector3& worldPos,
                        GridCoord& out,
                        float tileSize,
                        int dungeonWidth, int dungeonHeight,
                        Vector3 dungeonOrigin = {0,0,0})
{
    // Convert world to local dungeon space
    float localX = worldPos.x - dungeonOrigin.x;
    float localZ = worldPos.z - dungeonOrigin.z;

    // Quick reject (tiny epsilon protects boundary float jitter)
    const float eps = 1e-5f;
    if (localX < -eps || localZ < -eps) return false;

    int gx = (int)floorf(localX / tileSize);  // world-grid (unflipped)
    int gy = (int)floorf(localZ / tileSize);

    if (gx < 0 || gy < 0 || gx >= dungeonWidth || gy >= dungeonHeight) return false;

    // Convert to image-space indices (your Generate uses flippedX/Y)
    out.x = dungeonWidth  - 1 - gx;
    out.y = dungeonHeight - 1 - gy;
    return true;
}


Vector3 GetDungeonWorldPos(int x, int y, float tileSize, float baseY) {
    //returns world position, centered on tile. 
    int flippedY = dungeonHeight - 1 - y;
    int flippedX = dungeonWidth - 1 - x;
    //flip x and y to match world coords. 
    return Vector3{
        flippedX * tileSize + tileSize / 2.0f, //center of the tile
        baseY,
        flippedY * tileSize + tileSize / 2.0f
    };
}

int GetDungeonImageX(float worldX, float tileSize, int dungeonWidth) {
    return dungeonWidth - 1 - (int)(worldX / tileSize);
}

int GetDungeonImageY(float worldZ, float tileSize, int dungeonHeight) {
    return dungeonHeight - 1 - (int)(worldZ / tileSize);
}

static inline bool IsMaskSet(const std::vector<uint8_t>& mask, int x, int y)
{
    if (x < 0 || y < 0 || x >= dungeonWidth || y >= dungeonHeight) return false;
    return mask[y * dungeonWidth + x] != 0;
}

void UpdateDungeonTileFlags(Player& player, float dt)
{
    GridCoord g;
    player.overLava = false;
    player.overVoid = false;

    if (!WorldToGrid(player.position, g, tileSize, dungeonWidth, dungeonHeight))
        return;

    player.overLava = IsMaskSet(lavaMask, g.x, g.y);

    player.overVoid = IsMaskSet(voidMask, g.x, g.y);


    if (lavaMask[g.y * dungeonWidth + g.x]) {
        //player.overLava = true;
        lavaTimer += dt;
        if (lavaTimer > tickDamage && player.grounded && player.position.y < floorHeight){ // only damage if on floor. //floor is lower over lava // a bit fucky
            player.TakeDamage(10);

            lavaTimer = 0.0;

        }
        
        // sfx/vfx here: maybe a fire decal, and a sizzle. 
    }
}


void ApplyEnemyLavaDPS(){
    GridCoord g;
    for (Character* enemy : enemyPtrs){
        if (!WorldToGrid(enemy->position, g, tileSize, dungeonWidth, dungeonHeight)) return;

        if (lavaMask[g.y * dungeonWidth + g.x]){
            enemy->overLava = true;
        }

        if (voidMask[g.y * dungeonWidth + g.x]) {
            enemy->overVoid = true;
        }
    }
}




void UpdateLauncherTraps(float dt){
    const float SPEED    = 900.0f;
    const float LIFE     = 5.0f;
    const float AHEAD    = 1500.0f;

    for (LauncherTrap& L : launchers){
        L.cooldown -= dt;
        if (L.cooldown > 0.0f) continue;

        Vector3 origin = { L.position.x, L.position.y + 100.0f, L.position.z };
        Vector3 target = Vector3Add(origin, Vector3Scale(L.direction, AHEAD));
        if (L.type == TrapType::fireball){
            FireFireball(origin, target, SPEED, LIFE, /*enemy=*/true, /*launcher=*/true, false);
            L.cooldown = std::max(0.01f, L.fireIntervalSec);

        }else if (L.type == TrapType::iceball){
            FireIceball(origin, target, SPEED, LIFE,true, true);
            L.cooldown = std::max(0.01f, L.fireIntervalSec);
        }

    }
}

void DrawFlatWeb(Texture2D texture, Vector3 position, float width, float height, float rotationY, Color tint)
{
    // Compute 4 corners of the quad in local space
    Vector3 p1 = {-width/2, -height/2, 0};
    Vector3 p2 = { width/2, -height/2, 0};
    Vector3 p3 = { width/2,  height/2, 0};
    Vector3 p4 = {-width/2,  height/2, 0};

    // Apply Y-axis rotation
    Matrix rot = MatrixRotateY(rotationY);
    p1 = Vector3Transform(p1, rot);
    p2 = Vector3Transform(p2, rot);
    p3 = Vector3Transform(p3, rot);
    p4 = Vector3Transform(p4, rot);

    // Translate to world position
    p1 = Vector3Add(p1, position);
    p2 = Vector3Add(p2, position);
    p3 = Vector3Add(p3, position);
    p4 = Vector3Add(p4, position);

    // Draw the textured quad
    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        rlTexCoord2f(0, 0); rlVertex3f(p1.x, p1.y, p1.z);
        rlTexCoord2f(1, 0); rlVertex3f(p2.x, p2.y, p2.z);
        rlTexCoord2f(1, 1); rlVertex3f(p3.x, p3.y, p3.z);
        rlTexCoord2f(0, 1); rlVertex3f(p4.x, p4.y, p4.z);

    rlEnd();

    rlSetTexture(0);
}



void DrawLaunchers() {
    for (const LauncherTrap& launcher : launchers) {

        Vector3 offsetPos = {launcher.position.x, launcher.position.y + 20, launcher.position.z}; 
        DrawModelEx(R.GetModel("stonePillar"), offsetPos, Vector3{0,1,0}, 0.0f, Vector3{100, 100, 100}, WHITE);
    }

}



void DrawDungeonBarrels() {
    for (const BarrelInstance& barrel : barrelInstances) {
        Vector3 offsetPos = {barrel.position.x, barrel.position.y + 20, barrel.position.z}; //move the barrel up a bit
        Model modelToDraw = barrel.destroyed ? R.GetModel("brokeBarrel") : R.GetModel("barrelModel");
        DrawModelEx(modelToDraw, offsetPos, Vector3{0, 1, 0}, 0.0f, Vector3{350.0f, 350.0f, 350.0f}, barrel.tint); //scaled half size
        
    }


}


void DrawDungeonChests() {
   
    for (const ChestInstance& chest : chestInstances) {
        Vector3 offsetPos = {chest.position.x, chest.position.y + 20, chest.position.z};
        if (chest.animFrame > 0){
            offsetPos.z -= 45;
        }
        DrawModelEx(chest.model, offsetPos, Vector3{0, 1, 0}, 0.0f, Vector3{60.0f, 60.0f, 60.0f}, chest.tint);
    }
    
}


void DebugDrawGrappleBox(){
    for (GrapplePoint& gp : grapplePoints){
        DrawBoundingBox(gp.box, GREEN);
    }
}

void DrawDungeonGeometry(Camera& camera, float maxDrawDist){
    const Vector3 baseScale   = {700, 700, 700};

    ViewConeParams vp = MakeViewConeParams(
        camera,
        55.0f,  //narrower cone for dungeons because we can get away with it.     
        maxDrawDist,
        400.0f    
    );

    //Walls
    for (const WallInstance& _wall : wallInstances) {
        if (!IsInViewCone(vp, _wall.position) && !debugInfo) continue;
        DrawModelEx(R.GetModel("wallSegment"), _wall.position, Vector3{0, 1, 0}, _wall.rotationY, Vector3{700, 700, 700}, _wall.tint);

    }

    // for (const WallInstance& window : windowWallInstances) {
    //     if (!IsInViewCone(vp, window.position) && !debugInfo) continue;
    //     DrawModelEx(R.GetModel("windowedWall"), window.position, Vector3{0, 1, 0}, window.rotationY, Vector3{700, 700, 700}, window.tint);
    // }


    //Doorways
    for (const DoorwayInstance& d : doorways) { 
        //if (!IsInViewCone(vp, d.position)) continue;  //dont cull doorways because of dungeon entrances outside are doorways
        

        if (d.window){
            Vector3 dPos = {d.position.x, d.position.y, d.position.z};
            DrawModelEx(R.GetModel("windowWay"), dPos, {0, 1, 0}, d.rotationY * RAD2DEG, {500, 595, 500}, d.tint);
        }else{
            Vector3 dPos = {d.position.x, d.position.y + 100, d.position.z};
            DrawModelEx(R.GetModel("doorWayGray"), dPos, {0, 1, 0}, d.rotationY * RAD2DEG, {490, 595, 476}, d.tint);
        }

        
    }

    //Floors

    for (const FloorTile& tile : floorTiles) {
        if (!IsInViewCone(vp, tile.position) && !debugInfo) continue;
        DrawModelEx(R.GetModel("floorTileGray"), tile.position, {0,1,0}, 0.0f, baseScale, tile.tint);    
    }

    //Lava floor
    for (const FloorTile& lavaTile : lavaTiles){
        if (!IsInViewCone(vp, lavaTile.position) && !debugInfo) continue;

        DrawModelEx(R.GetModel("lavaTile"), lavaTile.position, {0, 1, 0}, 0.0f, baseScale, lavaTile.tint);

    }

    //Ceilings
    //rlEnableBackfaceCulling();
    float scale = dungeonWidth * tileSize;
    if (drawCeiling && isDungeon){
        DrawModelEx(R.GetModel("ceilingPlane"), Vector3 {scale/2, ceilingHeight, scale/2}, {0,1,0}, 0.0f, Vector3{scale, scale, scale}, WHITE); 
    } 
    // for (CeilingTile& tile : ceilingTiles){
    //     if (!IsInViewCone(vp, tile.position) && !debugInfo) continue;

    //     if (!drawCeiling) continue;
    //     DrawModelEx(R.GetModel("floorTileGray"), tile.position, {1,0,0}, 180.0f, baseScale, tile.tint);
    // }
    //rlDisableBackfaceCulling();

}


void DrawFlatDoor(Texture2D tex,
                  Vector3 hingeBL,     // bottom-left hinge corner
                  float width,
                  float height,
                  float rotYClosed,    // closed yaw (radians)
                  bool isOpen,
                  Color tint)
{
    float w = width;
    float h = height;

    Vector3 up = { 0, 1, 0 };

    // Use CLOSED forward to decide swing direction
    Vector3 forwardClosed = Vector3RotateByAxisAngle({0, 0, 1}, up, rotYClosed);

    bool isVertical = fabsf(forwardClosed.x) > fabsf(forwardClosed.z);

    // Pick which way each family swings
    float openSign = isVertical ? 1.0f : -1.0f;

    float openAngle = isOpen ? (openSign * PI * 0.5f) : 0.0f; // ±90° or 0
    float rotY      = rotYClosed + openAngle;


    // Final basis from FINAL yaw
    Vector3 forward = Vector3RotateByAxisAngle({0, 0, 1}, up, rotY);
    Vector3 right   = Vector3CrossProduct(up, forward);


    // Optional: small fudge so open door looks less "slid"
    Vector3 openOffset = {0, 0, 0};

    if (isOpen)
    {
        const float base = 75.0f;

        float kRight   = 0.0f;
        float kForward = 0.0f;

        // Decide facing based on CLOSED forward
        if (forwardClosed.z > 0.5f) {
            // Facing +Z
            kRight   =  base;
            kForward = -base;
        }
        else if (forwardClosed.z < -0.5f) {
            // Facing -Z
            kRight   = -base;
            kForward = -base;
        }
        else if (forwardClosed.x > 0.5f) {
            // Facing +X  (vertical door #1)
            kRight   = -base;
            kForward =  base;   // <-- try +base here
        }
        else if (forwardClosed.x < -0.5f) {
            // Facing -X  (vertical door #2)
            kRight   =  base;
            kForward =  base;   // <-- and +base here
        }

        openOffset = Vector3Add(
            Vector3Scale(right,   kRight),
            Vector3Scale(forward, kForward)
        );
    }

    // Build quad from hinge corner + openOffset
    Vector3 bottomLeft  = Vector3Add(hingeBL, openOffset);
    Vector3 bottomRight = Vector3Add(bottomLeft, Vector3Scale(right, w));
    Vector3 topLeft     = Vector3Add(bottomLeft, (Vector3){0, h, 0});
    Vector3 topRight    = Vector3Add(bottomRight,(Vector3){0, h, 0});


    BeginBlendMode(BLEND_ALPHA);
    if (!isDungeon) BeginShaderMode(R.GetShader("treeShader"));

    rlEnableDepthTest();
    rlDisableDepthMask();

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        rlTexCoord2f(0, 1); rlVertex3f(bottomLeft.x,  bottomLeft.y,  bottomLeft.z);
        rlTexCoord2f(1, 1); rlVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
        rlTexCoord2f(1, 0); rlVertex3f(topRight.x,    topRight.y,    topRight.z);
        rlTexCoord2f(0, 0); rlVertex3f(topLeft.x,     topLeft.y,     topLeft.z);
    rlEnd();

    rlSetTexture(0);
    rlColor4ub(255, 255, 255, 255);
    rlEnableDepthMask();

    EndBlendMode();
    if (!isDungeon) EndShaderMode();
}




void DrawDungeonPillars() {
    //Pillars means Pedestal fire light sources. Light sources are generated separatly and spawn at pillar positions. 
    for (size_t i = 0; i < pillars.size(); ++i) {
        const PillarInstance& pillar = pillars[i];
        //Fire& fire = fires[i];

        // Draw the pedestal model
        DrawModelEx(R.GetModel("lampModel"), pillar.position, Vector3{0, 1, 0}, pillar.rotation, Vector3{350, 350, 350}, LIGHTGRAY);

    }
}

void HandleDungeonTints() {

    UpdateChestTints(player.position);
    UpdateDoorTints(player.position);
}


void UpdateChestTints(Vector3 playerPos) {
    const float maxLightDistance = 4000.0f;
    const float minBrightness = 0.2f;

    Vector3 warmchestColor = {0.9f, 0.7f, 0.6f}; // slightly muted warm tone

    for (ChestInstance& chest : chestInstances) {
        float dist = Vector3Distance(playerPos, chest.position);
        float brightness = Clamp(1.0f - (dist / maxLightDistance), minBrightness, 1.0f);
        Vector3 tinted = Vector3Scale(warmchestColor, brightness);
        chest.tint = ColorFromNormalized((Vector4){ tinted.x, tinted.y, tinted.z, 1.0f });
    }
}


void UpdateBarrelTints(Vector3 playerPos) {
    const float maxLightDistance = 4000.0f;
    const float minBrightness = 0.2f;

    Vector3 warmbarrelColor = {0.9f, 0.7f, 0.6f}; // slightly muted warm tone

    for (BarrelInstance& barrel : barrelInstances) {
        float dist = Vector3Distance(playerPos, barrel.position);
        float brightness = Clamp(1.0f - (dist / maxLightDistance), minBrightness, 1.0f);
        Vector3 tinted = Vector3Scale(warmbarrelColor, brightness);
        barrel.tint = ColorFromNormalized((Vector4){ tinted.x, tinted.y, tinted.z, 1.0f });
    }
}

void UpdateDoorTints(Vector3 playerPos) {
    if (!isDungeon) return;
    const float maxLightDistance = 5000.0f;
    const float minBrightness = 0.2f;

    Vector3 baseDoorColor = {0.8f, 0.6f, 0.4f}; // warm wood tone

    for (Door& door : doors) {
        float dist = Vector3Distance(playerPos, door.position);
        float brightness = Clamp(1.0f - (dist / maxLightDistance), minBrightness, 1.0f);
        Vector3 tinted = Vector3Scale(baseDoorColor, brightness);
        door.tint = ColorFromNormalized((Vector4){ tinted.x, tinted.y, tinted.z, 1.0f });
    }
}


bool IsDungeonFloorTile(int x, int y) {
    if (x < 0 || x >= dungeonWidth || y < 0 || y >= dungeonHeight) return false;
    Color c = dungeonPixels[y * dungeonWidth + x];
    return c.r > 200 && c.g > 200 && c.b > 200;  // close to white
}



BoundingBox MakeDoorBoundingBox(Vector3 position, float rotationY, float halfWidth, float height, float depth) {
    //covers the full archway
    Vector3 forward = Vector3RotateByAxisAngle({0, 0, 1}, {0, 1, 0}, rotationY);
    Vector3 right = Vector3CrossProduct({0, 1, 0}, forward);

    // Calculate combined half extents
    Vector3 halfExtents = Vector3Add(
        Vector3Scale(right, halfWidth),
        Vector3Scale(forward, depth)
    );

    Vector3 boxMin = {
        position.x - fabsf(halfExtents.x),
        position.y,
        position.z - fabsf(halfExtents.z)
    };

    Vector3 boxMax = {
        position.x + fabsf(halfExtents.x),
        position.y + height,
        position.z + fabsf(halfExtents.z)
    };

    return { boxMin, boxMax };
}

void ClearDungeon() {
    wallRunColliders.clear();
    floorTiles.clear();
    wallInstances.clear();
    ceilingTiles.clear();
    pillars.clear();
    barrelInstances.clear();
    dungeonLights.clear();
    doors.clear();
    doorways.clear();
    collectables.clear();
    decals.clear();
    eggs.clear();
    frameLights.clear();
    lavaMask.clear();
    launchers.clear();
    lavaTiles.clear();
    grapplePoints.clear();
    secretWalls.clear();
    invisibleWalls.clear(); //there aren't any invisible walls yet.
    windowColliders.clear(); 


    for (ChestInstance& chest : chestInstances) {
        UnloadModelAnimations(chest.animations, chest.animCount);
    }
    chestInstances.clear();
   
}

