#pragma once
#include "raylib.h"
#include <cstddef>
#include "level.h"
#include "camera_system.h"

namespace MainMenu
{
    enum class Action
    {
        None,
        StartGame,
        CycleLevel,
        ToggleFullscreen,
        Quit
    };

    struct State
    {
        int selectedOption = 0;
        float pressFlash[4] = { 0,0,0,0 }; // seconds remaining for “push” effect
    };

    // Call once if you want to reset selection when entering menu
    inline void Reset(State& s) { s.selectedOption = 0; }

    struct Layout
    {
        Rectangle selectable[4]; // Start, Level, Fullscreen, Quit
        Rectangle levelName;     // display-only
    };

    Layout ComputeLayout(float menuX,
                         float baseY,
                         float gapY,
                         float btnW,
                         float btnH);

    Action Update(State& s, float dt,
                  bool levelLoaded,
                  int optionsCount,
                  int& levelIndex,
                  int levelsCount,
                  const Layout& layout);
    



    // Draws your title + menu lines (same look, shadowed text).
    void Draw(const State& s,
              int levelIndex,
              const LevelData* levels,
              int levelsCount);
    
}

int ComputeMenuX(Font titleFont,
    const char* title,
    int titleFontSize,
    int offset);

