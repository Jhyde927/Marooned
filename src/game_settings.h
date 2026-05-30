#pragma once

namespace GameSettings
{
    inline bool squareRes = false; // set true for 1024x1024, false for widescreen
    inline bool showTutorial = true;
    inline bool useVsync = false;

    inline float mouseSensitivity = 0.05f;

    inline constexpr float minMouseSensitivity = 0.01f;
    inline constexpr float maxMouseSensitivity = 0.10f;

    inline float fovY = 45.0f;

    inline constexpr float minFovY = 30.0f;
    inline constexpr float maxFovY = 80.0f;

    inline float maxDrawDist = 20000.0f;
    inline constexpr float minDrawDist = 5000.0f;
    inline constexpr float maxDrawDistLimit = 50000.0f;

    inline float maxDungeonDist = 16000;

    inline int gVisibleFloorTileCount;
    inline int gTotalFloorTileCount;
}