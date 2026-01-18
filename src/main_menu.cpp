#include "main_menu.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "world.h"
#include "fullscreen_toggle.h"
#include "raylib.h"
#include "rlgl.h"


static inline void DrawControlsText(Font font, Rectangle r)
{
    const char* txt =
        "CONTROLS:\n"
        "\n"
        "WASD: Move\n"
        "Mouse: Look\n"
        "LMB: Fire\n"
        "RMB: Alt Fire\n"
        "Space: Jump\n"
        "Shift: Sprint\n"
        "E: Interact\n"
        "1-4: Weapons\n"
        "F: Use Health Potion\n"
        "G: Use Mana Potion\n"
        "Esc: Back / Menu\n";

    float fontSize = 32.0f;
    float spacing  = 1.0f;

    float pad = 22.0f;
    Vector2 pos = { r.x + pad, r.y + pad };

    // Simple readability shadow
    Color shadow = {0,0,0,180};
    Color col    = BLACK;

    DrawTextEx(font, txt, {pos.x+1, pos.y+1}, fontSize, spacing, shadow);
    DrawTextEx(font, txt, pos,               fontSize, spacing, col);
}

static const PreviewInfo* GetPreviewForSelectionIndex(int selectionIndex)
{
    if (selectionIndex < 0 || selectionIndex >= (int)MainMenu::gLevelPreviews.size())
        return nullptr;

    const PreviewInfo& p = MainMenu::gLevelPreviews[selectionIndex];
    return p.IsValid() ? &p : nullptr;
}



static Rectangle ComputePreviewPanelRect(const Rectangle& rStart, const Rectangle& rQuit)
{
    float panelX = rStart.x + rStart.width + 30.0f;
    float panelY = rStart.y;
    float panelW = 420.0f;
    float panelH = (rQuit.y + rQuit.height) - rStart.y;

    Rectangle r = { panelX, panelY, panelW, panelH };

    // Keep it on-screen (important for small windows)
    float rightEdge = r.x + r.width;
    float maxRight  = (float)GetScreenWidth() - 20.0f;
    if (rightEdge > maxRight) r.x -= (rightEdge - maxRight);

    return r;
}




Rectangle ComputeControlsPanelRect(Rectangle rTopButton, Rectangle rBottomButton)
{
    float gap = 24.0f;

    float x = rTopButton.x + rTopButton.width + gap;
    float y = rTopButton.y;

    float h = (rBottomButton.y + rBottomButton.height) - rTopButton.y + 120.0f;
    float w = 300.0f; // pick a fixed width or compute from screen

    Rectangle r = { x, y, w, h };

    // Clamp to screen (important)
    float right = r.x + r.width;
    float maxRight = (float)GetScreenWidth() - 20.0f;
    if (right > maxRight) r.x -= (right - maxRight);

    return r;
}

Rectangle ComputeOptionsPanelRect(Rectangle rTopButton, Rectangle rBottomButton)
{
    float gap = 24.0f;

    float x = rTopButton.x - rTopButton.width/2 + gap;
    float y = rTopButton.y;

    float h = (rBottomButton.y + rBottomButton.height) - rTopButton.y + 120.0f;
    float w = 300.0f * 2.0f; // 3× wider, same height

    Rectangle r = { x, y, w, h };

    // Clamp to screen (important)
    float right = r.x + r.width;
    float maxRight = (float)GetScreenWidth() - 20.0f;
    if (right > maxRight)
        r.x -= (right - maxRight);

    return r;
}






static inline Color WithAlpha(Color c, float alphaMul)
{
    c.a = (unsigned char)(c.a * alphaMul);
    return c;
}

static inline Rectangle MakeButtonRect(float cx, float cy, float w, float h)
{
    return Rectangle{ cx - w*0.5f, cy - h*0.5f, w, h };
}


MainMenu::Layout MainMenu::ComputeLayout(float menuX, float baseY, float gapY, float btnW, float btnH)
{
    float cx = menuX + btnW * 0.5f;

    Layout L{};
    L.selectable[0] = MakeButtonRect(cx, baseY + gapY*0.0f, btnW, btnH); // Start
    L.selectable[1] = MakeButtonRect(cx, baseY + gapY*1.0f, btnW, btnH); // Level
    L.selectable[2] = MakeButtonRect(cx, baseY + gapY*2.0f, btnW, btnH); //Controls
    L.selectable[3] = MakeButtonRect(cx, baseY + gapY*3.0f, btnW, btnH); // Fullscreen
    L.selectable[4] = MakeButtonRect(cx, baseY + gapY*4.0f, btnW, btnH); // Quit

   

    return L;
}


static inline void DrawMenuButtonRounded(Rectangle r, bool selected, float alphaMul = 1.0f, bool title = false)
{
    // Orangish-beige base palette (tweak)
    Color base      = WithAlpha({214, 182, 132, 255}, alphaMul);   
    Color baseHot   = WithAlpha({242, 212, 164, 240}, alphaMul);
    Color border    = WithAlpha({120,  86,  52, 220}, alphaMul);
    Color borderSel = WithAlpha({210, 170,  80, 235}, alphaMul);
    Color topHi     = WithAlpha({255, 255, 255,  35}, alphaMul);
    Color innerDark = WithAlpha({  0,   0,   0,  55}, alphaMul);

    float roundness = 0.25f;
    int   segments  = 12;

    Color face = WithAlpha({214, 182, 132, 0}, alphaMul); // lighter, warmer

    // --- Raised face panel ---
    float faceInset = 4.0f; // thickness of the outer rim


    if (title)
    {
        base = WithAlpha( { 200,  150,  100, 240 }, 0.9f);
        face = WithAlpha({ 214, 182, 132, 255 }, 1.0); // brighter cap
        faceInset = 8.0f;
        roundness = .125f;                                 // heavier slab
    }

    Rectangle faceRect = {
        r.x + faceInset,
        r.y + faceInset,
        r.width  - faceInset * 2.0f,
        r.height - faceInset * 2.0f
    };


    // Slightly smaller roundness so corners feel tighter
    float faceRoundness = roundness * 0.8f;
    DrawRectangleRounded(r, roundness, segments, selected ? baseHot : base);

    // Draw face
    DrawRectangleRounded(faceRect, faceRoundness, segments, face);

    // Optional: subtle edge on face to separate planes
    DrawRectangleRoundedLines(
        faceRect,
        faceRoundness,
        segments,
        WithAlpha({255,255,255,40}, alphaMul)
    );

    DrawRectangleRoundedLines(r, roundness, segments, selected ? borderSel : border);

    // Thicker inner groove (multiple inset lines)
    for (int i = 0; i < 5; ++i)
    {
        float inset = 0.0f + i;
        Rectangle inner = {
            r.x + inset,
            r.y + inset,
            r.width  - inset * 2.0f,
            r.height - inset * 2.0f
        };

        DrawRectangleRoundedLines(inner, roundness, segments, innerDark);
    }

    // Drop shadow (1–2 px below)
    Rectangle shadow = {
        r.x + 2,
        r.y + r.height - 1,
        r.width - 4,
        3
    };

    DrawRectangleRounded(shadow, 0.3f, 8, WithAlpha({0,0,0,35}, alphaMul));

    // Top highlight strip
    Rectangle hi = { r.x + 6, r.y + 5, r.width - 12, 6 };
    DrawRectangleRounded(hi, 0.9f, 8, topHi);
}

static inline void DrawVerticalFade(Rectangle r, Color bottom, Color top)
{
    DrawRectangleGradientV(
        (int)r.x, (int)r.y,
        (int)r.width, (int)r.height,
        top, bottom
    );
}


static inline Rectangle InsetRect(Rectangle r, float inset)
{
    return { r.x + inset, r.y + inset, r.width - inset*2.0f, r.height - inset*2.0f };
}

static inline void DrawStoneSlab(Rectangle r, bool selected, float alphaMul = 1.0f)
{
    // Cool gray stone palette
    Color base      = WithAlpha({ 240, 240, 240, 0 }, alphaMul);
    Color baseHot   = WithAlpha({ 255, 255, 255, 0 }, alphaMul); // selected slightly brighter
    Color border    = WithAlpha({  45,  50,  58, 0 }, alphaMul);
    Color bevelHi   = WithAlpha({ 255, 255, 255,  0 }, alphaMul); // top-left rim
    Color bevelLo   = WithAlpha({   0,   0,   0,  0 }, alphaMul); // bottom-right rim
    Color face      = WithAlpha({ 0, 0, 0, 0}, alphaMul); // inner face
    Color faceEdge  = WithAlpha({ 255, 255, 255,  0 }, alphaMul);

    float roundness = 0.1f;
    int   segments  = 12;

    // Slab fill
    DrawRectangleRounded(r, roundness, segments, selected ? baseHot : base);

    // Outer dark edge
    DrawRectangleRoundedLines(r, roundness, segments, border);

    // Bevel rim: draw multiple inset outlines (gives "cut stone" thickness)
    // We fake lighting: highlight near top-left, shadow near bottom-right
    for (int i = 0; i < 7; ++i)
    {
        float inset = (float)i;
        Rectangle ri = InsetRect(r, inset);

        // Top-left highlight
        DrawRectangleRoundedLines(ri, roundness, segments, bevelHi);

        // Bottom-right shadow (same stroke, but stronger as it goes inward)
        Color lo = bevelLo;
        lo.a = (unsigned char)std::min(255, (int)lo.a + i*4);
        DrawRectangleRoundedLines(ri, roundness, segments, lo);
    }

    // Inner face (slightly inset, slightly tighter corners)
    float faceInset = 8.0f;
    Rectangle rf = InsetRect(r, faceInset);
    float faceRound = roundness * 0.85f;
    DrawRectangleRounded(rf, faceRound, segments, face);
    DrawRectangleRoundedLines(rf, faceRound, segments, faceEdge);

}

static inline void DrawStoneOutlinedText(Font font, const char* text,
                                         Rectangle r, float fontSize, float spacing)
{
    // Dark stone shadow / outline
    Color outline = { 25, 28, 34, 255 };   // deep slate
    Color fill    = { 176,  32,  38, 255 }; // light stone highlight

    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p  = {
        r.x + (r.width  - sz.x) * 0.5f,
        r.y + (r.height - sz.y) * 0.5f
    };

    // Outline thickness
    const int t = 3;

    // Draw outline (8-direction)
    for (int y = -t; y <= t; ++y)
    {
        for (int x = -t; x <= t; ++x)
        {
            if (x == 0 && y == 0) continue;
            DrawTextEx(font, text,
                       { p.x + (float)x, p.y + (float)y },
                       fontSize, spacing, outline);
        }
    }

    // Main fill
    DrawTextEx(font, text, p, fontSize, spacing, fill);
}


static inline void DrawControlsPanelAsButton(Rectangle r, bool titleStyle = false)
{
    // Reuse your existing button renderer as the panel background
    // selected=false so it uses the normal palette
    DrawMenuButtonRounded(r, false, 1.0f, true);
}

static inline void DrawOptionsPanelAsButton(Rectangle r, bool titleStyle = false)
{
    // Reuse your existing button renderer as the panel background
    // selected=false so it uses the normal palette
    DrawMenuButtonRounded(r, false, 1.0f, true);
}


static inline void DrawCarvedText(Font font, const char* text, Rectangle r, float fontSize, float spacing, bool title = false, bool selected = false)
{
    // dark “burnt” letter color
    Color ink = { 100, 70, 40, 255 };
    Color inkSelected = { 120, 90, 60, 255 };

    if (title) ink = {60, 40, 20, 255};

    // subtle top-left highlight (makes it look carved)
    Color hi  = { 255, 255, 255, 85 };

    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p  = { r.x + (r.width - sz.x)*0.5f,
                   r.y + (r.height - sz.y)*0.5f };

    // Highlight first (top-left)
    DrawTextEx(font, text, { p.x - 1, p.y - 1 }, fontSize, spacing, hi);

    // Main “engraved” text (slightly down-right)
    DrawTextEx(font, text, { p.x + 1, p.y + 1 }, fontSize, spacing, (selected ? inkSelected : ink));

    // Optional: reinforce center
    DrawTextEx(font, text, p, fontSize, spacing, (selected ? inkSelected : ink));
}



static inline void DrawCenteredShadowedText(Font font, const char* text,
                                            Rectangle r, float fontSize, float spacing,
                                            Color col, int shadowPx, Color shadowCol)
{
    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 pos = {
        r.x + (r.width  - sz.x) * 0.5f,
        r.y + (r.height - sz.y) * 0.5f
    };
    DrawTextEx(font, text, {pos.x + (float)shadowPx, pos.y + (float)shadowPx}, fontSize, spacing, shadowCol);
    DrawTextEx(font, text, pos, fontSize, spacing, col);
}


static inline Rectangle PadRect(Rectangle r, float px, float py)
{
    r.x -= px; r.y -= py;
    r.width  += px*2.0f;
    r.height += py*2.0f;
    return r;
}


static void DrawLevelPreviewPanel(const Rectangle& panelRect, const PreviewInfo* preview)
{
    // Background uses your existing menu/button style
    DrawControlsPanelAsButton(panelRect, false);

    auto& font = R.GetFont("Pieces");

    // Title
    const char* header = "Map Preview";
    float headerSize = 44.0f;
    float headerSpacing = 2.0f;

    Rectangle rHeader = panelRect;
    rHeader.height = 60.0f;
    DrawCarvedText(font, header, rHeader, headerSize, headerSpacing, false, false);

    // Inner content area (padding)
    Rectangle inner = panelRect;
    inner.x += 18.0f;
    inner.y += 72.0f;
    inner.width  -= 36.0f;
    inner.height -= 90.0f;

    if (!preview)
    {
        DrawTextEx(GetFontDefault(), "No preview available", { inner.x, inner.y }, 20.0f, 1.0f, RAYWHITE);
        return;
    }

    Texture2D& tex = R.GetTexture(preview->textureKey);

    // Preserve aspect ratio, fit inside 'inner'
    float sx = inner.width  / (float)tex.width;
    float sy = inner.height / (float)tex.height;

    float s = (sx < sy) ? sx : sy;

    float drawW = tex.width  * s;
    float drawH = tex.height * s;

    float dx = inner.x + (inner.width  - drawW) * 0.5f;
    float dy = inner.y + (inner.height - drawH) * 0.125f; //quarter of the way down. 

    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = { dx, dy, drawW, drawH };

    DrawTexturePro(tex, src, dst, {0,0}, 0.0f, WHITE);

    // Optional tiny footer line (shows kind)
    const char* kindStr =
        (preview->kind == PreviewKind::DungeonMap) ? "Dungeon layout PNG" :
        (preview->kind == PreviewKind::OverworldHeightmap) ? "Overworld heightmap" :
        "Preview";

    Vector2 footerPos = { panelRect.x + 18.0f, panelRect.y + panelRect.height -64.0f };
    Font& pieces = R.GetFont("Pieces");
    DrawTextEx(pieces, kindStr, footerPos, 36.0f, 1.0f, BLACK);
}




static inline void DrawTextExShadowed(Font font,
                                      const char* text,
                                      Vector2 pos,
                                      float fontSize,
                                      float spacing,
                                      Color color,
                                      int shadowPx = -1,
                                      Color shadowCol = {0,0,0,190})
{
    if (shadowPx < 0) shadowPx = std::max(1, (int)std::round(fontSize/18.0f));
    Vector2 sh = { (float)shadowPx, (float)shadowPx };
    DrawTextEx(font, text, {pos.x + sh.x, pos.y + sh.y}, fontSize, spacing, shadowCol);
    DrawTextEx(font, text, pos,                           fontSize, spacing, color);
}

static inline void DrawTextExShadowed(Font font,
                                      const std::string& s,
                                      Vector2 pos,
                                      float fontSize,
                                      float spacing,
                                      Color color,
                                      int shadowPx = -1,
                                      Color shadowCol = {0,0,0,190})
{
    DrawTextExShadowed(font, s.c_str(), pos, fontSize, spacing, color, shadowPx, shadowCol);
}



static int WrapIndex(int i, int count)
{
    if (count <= 0) return 0;
    i %= count;
    if (i < 0) i += count;
    return i;
}

// Returns -1, 0, +1 for menu navigation using a stick with repeat.
static int StickNavStep(float stickY, float dt)
{
    // stickY: up = -1, down = +1 (raylib)
    const float DEADZONE = 0.35f;
    const float FIRST_DELAY = 0.25f; // delay before repeating when held
    const float REPEAT_RATE = 0.10f; // repeat interval

    static float holdTimer = 0.0f;
    static float repeatTimer = 0.0f;
    static int lastDir = 0;

    int dir = 0;
    if (stickY < -DEADZONE) dir = -1; // UP
    else if (stickY > DEADZONE) dir = +1; // DOWN

    if (dir == 0)
    {
        holdTimer = 0.0f;
        repeatTimer = 0.0f;
        lastDir = 0;
        return 0;
    }

    // New direction or fresh push -> instant step
    if (dir != lastDir)
    {
        lastDir = dir;
        holdTimer = 0.0f;
        repeatTimer = 0.0f;
        return dir;
    }

    // Held: wait first delay then repeat
    holdTimer += dt;
    if (holdTimer < FIRST_DELAY) return 0;

    repeatTimer += dt;
    if (repeatTimer >= REPEAT_RATE)
    {
        repeatTimer = 0.0f;
        return dir;
    }

    return 0;
}




// -------- API --------

namespace MainMenu
{

    std::vector<PreviewInfo> gLevelPreviews;

    MainMenu::Action Update(State& s, float dt, bool levelLoaded, int optionsCount, int& levelIndex, int levelsCount, const Layout& L)
    {

        auto SplitLevelRow = [&](Rectangle rLevel, Rectangle& rMinus, Rectangle& rCenter, Rectangle& rPlus)
        {
            float sideW = rLevel.height; // square buttons

            rMinus  = { rLevel.x, rLevel.y, sideW, rLevel.height };
            rPlus   = { rLevel.x + rLevel.width - sideW, rLevel.y, sideW, rLevel.height };
            rCenter = { rLevel.x + sideW, rLevel.y, rLevel.width - sideW*2.0f, rLevel.height };
        };


        if (s.showOptions && IsKeyPressed(KEY_ESCAPE))
        {
            s.showPreview = false;
            s.showOptions = false;
            s.showMenu = true;
            return Action::None;
        }

        for (int i = 0; i < 4; ++i) //button press flash
        {
            if (s.pressFlash[i] > 0.0f)
            {
                s.pressFlash[i] -= dt;
                if (s.pressFlash[i] < 0.0f) s.pressFlash[i] = 0.0f;
            }
        }

        // --- Keyboard + Gamepad navigation ---
        int navStep = 0;

        // Keyboard (one-shot)
        if (IsKeyPressed(KEY_UP))   navStep -= 1;
        if (IsKeyPressed(KEY_DOWN)) navStep += 1;

        // Keyboard repeat (your style, but dt-based)
        static constexpr float KEY_DELAY = 0.50f;
        static float upKeyTimer = 0.0f;
        static float downKeyTimer = 0.0f;

        if (IsKeyDown(KEY_UP)) {
            upKeyTimer += dt;
            if (upKeyTimer >= KEY_DELAY) { upKeyTimer = 0.0f; navStep -= 1; }
        } else upKeyTimer = 0.0f;

        if (IsKeyDown(KEY_DOWN)) {
            downKeyTimer += dt;
            if (downKeyTimer >= KEY_DELAY) { downKeyTimer = 0.0f; navStep += 1; }
        } else downKeyTimer = 0.0f;

        // Gamepad: D-pad
        if (IsGamepadAvailable(0))
        {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))   navStep -= 1;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) navStep += 1;

            // Gamepad: left stick with repeat
            float ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y); // up=-1, down=+1
            navStep += StickNavStep(ly, dt);
        }

        if (navStep != 0)
        {
            s.usingMouse = false; // last input is keyboard/gamepad
            s.selectedOption = WrapIndex(s.selectedOption + navStep, optionsCount);
        }

        auto TriggerPress = [&]()
        {
            int idx = s.selectedOption;
            if (idx >= 0 && idx < 4) s.pressFlash[idx] = 0.1f; // 100ms 
        };

        auto ActivateSelected = [&]() -> Action
        {
            if (!s.showMenu) return Action::None;

            switch (s.selectedOption)
            {
                case 0:
                    if (playerInit && levelIndex == gCurrentLevelIndex) return Action::Resume;
                    return Action::StartGame;
                case 1:
                    //Handled below. Special case for splitting button into 3. 
                    //if (levelsCount > 0) levelIndex = (levelIndex + 1) % levelsCount;
                    //if (!s.showPreview) s.showPreview = true;
                    return Action::CycleLevel;
                case 2: 
                    s.showMenu = false;
                    s.showOptions = true;
                    s.showPreview = false;
                    return Action::Controls;
                case 3:
                    ToggleBorderlessFullscreenClean();
                    isFullscreen = !isFullscreen;
                    return Action::ToggleFullscreen;
                case 4: return Action::Quit;
            }
            return Action::None;
        };

        bool activatePressed =
            IsKeyPressed(KEY_ENTER) ||
            (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)); // A

        if (activatePressed){
            TriggerPress();
            return ActivateSelected();
            
        }

        // --- Mouse hover support ---
        Vector2 m = GetMousePosition();

        int hovered = -1;
        for (int i = 0; i < 5; ++i)
        {
            if (CheckCollisionPointRec(m, L.selectable[i]))
            {
                hovered = i;
                break;
            }
        }

        s.hoveredOption = hovered;

        if (hovered != -1)
        {
            s.usingMouse = true;
            // Optional: you can *also* move focus when hovering,
            // but do NOT set -1 when not hovering.
            s.selectedOption = hovered;
        }

        if (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && s.showMenu)
        {
            // Special handling for the Level row: split into (-) [center] (+)
            if (hovered == 1 && levelsCount > 0)
            {
                Rectangle rMinus, rCenter, rPlus;
                SplitLevelRow(L.selectable[1], rMinus, rCenter, rPlus);

                if (CheckCollisionPointRec(m, rMinus))
                {
                    // back
                    levelIndex = (levelIndex - 1 + levelsCount) % levelsCount;
                    if (!s.showPreview) s.showPreview = true;
                    TriggerPress();
                    return Action::CycleLevel;
                }
                else if (CheckCollisionPointRec(m, rPlus))
                {
                    // forward
                    levelIndex = (levelIndex + 1) % levelsCount;
                    if (!s.showPreview) s.showPreview = true;
                    TriggerPress();
                    return Action::CycleLevel;
                }
                else if (CheckCollisionPointRec(m, rCenter))
                {
                    s.showPreview = !s.showPreview;
                    TriggerPress();
                    return Action::None;
                }
            }else{ //select option, other than level
                TriggerPress();
                return ActivateSelected();
            }



            
        }



        return Action::None;
    }


    void Draw(const State& s,
              int levelIndex,
              const LevelData* levels,
              int levelsCount)
    {
        auto& pieces = R.GetFont("Pieces");
        Texture2D& backFade = R.GetTexture("backFade");

        const char* title = "Marooned";
        float titleFontSize = 200.0f;
        float titleSpacing  = 10.0f;

        Vector2 titleSize = MeasureTextEx(pieces, title, titleFontSize, titleSpacing);

        float padX = -0.0f;
        float padY = -0.0f;

        float titleCX = GetScreenWidth() * 0.5f;
        float titleY  = 75.0f;

        Rectangle rTitle = {
            titleCX - titleSize.x*0.5f - padX,
            titleY,
            titleSize.x + padX*2.0f,
            titleSize.y + padY*2.0f
        };


        // --- Floating outlined title ---
        DrawStoneOutlinedText(pieces, title, rTitle, titleFontSize, titleSpacing);

        const char* levelName = (levels && levelsCount > 0) ? levels[levelIndex].name.c_str() : "None";
        float subtitleFontSize = 80.0f;
        float subtitleSpacing = 4.0f;

        Vector2 subtitleSize = MeasureTextEx(pieces, levelName, subtitleFontSize, subtitleSpacing);

        float subtitleCX = GetScreenWidth() * 0.5f;
        float subtitleY = titleY + titleSize.y * 0.5f + subtitleSize.y * 0.5f + 35.0f;

        Rectangle rSubtitle = {
            subtitleCX - subtitleSize.x * 0.5f,
            subtitleY,
            subtitleSize.x,
            subtitleSize.y
        };

        DrawStoneOutlinedText(pieces, levelName, rSubtitle, subtitleFontSize, subtitleSpacing);

        // --- Menu items (buttons) ---
        float menuFontSizeF = 60.0f;
        float menuSpacing   = 1.0f;
        int   menuShadowPx  = std::max(1, (int)(menuFontSizeF/18.0f));

        float baseY = 375.0f;
        float gapY  = 75.0f;                 // spacing between rows
        float btnH  = menuFontSizeF + 6.0f; // button height
        float btnW  = 320.0f;                // fixed width
        float menuX = GetScreenWidth() / 2.f;// rTitle.x + (rTitle.width - btnW) * 0.5f;

        // Row centers (now 5 rows because we added a display row)
        Rectangle rStart    = MakeButtonRect(menuX, baseY + gapY*0.0f, btnW, btnH);
        Rectangle rLevel    = MakeButtonRect(menuX, baseY + gapY*1.0f, btnW, btnH);
        Rectangle rControls = MakeButtonRect(menuX, baseY + gapY*2.0f, btnW, btnH);
        Rectangle rFull     = MakeButtonRect(menuX, baseY + gapY*3.0f, btnW, btnH);
        Rectangle rQuit     = MakeButtonRect(menuX, baseY + gapY*4.0f, btnW, btnH); 

        Rectangle rMenu     = MakeButtonRect(menuX, baseY + gapY*4.0f, btnW, btnH); 

        // Split Level row into (-) [center] (+)
        float sideW = rLevel.height; // square mini-buttons

        Rectangle rLevelMinus  = { rLevel.x, rLevel.y, sideW, rLevel.height };
        Rectangle rLevelPlus   = { rLevel.x + rLevel.width - sideW, rLevel.y, sideW, rLevel.height };
        Rectangle rLevelCenter = { rLevel.x + sideW, rLevel.y, rLevel.width - sideW*2.0f, rLevel.height };


        Rectangle rPanel = ComputeControlsPanelRect(rStart, rQuit);

        // Button labels
        const char* lblStart = "Start";
        if (playerInit && levelIndex == gCurrentLevelIndex) lblStart = "Resume"; //only resume if the current level = selected level. 

        const char* lblLevel = "Level";
        const char* lblControls = "Options";
        const char* lblFull  = "Fullscreen";
        const char* lblQuit  = "Quit";

        
        bool selStart    = (s.selectedOption == 0);
        bool selLevel    = (s.selectedOption == 1);
        bool selControls = (s.selectedOption == 2);
        bool selFull     = (s.selectedOption == 3);
        bool selQuit     = (s.selectedOption == 4);

        bool selMenu     = false;

        Vector2 m = GetMousePosition();
        bool hovMinus  = CheckCollisionPointRec(m, rLevelMinus);
        bool hovPlus   = CheckCollisionPointRec(m, rLevelPlus);
        bool hovCenter = CheckCollisionPointRec(m, rLevelCenter);

        // If you want hover to highlight these even if your Update sets selectedOption to -1 sometimes:
        bool selLevelMinus  = hovMinus;
        bool selLevelPlus   = hovPlus;
        bool selLevelCenter = hovCenter || (s.selectedOption == 1); // center highlights if row selected

        

        // Example: blink highlight off for Level when pressed
        if (s.pressFlash[1] > 0.05f)
        {
            selLevel = false;
            selLevelMinus = false;
            selLevelPlus = false;
            //selLevelCenter = false; //looks better without the center flashing. 

        }



        // Panel box to the right of the buttons
        float panelX = rStart.x + rStart.width + 30.0f;
        float panelY = rStart.y;
        float panelW = 420.0f;
        float panelH = (rQuit.y + rQuit.height) - rStart.y; // same height as stack

        ControlsPanel panel;
        panel.rect = { panelX, panelY, panelW, panelH };

        Rectangle pPanel = ComputePreviewPanelRect(rStart, rQuit); //preview panel

        // Keep it on-screen (important for small windows)
        float rightEdge = panel.rect.x + panel.rect.width;
        float maxRight  = (float)GetScreenWidth() - 20.0f;
        if (rightEdge > maxRight) panel.rect.x -= (rightEdge - maxRight);

        if (s.showOptions){

            //background panel
            Rectangle oPanel = ComputeOptionsPanelRect(rStart, rQuit);
            DrawOptionsPanelAsButton(oPanel, false);
            DrawMenuButtonRounded(rMenu, selMenu);

            //controls panel
            Rectangle rPanel = ComputeControlsPanelRect(rStart, rQuit);
            DrawControlsPanelAsButton(rPanel, false);     // background uses your button style
            DrawControlsText(R.GetFont("Pieces"), rPanel);   // text inside

        }

        if (s.showPreview) {
            const PreviewInfo* preview = GetPreviewForSelectionIndex(levelIndex);
            DrawLevelPreviewPanel(rPanel, preview);

        }

        if (s.showMenu){

            // Draw selectable buttons (highlight only these)
            DrawMenuButtonRounded(rStart, selStart);
            //DrawMenuButtonRounded(rLevel, selLevel); //split this into two buttons some how. 
            DrawMenuButtonRounded(rControls, selControls);
            DrawMenuButtonRounded(rFull,  selFull);
            DrawMenuButtonRounded(rQuit,  selQuit);

            // Centered text

            DrawCarvedText(pieces, lblStart, rStart, menuFontSizeF, menuSpacing, false, selStart);
            //DrawCarvedText(pieces, lblLevel, rLevel, menuFontSizeF, menuSpacing, false, selLevel);
            DrawCarvedText(pieces, lblControls,  rControls,  menuFontSizeF, menuSpacing, false, selControls);
            DrawCarvedText(pieces, lblFull,  rFull,  menuFontSizeF, menuSpacing, false, selFull);
            DrawCarvedText(pieces, lblQuit,  rQuit,  menuFontSizeF, menuSpacing, false, selQuit);

            // Draw center "Level" button + mini +/- buttons
            DrawMenuButtonRounded(rLevelCenter, selLevelCenter);
            DrawMenuButtonRounded(rLevelMinus,  selLevelMinus);
            DrawMenuButtonRounded(rLevelPlus,   selLevelPlus);

            // Center label
            DrawCarvedText(pieces, lblLevel, rLevelCenter, menuFontSizeF, menuSpacing, false, selLevelCenter);

            // Mini labels (+ / -) — slightly bigger usually looks nice
            float pmFont = menuFontSizeF; // or menuFontSizeF * 1.1f
            DrawCarvedText(GetFontDefault(), "-", rLevelMinus, pmFont, menuSpacing, false, selLevelMinus);
            DrawCarvedText(GetFontDefault(), "+", rLevelPlus,  pmFont, menuSpacing, false, selLevelPlus);

        }
    }
}


