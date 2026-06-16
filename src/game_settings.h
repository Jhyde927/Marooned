#pragma once

namespace GameSettings
{
    inline bool squareRes = false; // set true for 1024x1024, false for widescreen
    inline bool showTutorial = true;
    inline bool useVsync = true;

    inline float mouseSensitivity = 0.05f;

    inline constexpr float minMouseSensitivity = 0.01f;
    inline constexpr float maxMouseSensitivity = 0.10f;

    inline float fovY = 45.0f;

    inline constexpr float minFovY = 30.0f;
    inline constexpr float maxFovY = 80.0f;

    inline float maxDrawDist = 30000.0f;
    inline constexpr float minDrawDist = 5000.0f;
    inline constexpr float maxDrawDistLimit = 50000.0f;

    inline int gVisibleDungeonInstanceCount;
    inline int gTotalDungeonInstanceCount;

    //Distant FOG:

    //outdoor props, entrances
    inline float treefogStartMenu = 16000.0f; //entrances were too heavily fogged. 
    inline float treeFogStart = 16000.0f;
    inline float treeFogEnd = 30000.0f;

    //terrain
    inline float terrainFogStartMenu = 9000.0f;
    inline float terrainFogStart = 6000.0f;
    inline float terrainFogEnd = 23000.0f;

    //trees and grass instanced
    inline float instancedFogStartMenu = 10000.0f;
    inline float instancedFogStart = 3000.0f;
    inline float instancedFogEnd = 20000.0f;

}