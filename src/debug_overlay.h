#pragma once

#include "raylib.h"
#include <string>
#include "weapon.h"

struct DebugOverlayInfo {
    // Core
    float elapsedTime = 0.0f;
    bool freeCam = false;
    bool useVsync = false;
    int showCeiling = true;
    int levelIndex = 0;
    const char* levelName = "None";
    int activeEnemies;
    int activeBullets;
    int maxParticles;
    int activeParticles;

    int staticLights;
    int dynamicLights;

    int lightmapWidth = 0;
    int lightmapHeight = 0;

    int dungeonWidth = 0;
    int dungeonHeight = 0;

    int fps = 0;
    // Camera / settings
    float drawDistance = 0.0f;
    float fovY = 0.0f;

    // Rendering stats
    int visibleInstances = 0;
    int totalInstances = 0;

    int visibleFoliage = 0;
    int totalFoliage = 0;

    int totalTerrainChunks;
    int visibleTerrainChunks;

    // World / sky
    float skyTransition = 0.0f; // 0.0 = day, 1.0 = night

    const char* currentWeapon = "None";

    // Optional text
    bool showFreeCameraHint = true;
};

void DrawDebugOverlay(const DebugOverlayInfo& info);
const char* WeaponTypeToString(WeaponType weapon);