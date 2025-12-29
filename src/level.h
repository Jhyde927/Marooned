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

// -------------------------
// Menu Preview Layer
// -------------------------

enum class PreviewKind
{
    None,
    OverworldHeightmap,
    DungeonMap
};

struct PreviewInfo
{
    int levelIndex = -1;
    std::string displayName;

    PreviewKind kind = PreviewKind::None;

    // What file represents the preview (used for registration/loading)
    std::string previewPath;

    // ResourceManager key used to fetch the texture at draw time
    std::string textureKey;

    bool IsValid() const
    {
        
        return kind != PreviewKind::None
            && !previewPath.empty()
            && !textureKey.empty();
    }
};

// Builds preview list from `levels`.
// If preloadTextures is true, registers/loads textures into ResourceManager.
// Returns a vector where each element corresponds to a level (either same order as `levels`,
// or indexed by levelIndex—your choice in implementation).
std::vector<PreviewInfo> BuildLevelPreviews(bool preloadTextures);

// Helper that decides which preview a level should use (and what kind it is).
PreviewInfo MakePreviewInfoFromLevel(const LevelData& level);




