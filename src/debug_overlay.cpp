#include "debug_overlay.h"

#include <cstdio>
#include "resourceManager.h"


// ------------------------------------------------------------
// Small helpers local to this file
// ------------------------------------------------------------



static const char* FormatTime(float seconds) {
    static char buffer[32];

    int totalSeconds = static_cast<int>(seconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
    return buffer;
}

static void DrawTextExShadowed(
    Font font,
    const char* text,
    int x,
    int y,
    float fontSize,
    float spacing,
    Color color
) {
    Vector2 pos = { static_cast<float>(x), static_cast<float>(y) };

    DrawTextEx(
        font,
        text,
        { pos.x + 2.0f, pos.y + 2.0f },
        fontSize,
        spacing,
        Fade(BLACK, 0.85f)
    );

    DrawTextEx(
        font,
        text,
        pos,
        fontSize,
        spacing,
        color
    );
}

static void DrawDebugLine(
    Font font,
    const char* label,
    const char* value,
    int x,
    int y,
    int labelWidth,
    float fontSize,
    float spacing,
    Color textColor
) {
    DrawTextExShadowed(font, label, x, y, fontSize, spacing, textColor);
    DrawTextExShadowed(font, value, x + labelWidth, y, fontSize, spacing, textColor);
}

// ------------------------------------------------------------
// Main overlay
// ------------------------------------------------------------

void DrawDebugOverlay(const DebugOverlayInfo& info) {

    Font terminal = R.GetFont("terminal");

    const int screenW = GetScreenWidth();
    //const int screenH = GetScreenHeight();

    const float scale = 1.25f;
    const float spacing = 1.0f * scale;

    const int lineHeight = static_cast<int>(24 * scale);

    const int padding = static_cast<int>(16 * scale);
    const float fontSize = 18.0f * scale;
    const float titleFontSize = 20.0f * scale;

    const int columns = 2;
    const int rowsPerColumn = 11;

    const int panelW = static_cast<int>(575 * scale); // was 720
    const int panelX = screenW - panelW - static_cast<int>(20 * scale) + static_cast<int>(1 * scale);

    const int columnW = (panelW - padding * 2) / columns;
    const int labelWidth = static_cast<int>(135 * scale);

    const int titleRows = 1;
    const int panelH = padding * 2 + lineHeight * (rowsPerColumn + titleRows);

    const int panelY = 20;//screenH / 2 - panelH / 2 + static_cast<int>(100 * scale);

    const int columnGap = static_cast<int>(300 * scale);


    Color textColor = { 255, 176, 64, 255 };      // warm amber
    Color titleColor = { 255, 210, 120, 255 };    // brighter amber
    Color borderColor = { 180, 110, 35, 180 };    // darker amber border
    Color panelColor = { 20, 12, 4, 210 };        // dark brown/black background
    Color tableLineColor = { 255, 176, 64, 45 };  // subtle amber grid lines

    // Panel background
    DrawRectangle(panelX, panelY, panelW, panelH, panelColor);

    // Border, doubled for a slightly stronger console look
    DrawRectangleLines(panelX, panelY, panelW, panelH, borderColor);
    DrawRectangleLines(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Fade(GREEN, 0.35f));

    //vertical column divider line
    int dividerX = panelX + padding + columnW - 20;
    DrawLine(
        dividerX,
        panelY + padding + lineHeight,
        dividerX,
        panelY + panelH - padding,
        tableLineColor
    );


    int startX = panelX + padding;
    int startY = panelY + padding/2;
    //horizontal table lines
    for (int row = 0; row < rowsPerColumn; row++)
    {
        int lineY = startY + lineHeight + row * lineHeight + lineHeight - 2;

        DrawLine(
            panelX + padding,
            lineY,
            panelX + panelW - padding,
            lineY,
            tableLineColor
        );
    }

    DrawTextExShadowed(
        terminal,
        "MAROONED DEBUG",
        startX,
        startY,
        titleFontSize,
        spacing,
        titleColor
    );

    int rowIndex = 0;
    char buffer[128];

    auto DrawRow = [&](const char* label, const char* value)
    {
        int col = rowIndex / rowsPerColumn;
        int row = rowIndex % rowsPerColumn;

        int drawX = startX + col * columnGap;
        int drawY = startY + lineHeight + row * lineHeight;

        DrawDebugLine(
            terminal,
            label,
            value,
            drawX,
            drawY,
            labelWidth,
            fontSize,
            spacing,
            textColor
        );

        rowIndex++;
    };

    DrawRow("FPS", TextFormat("%d", info.fps));
    DrawRow("UPTIME", FormatTime(info.elapsedTime));
    DrawRow("VSync", info.useVsync ? "true" : "false");
    DrawRow("FreeCam", info.freeCam ? "true" : "false");
    DrawRow("Ceiling", info.showCeiling ? "true" : "false");
    DrawRow("Level", info.levelName);
    DrawRow("Index", TextFormat("%d", info.levelIndex));

    std::snprintf(buffer, sizeof(buffer), "%.0f", info.drawDistance);
    DrawRow("DRAW DIST", buffer);

    std::snprintf(buffer, sizeof(buffer), "%.1f", info.fovY);
    DrawRow("FOV Y", buffer);

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%4d / %4d",
        info.visibleInstances,
        info.totalInstances
    );
    DrawRow("Geometry", buffer);

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%4d / %4d",
        info.visibleFoliage,
        info.totalFoliage
    );
    DrawRow("FOLIAGE", buffer);

    std::snprintf(
        buffer,
        sizeof(buffer),
        "%4d / %4d",
        info.visibleTerrainChunks,
        info.totalTerrainChunks
    );
    DrawRow("Terrain", buffer);

    std::snprintf(buffer, sizeof(buffer), "%.0f%%", info.skyTransition * 100.0f);
    DrawRow("SKY", buffer);

    DrawRow("Dungeon", TextFormat("%d x %d", info.dungeonWidth, info.dungeonHeight));
    DrawRow("Static L", TextFormat("%d", info.staticLights));
    DrawRow("Dynamic L", TextFormat("%d", info.dynamicLights));
    DrawRow("Lightmap", TextFormat("%d x %d", info.lightmapWidth, info.lightmapHeight));
    DrawRow("Enemies", TextFormat("%d", info.activeEnemies));
    DrawRow("Bullets", TextFormat("%d", info.activeBullets));

    DrawRow("Particles", TextFormat("%d", info.activeParticles));

    DrawRow("Weapon", info.currentWeapon);

}
