// fullscreen_toggle.h 
#pragma once

#include "raylib.h"

struct WindowedRestore
{
    bool  hasSaved = false;
    int   x = 100, y = 100;
    int   w = 1280, h = 720;
    int   monitor = 0;
};

static WindowedRestore gWindowed;

// Call this once after InitWindow() if you want a sane default restore size.
static void InitWindowedRestoreDefaults(int w, int h)
{
    gWindowed.w = w;
    gWindowed.h = h;
    gWindowed.hasSaved = false;
}

// Borderless fullscreen toggle (does NOT change display mode)
static void ToggleBorderlessFullscreenClean()
{
    // If we're currently NOT borderless, we're about to enter it -> save windowed rect
    if (!IsWindowState(FLAG_WINDOW_UNDECORATED))
    {
        Vector2 pos = GetWindowPosition(); // raylib provides this on desktop
        gWindowed.x = (int)pos.x;
        gWindowed.y = (int)pos.y;
        gWindowed.w = GetScreenWidth();
        gWindowed.h = GetScreenHeight();
        gWindowed.monitor = GetCurrentMonitor();
        gWindowed.hasSaved = true;

        // Enter borderless fullscreen (raylib sizes window to the current monitor)
        ToggleBorderlessWindowed();
    }
    else
    {
        // Exit borderless -> back to decorated window + restore size/pos
        ToggleBorderlessWindowed();

        // Some WMs can be finicky; be explicit about returning to normal window state.
        ClearWindowState(FLAG_WINDOW_UNDECORATED);

        if (gWindowed.hasSaved)
        {
            SetWindowSize(gWindowed.w, gWindowed.h);
            SetWindowPosition(gWindowed.x, gWindowed.y);
        }
    }
}
