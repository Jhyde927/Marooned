#pragma once

#include <string>
#include <vector>
#include "raylib.h"
#include <optional>

enum class PropType { Barrel, FirePit, Boat};

struct PropSpawn {
    PropType type;
    Vector3 position; // x, z;        // author in XZ; we'll resolve Y from heightmap //why is this not just a position. where do we set the y value? 
    float yawDeg = 0;  // rotation around Y
    float scale  = 1.f;
};

struct DungeonEntrance {
    Vector3 position;
    int linkedLevelIndex;
    bool isLocked = false;

};

// Struct that holds all necessary data for a level
struct LevelData {
    std::string name;                 // Display name
    std::string heightmapPath;       // Path to the heightmap image
    std::string dungeonPath;

    Vector3 startPosition;           // Player starting position
    float startingRotationY; // ← new
    Vector3 raptorSpawnCenter;       // Center point for raptor spawns
    int raptorCount;                 // Number of raptors to spawn
    bool isDungeon;

    std::vector<DungeonEntrance> entrances;
    int levelIndex; //current LevelIndex
    int nextLevel; //next level index

    std::vector<PropSpawn> overworldProps;   // authored list
    bool hasCeiling;
    
    
};

// All available levels
extern std::vector<LevelData> levels;


