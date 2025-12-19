#include "main_menu.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "world.h"
#include "utilities.h"



int ComputeMenuX(Font titleFont,
                               const char* title,
                               int titleFontSize,
                               int offset)
{
    int titleWidth = MeasureText(title, titleFontSize);
    int titleX = GetScreenWidth() / 2 - titleWidth / 2;
    return titleX + offset;
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

// MainMenu::Layout MainMenu::ComputeLayout(int screenW, int screenH, int menuX,
//                                         float baseY, float gapY, float btnW, float btnH)
// {
//     (void)screenW; (void)screenH;

//     float cx = (float)menuX + btnW * 0.5f;

//     Layout L;
//     L.start     = MakeButtonRect(cx, baseY + gapY*0.0f, btnW, btnH);
//     L.level     = MakeButtonRect(cx, baseY + gapY*1.0f, btnW, btnH);
//     L.levelName = MakeButtonRect(cx, baseY + gapY*2.0f, btnW, btnH); // display-only
//     L.fullscreen= MakeButtonRect(cx, baseY + gapY*3.0f, btnW, btnH);
//     L.quit      = MakeButtonRect(cx, baseY + gapY*4.0f, btnW, btnH);
//     return L;
// }

MainMenu::Layout MainMenu::ComputeLayout(float menuX, float baseY, float gapY, float btnW, float btnH)
{
    float cx = menuX + btnW * 0.5f;

    Layout L{};
    L.selectable[0] = MakeButtonRect(cx, baseY + gapY*0.0f, btnW, btnH); // Start
    L.selectable[1] = MakeButtonRect(cx, baseY + gapY*1.0f, btnW, btnH); // Level
    L.selectable[2] = MakeButtonRect(cx, baseY + gapY*2.0f, btnW, btnH); // Fullscreen
    L.selectable[3] = MakeButtonRect(cx, baseY + gapY*3.0f, btnW, btnH); // Quit
    L.levelName     = MakeButtonRect(cx, baseY + gapY*5.0f, btnW, btnH); // display-only

    return L;
}


static inline void DrawMenuButtonRounded(Rectangle r, bool selected, float alphaMul = 1.0f)
{
    // Orangish-beige base palette (tweak)
    Color base      = WithAlpha({214, 182, 132, 255}, alphaMul);
    Color baseHot   = WithAlpha({232, 202, 154, 240}, alphaMul);
    Color border    = WithAlpha({120,  86,  52, 220}, alphaMul);
    Color borderSel = WithAlpha({210, 170,  80, 235}, alphaMul);
    Color topHi     = WithAlpha({255, 255, 255,  35}, alphaMul);
    Color innerDark = WithAlpha({  0,   0,   0,  55}, alphaMul);

    float roundness = 0.25f;
    int   segments  = 12;



    DrawRectangleRounded(r, roundness, segments, selected ? baseHot : base);
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




static inline void DrawCarvedText(Font font, const char* text,
                                  Rectangle r, float fontSize, float spacing)
{
    // dark “burnt” letter color
    Color ink = { 100, 70, 40, 255 };

    // subtle top-left highlight (makes it look carved)
    Color hi  = { 255, 255, 255, 35 };

    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 p  = { r.x + (r.width - sz.x)*0.5f,
                   r.y + (r.height - sz.y)*0.5f };

    // Highlight first (top-left)
    DrawTextEx(font, text, { p.x - 1, p.y - 1 }, fontSize, spacing, hi);

    // Main “engraved” text (slightly down-right)
    DrawTextEx(font, text, { p.x + 1, p.y + 1 }, fontSize, spacing, ink);

    // Optional: reinforce center
    DrawTextEx(font, text, p, fontSize, spacing, ink);
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

// -------- API --------

namespace MainMenu
{
    
    MainMenu::Action Update(State& s, float dt,
                                    bool levelLoaded,
                                    int optionsCount,
                                    int& levelIndex,
                                    int levelsCount,
                                    const Layout& L)
    {

        for (int i = 0; i < 4; ++i)
        {
            if (s.pressFlash[i] > 0.0f)
            {
                s.pressFlash[i] -= dt;
                if (s.pressFlash[i] < 0.0f) s.pressFlash[i] = 0.0f;
            }
        }

        // --- Keyboard navigation (same as before) ---
        if (IsKeyPressed(KEY_UP))   s.selectedOption = (s.selectedOption - 1 + optionsCount) % optionsCount;
        if (IsKeyPressed(KEY_DOWN)) s.selectedOption = (s.selectedOption + 1) % optionsCount;

        auto TriggerPress = [&]()
        {
            int idx = s.selectedOption;
            if (idx >= 0 && idx < 4) s.pressFlash[idx] = 0.10f; // 100ms feels good
        };

        auto ActivateSelected = [&]() -> Action
        {
            switch (s.selectedOption)
            {
                case 0: return Action::StartGame;
                case 1:
                    if (levelsCount > 0) levelIndex = (levelIndex + 1) % levelsCount;
                    return Action::CycleLevel;
                case 2:
                    ToggleFullscreen();
                    isFullscreen = !isFullscreen;
                    return Action::ToggleFullscreen;
                case 3: return Action::Quit;
            }
            return Action::None;
        };

        if (IsKeyPressed(KEY_ENTER))
        {
            TriggerPress();
            return ActivateSelected();
        }

        // --- Mouse hover support ---
        Vector2 m = GetMousePosition();

        int hovered = -1;
        for (int i = 0; i < 4; ++i)
        {
            if (CheckCollisionPointRec(m, L.selectable[i]))
            {
                hovered = i;
                break;
            }
        }

        if (hovered != -1) s.selectedOption = hovered;

        if (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            TriggerPress();
            return ActivateSelected();
        }


        return Action::None;
    }


    void Draw(const State& s,
              int levelIndex,
              const LevelData* levels,
              int levelsCount)
    {
        // Backdrop (same as your code)
        Texture2D backDrop = R.GetTexture("backDrop");
        bool cover = false;
        Rectangle src  = { 0, 0, (float)backDrop.width, (float)backDrop.height };
        Rectangle dest = FitTextureDest(backDrop, GetScreenWidth(), GetScreenHeight(), cover);

        // If you want to draw it, do it here (you had it commented out):
        // DrawTexturePro(backDrop, src, dest, Vector2{0,0}, 0.0f, WHITE);

        auto& pieces = R.GetFont("Pieces");

        const char* title = "MAROONED";
        int fontSize = 128;

        int titleX = GetScreenWidth()/2 - MeasureText(title, fontSize)/2;
        int titleY = 128;
        int menuX  = titleX + 150;

        int shadowPx = std::max(1, fontSize/18);
        Color shadowCol = {0, 0, 0, 180};

        DrawTextExShadowed(pieces, title,
                           {(float)titleX, (float)titleY},
                           (float)fontSize, 0.5f, GREEN,
                           shadowPx, shadowCol);

        // --- Menu items (buttons) ---
        float menuFontSizeF = 60.0f;
        float menuSpacing   = 1.0f;
        int   menuShadowPx  = std::max(1, (int)(menuFontSizeF/18.0f));

        float baseY = 340.0f;
        float gapY  = 75.0f;                 // spacing between rows
        float btnH  = menuFontSizeF + 6.0f; // button height
        float btnW  = 320.0f;                // fixed width

        float cx = (float)menuX + btnW * 0.5f;

        // Row centers (now 5 rows because we added a display row)
        Rectangle rStart = MakeButtonRect(cx, baseY + gapY*0.0f, btnW, btnH);
        Rectangle rLevel = MakeButtonRect(cx, baseY + gapY*1.0f, btnW, btnH);
        Rectangle rFull  = MakeButtonRect(cx, baseY + gapY*2.0f, btnW, btnH);
        Rectangle rQuit  = MakeButtonRect(cx, baseY + gapY*3.0f, btnW, btnH);

        // Non-selectable display "button" for the level name
        Rectangle rLevelName = MakeButtonRect(cx, baseY + gapY*5.0f, btnW, btnH);



        // Button labels
        const char* lblStart = "Start";
        const char* lblLevel = "Level";
        const char* lblFull  = "Fullscreen";
        const char* lblQuit  = "Quit";
        
        bool selStart = (s.selectedOption == 0);
        bool selLevel = (s.selectedOption == 1);
        bool selFull  = (s.selectedOption == 2);
        bool selQuit  = (s.selectedOption == 3);

        // Example: blink highlight off for Level when pressed
        if (s.pressFlash[1] > 0.05f)
        {
            selLevel = false;
        }

        

        // Draw selectable buttons (highlight only these)
        DrawMenuButtonRounded(rStart, selStart);
        DrawMenuButtonRounded(rLevel, selLevel);
        DrawMenuButtonRounded(rFull,  selFull);
        DrawMenuButtonRounded(rQuit,  selQuit);

        // Level name display button (non-selectable, faded)
        DrawMenuButtonRounded(rLevelName, false, 1.0f);

        // Centered text

        DrawCarvedText(pieces, lblStart, rStart, menuFontSizeF, menuSpacing);
        DrawCarvedText(pieces, lblLevel, rLevel, menuFontSizeF, menuSpacing);
        DrawCarvedText(pieces, lblFull,  rFull,  menuFontSizeF, menuSpacing);
        DrawCarvedText(pieces, lblQuit,  rQuit,  menuFontSizeF, menuSpacing);

        // Level name text (centered inside its own display button)
        const char* levelName = (levels && levelsCount > 0) ? levels[levelIndex].name.c_str() : "None";

        // Optional: slightly different color so it reads as “value”
        //Color levelNameCol = YELLOW;
        Color levelNameCol = WithAlpha(BROWN, 1.0f);
        DrawCenteredShadowedText(pieces, levelName, rLevelName, menuFontSizeF, menuSpacing, levelNameCol, 1, {0,0,0});

    }
}


