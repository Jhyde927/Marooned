#pragma once
#include "raylib.h"
#include <cstddef>
#include "level.h"
#include "camera_system.h"

namespace MainMenu
{

    extern std::vector<PreviewInfo> gLevelPreviews;

    struct State
    {
        int selectedOption = 0;
        bool showControls = false;
        bool showPreview = false;
        bool showOptions = false;
        bool showMenu = true;
        float pressFlash[4] = { 0,0,0,0 }; // seconds remaining for “push” effect
    };

    enum class Action
    {
        None,
        StartGame,
        Resume,
        CycleLevel,
        Controls,
        ToggleFullscreen,
        Quit
    };


    struct Layout
    {
        Rectangle selectable[5]; // Start, Level, Controls, Fullscreen, Quit, 
    };

    // Call once if you want to reset selection when entering menu
    inline void Reset(State& s) { s.selectedOption = 0; }

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
