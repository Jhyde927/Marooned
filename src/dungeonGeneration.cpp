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

std::vector<PillarInstance> pillars;
std::vector<WallRun> wallRunColliders;
std::vector<LightSource> dungeonLights; //static lights.
std::vector<SimpleLight> gDungeonLightsForward; //forward static lights 
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
        }
    }
}



void UpdateDungeonChests() {
    
    const int OPEN_START_FRAME = 0;
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
            Collectable key(CollectableType::Key, pos, R.GetTexture("keyTexture"), 100);
            
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


// BoundingBox MakeWallBoundingBox(const Vector3& start, const Vector3& end, float thickness, float height) {
//     Vector3 min = Vector3Min(start, end);
//     Vector3 max = Vector3Max(start, end);
    

//     // Expand by half thickness in perpendicular direction
//     if (start.x == end.x) {
//         // Vertical wall (same x, different z)
//         min.x -= thickness * 0.5f;
//         max.x += thickness * 0.5f;
//     } else {
//         // Horizontal wall (same z, different x)
//         min.z -= thickness * 0.5f;
//         max.z += thickness * 0.5f;
//     }

//     max.y += height; // dont jump over walls.

//     return { min, max };
// }


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

inline bool IsLava(int gx, int gy) {
    return lavaMask[Idx(gx,gy)] != 0;
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

    lavaMask.assign(dungeonWidth * dungeonHeight, 0);
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color pixel = GetImageColor(dungeonImg, x, y);
            Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

            // Only skip transparent pixels
            if (pixel.a == 0) continue;

            if (pixel.r == 200 && pixel.g == 0 && pixel.b == 0){
                //generate lava tile instead, add to vector of lava tiles. 
                FloorTile lavaTile;
                Vector3 offset = {0, -150, 0};
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




void GenerateWallTiles(float baseY) {
    //and create bounding boxes
    wallInstances.clear();
    wallRunColliders.clear();

    float wallThickness = 50.0f;
    float wallHeight = 400.0f;

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (IsBarrelColor(current)) continue;
            if (!IsWallColor(current)) continue;

            // === Horizontal Pair ===
            if (x < dungeonWidth - 1) {
                Color right = dungeonPixels[y * dungeonWidth + (x + 1)];

                if (current.a == 0 || right.a == 0) continue; // skip if either tile is void

                if (IsWallColor(right) && !IsBarrelColor(right)) {
                    Vector3 a = GetDungeonWorldPos(x, y, tileSize, baseY);
                    Vector3 b = GetDungeonWorldPos(x + 1, y, tileSize, baseY);

                    // Wall positioned halfway between tiles
                    Vector3 mid = Vector3Lerp(a, b, 0.5f);
                    mid.y = baseY;

                    WallInstance wall;
                    wall.position = mid;
                    wall.rotationY = 90.0f;
                    wall.tint = WHITE;
                    wallInstances.push_back(wall);

                    // Move them down to match the wall visuals
                    a.y -= 190.0f;
                    b.y -= 190.0f;

                    BoundingBox bounds = MakeWallBoundingBox(a, b, wallThickness, wallHeight);

                    wallRunColliders.push_back({ a, b, 90.0f, bounds });
                }
            }

            // === Vertical Pair ===
            if (y < dungeonHeight - 1) {
                Color down = dungeonPixels[(y + 1) * dungeonWidth + x];

                if (current.a == 0 || down.a == 0) continue; // skip if either tile is void

                if (IsWallColor(down) && !IsBarrelColor(down)) {
                    Vector3 a = GetDungeonWorldPos(x, y, tileSize, baseY);
                    Vector3 b = GetDungeonWorldPos(x, y + 1, tileSize, baseY);

                    Vector3 mid = Vector3Lerp(a, b, 0.5f);
                    mid.y = baseY;

                    WallInstance wall;
                    wall.position = mid;
                    wall.rotationY = 0.0f;
                    wall.tint = WHITE;
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
    float sideWidth = 20.0f;
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

            bool isDoor = (current.r == 128 && current.g == 0 && current.b == 128);   // purple
            bool isExit     = (current.r == 0 && current.g == 128 && current.b == 128);   // teal
            bool nextLevel = (current.r == 255 && current.g == 128 && current.b == 0); //orange
            bool lockedDoor = (current.r == 0 && current.g == 255 && current.b == 255); //CYAN
            bool portal = (current.r == 200 && current.g == 0 && current.b == 200); //portal 

            bool eventLocked = (current.r == 0   && current.g == 255 && current.b == 128); //spring-green

            if (!isDoor && !isExit && !nextLevel && !lockedDoor && !portal && !eventLocked) continue;

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

            GenerateSideColliders(pos, rotationY, archway);


            if (portal){
                archway.isPortal = true;
                nextLevel = true;
            }

            if (isExit) { //teal
                archway.linkedLevelIndex = previousLevelIndex; //go back outside. 
            }else if (nextLevel){ //orange
                archway.linkedLevelIndex = levels[currentLevelIndex].nextLevel; //door to next level
            }else if (lockedDoor){ //Aqua
                archway.isLocked = true; //locked door
            }else if (eventLocked) {
                archway.eventLocked = true; //unlock on giant spider death ect..
            } else { //purple
                archway.linkedLevelIndex = -1; //regular door
            }
        
            doorways.push_back(archway);
        }
    }

    GenerateDoorsFromArchways();
}


void GenerateDoorsFromArchways() {
    doors.clear();

    for (const DoorwayInstance& dw : doorways) {
        if (dw.isOpen) continue; // skip if this archway should remain open

        // Match position/rotation of archway
        Door door{};
        door.position = dw.position;
        door.rotationY = dw.rotationY + DEG2RAD * 90.0f; //offset why? 
        door.isOpen = false;
        door.isPortal = dw.isPortal;
        door.eventLocked = dw.eventLocked;
        door.isLocked = dw.isLocked;

        door.doorTexture = R.GetTexture("doorTexture");
        door.scale = {300, 365, 1}; //stretch it taller
        door.tileX = dw.tileX;
        door.tileY = dw.tileY;
        door.sideColliders = dw.sideColliders; //side colliders for when door is open

        
        float halfWidth = 200.0f;   // Half of the 400-unit wide doorway
        float height = 365.0f;
        float depth = 20.0f;        // Thickness into the doorway (forward axis)

        door.collider = MakeDoorBoundingBox(door.position, door.rotationY, halfWidth, height, depth); //covers the whole archway

        if (dw.linkedLevelIndex == previousLevelIndex) {
            door.doorType = DoorType::ExitToPrevious;
        } else if (dw.linkedLevelIndex == levels[levelIndex].nextLevel) {
            door.doorType = DoorType::GoToNext;
            
        } else {
            door.doorType = DoorType::Normal;
}

        door.linkedLevelIndex = dw.linkedLevelIndex; //get the level index from the archway

        doors.push_back(door);

    }
}

static constexpr float WALL_MODEL_H   = 320.0f;   // your mesh height
static constexpr float EDGE_LEN       = 200.0f;   // tile size
static constexpr float THICKNESS      = 50.0f;
static constexpr float PUSH_OUT_FACTOR = 0.5f;    // out of pit

BoundingBox MakeAABBFromSkirt(const WallInstance& s, int dir)
{
    // sizes
    const float halfLen = EDGE_LEN * 0.5f;
    const float halfTh  = THICKNESS * 0.5f;
    const float halfY   = (s.scale.y * WALL_MODEL_H) * 0.4f;

    // center (push fully into pit so it never straddles the rim)
    Vector3 outward = (dir==0)? Vector3{-1,0,0}
                    : (dir==1)? Vector3{+1,0,0}
                    : (dir==2)? Vector3{ 0,0,-1}
                               : Vector3{ 0,0,+1};

    Vector3 c = Vector3Add(s.position, Vector3Scale(outward, THICKNESS * PUSH_OUT_FACTOR));

    // decide which axis is the long edge from s.rotationY
    float r = fmodf(fabsf(s.rotationY), 180.0f);
    bool longAlongX = (r < 45.0f || r > 135.0f);   // 0°/180° → along X, 90° → along Z

    BoundingBox bb;
    bb.min.y = c.y - halfY;  bb.max.y = c.y + halfY;

    //flipped this to make it work. 
    if (!longAlongX) {                // length on X, thickness on Z
        bb.min.x = c.x - halfLen;    bb.max.x = c.x + halfLen;
        bb.min.z = c.z - halfTh;     bb.max.z = c.z + halfTh;
    } else {                         // length on Z, thickness on X
        bb.min.x = c.x - halfTh;     bb.max.x = c.x + halfTh;
        bb.min.z = c.z - halfLen;    bb.max.z = c.z + halfLen;
    }
    return bb;
}


// Make one vertical liner on edge between (x,y) and its neighbor in DIR
// dir: 0=+X (east), 1=-X (west), 2=+Z (south), 3=-Z (north)
void AddLavaSkirtEdge(int x, int y, int dir, float baseY) {
    // endpoints on the floor plane at the edge between two tiles
    Vector3 a, b;
    Vector3 ta = GetDungeonWorldPos(x, y, tileSize, baseY);

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
    const float bTop   = baseY - 50;
    const float height = (topY - lavaY);
    const float WALL_MODEL_HEIGHT = 400.0f; // visual height of your wall model

    const float t = 50.0f;
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
    const float extraThick = tileSize * 0.50f;  // add thickness so it eats the shelf from both sides

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
            bool isWeb = (current.r == 128 && current.g == 128 && current.b == 128);  // light gray

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
            if (!(current.r == 255 && current.g == 66 && current.b == 52)) continue; //if not vermillion try again. 
            
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

            launchers.push_back({ TrapType::fireball, pos, fireDir, fireIntervalSec, 0.0f, box });
        }
    }
}




void GenerateBarrels(float baseY) {
    barrelInstances.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (current.r == 0 && current.g == 0 && current.b == 255) { // Blue = Barrel
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
                    pos.y + 100.0f,
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

void GenerateChests(float baseY) {
    chestInstances.clear();
    int chestID = 0;
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (current.r == 0 && current.g == 128 && current.b == 255) { // SkyBlue = chest
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


void GeneratePotions(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (current.r == 255 && current.g == 105 && current.b == 180) { // pink for potions
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 50); // raised slightly off floor
                Collectable p = {CollectableType::HealthPotion, pos, R.GetTexture("healthPotTexture"), 40};
                collectables.push_back(p);
            }
        }
    }
}

void GenerateKeys(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            if (current.r == 255 && current.g == 200 && current.b == 0) { // Gold for keys
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY + 80); // raised slightly off floor
                Collectable key = {CollectableType::Key, pos, R.GetTexture("keyTexture"), 100.0f};
                collectables.push_back(key);
            }
        }
    }
}


void GenerateSpiderFromImage(float baseY) {
    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Look for pure red pixels (255, 0, 0) → Skeleton spawn
            if (current.r == 64 && current.g == 64 && current.b == 64) {
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
            
            if (current.r == 200 && current.g == 200 && current.b == 200) { //very light gray = ghost. 
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

            // Look for pure red pixels (255, 0, 0) → Skeleton spawn
            if (current.r == 96 && current.g == 96 && current.b == 96) {
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
            if (current.r == 255 && current.g == 0 && current.b == 0) {
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
            if (current.r == 255 && current.g == 0 && current.b == 255) {
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
                pirate.currentHealth = 400;//bullets are 25 damage x 7 for the blunderbus. 175 if all the pellets hit 
                enemies.push_back(pirate);
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
            if (current.r == 255 && current.g == 255 && current.b == 0) {
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

                dungeonLights.push_back({ pos });

                // Create a 100x100x100 bounding box centered on pos
                BoundingBox box;
                box.min = Vector3Subtract(pos, Vector3{50.0f, 0.0f, 50.0f});
                box.max = Vector3Add(pos, Vector3{50.0f, 100.0f, 50.0f});

                pillars.push_back({ pos, 1.0f, box });

                Fire newFire;
                newFire.fireFrame = GetRandomValue(0, 59);
                fires.push_back(newFire);
            }

        }
    }


}

void GenerateLightSourcesForward(float baseY) {
    gDungeonLightsForward.clear();
    pillars.clear();
    fires.clear();

    for (int y = 0; y < dungeonHeight; y++) {
        for (int x = 0; x < dungeonWidth; x++) {
            Color current = dungeonPixels[y * dungeonWidth + x];

            // Check for yellow (pure R + G, no B)
            if (current.r == 255 && current.g == 255 && current.b == 0) {
                Vector3 pos = GetDungeonWorldPos(x, y, tileSize, baseY);

                dungeonLights.push_back({ pos });

                // --- existing pillar/fire stuff ---
                BoundingBox box;
                box.min = Vector3Subtract(pos, Vector3{50.0f, 0.0f, 50.0f});
                box.max = Vector3Add(pos, Vector3{50.0f, 100.0f, 50.0f});
                pillars.push_back({ pos, 1.0f, box });

                Fire newFire;
                newFire.fireFrame = GetRandomValue(0, 59);
                fires.push_back(newFire);

                // --- new forward-light entry ---
                SimpleLight L;
                L.pos       = pos;
                L.radius    = lightConfig.staticRadius;   // tweak to taste
                L.color     = V3ToColor(lightConfig.staticColor); // warm torch color
                L.intensity = lightConfig.staticIntensity;     // scalar multiplier
                gDungeonLightsForward.push_back(L);
            }
        }
    }

    gStaticLightCount = gDungeonLightsForward.size(); // remember how many statics we have
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

void ApplyLavaDPS(Player& player, float dt, float lavaDps) {
    GridCoord g;
    player.overLava = false;
    if (!WorldToGrid(player.position, g, tileSize, dungeonWidth, dungeonHeight)) return;
    
    if (lavaMask[g.y * dungeonWidth + g.x]) {
        player.overLava = true;
        lavaTimer += dt;
        if (lavaTimer > tickDamage && player.grounded && player.position.y < floorHeight){ // only damage if on floor. //floor is lower over lava // a bit fucky
            player.TakeDamage(10);

            lavaTimer = 0.0;

        }
        
        // sfx/vfx here: maybe a fire decal, and a sizzle. 
    }
}




void UpdateLauncherTraps(float dt){
    const float SPEED    = 900.0f;
    const float LIFE     = 60.0f;
    const float AHEAD    = 1500.0f;

    for (LauncherTrap& L : launchers){
        L.cooldown -= dt;
        if (L.cooldown > 0.0f) continue;

        Vector3 origin = { L.position.x, L.position.y + 100.0f, L.position.z };
        Vector3 target = Vector3Add(origin, Vector3Scale(L.direction, AHEAD));

        FireFireball(origin, target, SPEED, LIFE, /*enemy=*/true, /*launcher=*/true);
        L.cooldown = std::max(0.01f, L.fireIntervalSec);
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

static inline void SetIsCeilingUniform(bool yes, Shader s) {
    int locCeil = GetShaderLocation(s, "isCeiling");
    int v = yes ? 1 : 0;
    if (locCeil >= 0) SetShaderValue(s, locCeil, &v, SHADER_UNIFORM_INT);
}


void DrawDungeonCeiling(){
    if (!drawCeiling) return;
    const float cull_radius = 5400;
    Model& ceilingModel = R.GetModel("floorTileGray");

    SetIsCeilingUniform(true, R.GetShader("lightingShader"));
    rlEnableBackfaceCulling();
    for (CeilingTile& tile : ceilingTiles){
        float dist = Vector3Distance(player.position, tile.position);
        if (dist < cull_radius){
            DrawModelEx(ceilingModel, tile.position, {1,0,0}, 180.0f, Vector3{700, 700, 700}, tile.tint);
        }

    }
        
    rlDisableBackfaceCulling();
    SetIsCeilingUniform(false, R.GetShader("lightingShader"));
}


void DrawDungeonFloor() {

    Model& floorModel = R.GetModel("floorTileGray");
    Model& lavaModel = R.GetModel("lavaTile");
    const float cull_radius = 5400.0f;

    const Vector3 baseScale   = {700, 700, 700};

    for (const FloorTile& tile : floorTiles) {
        float dist = Vector3Distance(player.position, tile.position);
        if (dist < cull_radius){
            DrawModelEx(floorModel, tile.position, {0,1,0}, 0.0f, baseScale, tile.tint);    
        }
        
    }

    for (const FloorTile& lavaTile : lavaTiles){
        float dist = Vector3Distance(player.position, lavaTile.position);
        if (dist < cull_radius){
            DrawModelEx(lavaModel, lavaTile.position, {0, 1, 0}, 0.0f, baseScale, lavaTile.tint);

        }

    }

}

void DrawDungeonWalls() {
    float cullRadius = 6500.0f;
    
    for (const WallInstance& _wall : wallInstances) {
        float distanceTo = Vector3Distance(player.position, _wall.position);
        if (distanceTo < cullRadius){
            DrawModelEx(R.GetModel("wallSegment"), _wall.position, Vector3{0, 1, 0}, _wall.rotationY, Vector3{700, 700, 700}, _wall.tint);

        }


    }
}

void DrawDungeonDoorways(){

    for (const DoorwayInstance& d : doorways) {
        Vector3 dPos = {d.position.x, d.position.y + 100, d.position.z};
        DrawModelEx(R.GetModel("doorWayGray"), dPos, {0, 1, 0}, d.rotationY * RAD2DEG, {490, 595, 476}, d.tint);
    }

}

void DrawFlatDoor(Texture2D tex, Vector3 pos, float width, float height, float rotY, Color tint) {
    float w = width;
    float h = height;

    // Determine local axes
    Vector3 forward = Vector3RotateByAxisAngle({0, 0, 1}, {0, 1, 0}, rotY);
    Vector3 right = Vector3CrossProduct({0, 1, 0}, forward);

    // Use pos directly as the center
    Vector3 center = pos;

    // Compute quad corners (centered on position)
    Vector3 bottomLeft  = Vector3Add(center, Vector3Add(Vector3Scale(right, -w * 0.5f), Vector3Scale(forward, -1.0f)));
    Vector3 bottomRight = Vector3Add(center, Vector3Add(Vector3Scale(right,  w * 0.5f), Vector3Scale(forward, -1.0f)));
    Vector3 topLeft     = Vector3Add(bottomLeft, {0, h, 0});
    Vector3 topRight    = Vector3Add(bottomRight, {0, h, 0});
    BeginBlendMode(BLEND_ALPHA);
    if (!isDungeon) BeginShaderMode(R.GetShader("treeShader")); //fog on flat door at distance in jungle
    rlEnableDepthTest();   // make sure testing is on
    rlDisableDepthMask();  // <-- NO depth writes from the portal..still occludes bullets for some reason. 
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
    EndShaderMode();
}




void DrawDungeonPillars() {
    //Pillars means Pedestal fire light sources. Light sources are generated separatly and spawn at pillar positions. 
    for (size_t i = 0; i < pillars.size(); ++i) {
        const PillarInstance& pillar = pillars[i];
        //Fire& fire = fires[i];

        // Draw the pedestal model
        DrawModelEx(R.GetModel("lampModel"), pillar.position, Vector3{0, 1, 0}, pillar.rotation, Vector3{350, 350, 350}, WHITE);

    }
}

void HandleDungeonTints() {

    //UpdateBarrelTints(player.position); //barrels now lit by shader
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
    const float maxLightDistance = 4000.0f;
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





void GatherFrameLights() {
    //each bullet has a struct called light. Check bullet lights and add info to LightSample struct and add to framelight vector.
    frameLights.clear();
    frameLights.reserve(activeBullets.size()); // upper bound

    for (const Bullet& b : activeBullets) {
        if (!b.light.active) continue;

        LightSample s;
        s.color     = b.light.color;
        s.range     = b.light.range;
        s.intensity = b.light.intensity;

        s.pos = b.light.detached ? b.light.posWhenDetached
                                 : b.GetPosition();

        // sanity checks (avoid NaN/zero range)
        if (s.range > 0.f && isfinite(s.range) &&
            isfinite(s.pos.x) && isfinite(s.pos.y) && isfinite(s.pos.z)) {
            frameLights.push_back(s);
        }
    }
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
    for (ChestInstance& chest : chestInstances) {
        UnloadModelAnimations(chest.animations, chest.animCount);
    }
    chestInstances.clear();
   
}

