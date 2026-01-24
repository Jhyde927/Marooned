// ui.h
#pragma once
#include "raylib.h"
#include "player.h"
#include "hintManager.h" 
#include <array>
#include "NPC.h"




// ui.h
struct WeaponSlot
{
    WeaponType type;
    const char* iconKey;   // "icon_sword", etc.
    bool* unlocked;
};

struct WeaponBar
{
    Vector2 position;
    float slotSize;
    float spacing;
    std::array<WeaponSlot, 4> slots;

    void Draw(WeaponType activeWeapon) const;
};

extern WeaponBar gWeaponBar;

enum class SlantSide { Right, Left };
extern bool gHermitIntroDone; 

// ---------- customizable style ----------
struct BarStyle {
    // geometry
    float width = 300.0f;
    float height = 30.0f;
    float slant = 24.0f;          // how much the slanted side is offset
    SlantSide slantSide = SlantSide::Right;

    // colors
    Color back      = {25,25,35,255};
    Color outline   = {200,200,220,255};
    Color lowColor  = {220,40,40,255};   // color at 0%
    Color highColor = {50,220,90,255};   // color at 100%

    // effects
    bool  gloss            = true;
    bool  stripes          = true;
    float stripesSpeed     = 80.0f;
    int   stripesSpacing   = 14;
    int   stripesThickness = 4;
    float stripesAlpha     = 0.06f;

    bool  pulseWhenLow     = true;
    float pulseThreshold   = 0.25f;       // start pulsing below this fraction
    float pulseRate        = 8.0f;        // Hz-ish
    Color pulseTarget      = BLACK;         // lerp toward this when pulsing

    bool  shadow           = true;
    Vector2 shadowOffset   = {2,4};
    float  shadowAlpha     = 0.25f;

    float outlineThickness = 2.0f;
};

void DrawMagicIcon();

// void DrawMenu(int selectedOption, int levelIndex); 
void DrawTimer(float ElapsedTime);
void UpdateMenu(Camera& camera, float dt);
void TutorialSetup();
void InitDialogs();
void DrawTrapezoidFill(Vector2 TL, Vector2 TR, Vector2 BR, Vector2 BL, float t, Color colFill, Color colBack);
void DrawTrapezoidBar(float x, float y, float value, float maxValue, const BarStyle& style);
void DrawHUDBars(const Player& player);
void UpdateHintManager(float deltaTime);
void DrawHints();
void TutorialSetup();
void InitWeaponBar();
void UpdateWeaponBarLayoutOnResize();
void DrawUI();
void UpdateInteractionNPC();
int FindHermitIndex(const std::vector<NPC>& npcs);