#pragma once

namespace GameSettings
{
    inline bool squareRes = false; // set true for 1024x1024, false for widescreen
    inline bool showTutorial = true;
    inline bool useVsync = true;
    inline bool drawMinimap = true;

    inline float mouseSensitivity = 0.05f;

    inline constexpr float minMouseSensitivity = 0.01f;
    inline constexpr float maxMouseSensitivity = 0.10f;

    inline float fovY = 45.0f;

    inline constexpr float minFovY = 30.0f;
    inline constexpr float maxFovY = 80.0f;

    inline float maxDrawDist = 25000.0f;
    inline constexpr float minDrawDist = 5000.0f;
    inline constexpr float maxDrawDistLimit = 50000.0f;

    inline int gVisibleDungeonInstanceCount;
    inline int gTotalDungeonInstanceCount;

    //Distant FOG:
    inline bool useFog = true;

    //Lighting mode
    inline bool useDDALighting = false;

    //outdoor props, entrances
    inline float treefogStartMenu = 8000.0f; 
    inline float treeFogStart = 8000.0f;
    inline float treeFogEnd = 20000.0f;

    //terrain
    inline float terrainFogStartMenu = 9000.0f;
    inline float terrainFogStart = 6000.0f;
    inline float terrainFogEnd = 23000.0f;

    //trees and grass instanced
    inline float instancedFogStartMenu = 10000.0f;
    inline float instancedFogStart = 3000.0f;
    inline float instancedFogEnd = 20000.0f;

    //randomized boss mobs. 5 percent chance for a large version of the enemy. 
    inline int BossPercentage = 5;

}