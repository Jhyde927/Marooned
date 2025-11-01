// ui.cpp
#include "ui.h"
#include <cmath>
#include "raymath.h"
#include "level.h"
#include "resourceManager.h"
#include "weapon.h"
#include "world.h"
#include "utilities.h"
#include "camera_system.h"


static HintManager hints;   // one global-ish instance, private to UI.cpp

void TutorialSetup(){
    if (!first){
        hints.SetMessage("");
    }
    else{
        hints.AddHint("WASD TO MOVE");
        hints.AddHint("MOVE MOUSE TO LOOK");
        hints.AddHint("LEFT CLICK TO ATTACK");
        hints.AddHint("Q TO SWITCH WEAPONS");
        hints.AddHint("RIGHT CLICK TO BLOCK");
        hints.AddHint("SPACEBAR TO JUMP");
        hints.AddHint("HOLD SHIFT TO RUN");
        
        //setmessage E TO INTERACT when you encounter first dungeon entrance
        //Also set message to 1 TO USE HEALTH POTION when health is low and you have a health pot. This should happen every time.
        //see hintmanager.cpp update tutuorial
        hints.SetAnchor({0.5f, 0.85f});         // bottom-center
        hints.SetMaxWidthFraction(0.7f);        // wrap at 70% of screen width
        hints.SetFontScale(0.04f);             // ~3% of screen height
        hints.SetLetterSpacing(5.0f);           // pixels
        hints.SetFadeSpeeds(2.0f, 2.0f);        // fadeIn/fadeOut
        hints.SetColors(WHITE, Color {0, 0, 0, 0}, BLACK);

    }

}

void AdvanceHint(){
    
    hints.Advance();
}

void UpdateHintManager(float deltaTime){
    if (!playerInit) return;

    hints.Update(deltaTime);
    hints.UpdateTutorial();
    
}

void DrawHints(){

    hints.Draw();
}

void DrawMagicIcon(){
    //magic staff selected spell icon. 
    Texture2D currentTexture;

    if (magicStaff.magicType == MagicType::Fireball) {
        currentTexture = R.GetTexture("fireIcon");
    } else if (magicStaff.magicType == MagicType::Iceball) {
        currentTexture = R.GetTexture("iceIcon");
    }

    int targetSize = 64;
    int marginX = 314; // distance from right screen edge
    int marginY = GetScreenHeight() - targetSize - 16;
    // Source rect: crop entire original texture
    Rectangle src = { 0.0f, 0.0f, (float)currentTexture.width, (float)currentTexture.height };

    // Destination rect: bottom-left corner, scaled to 64x64
    Rectangle dest = {
        (float)marginX,
        (float)marginY,
        (float)targetSize,
        (float)targetSize
    };

    Vector2 origin = { 0.0f, 0.0f }; // top-left origin

    DrawTexturePro(currentTexture, src, dest, origin, 0.0f, WHITE);
}

// Fills a trapezoid up to t (0..1) with a slanted side preserved.
// TL,TR,BR,BL define the full bar polygon in screen space.
void DrawTrapezoidFill(Vector2 TL, Vector2 TR, Vector2 BR, Vector2 BL, float t, Color colFill, Color colBack) {
    // back
    DrawTriangle(TL, TR, BR, colBack);
    DrawTriangle(TL, BR, BL, colBack);

    // current right edge obtained by interpolating along top/bottom edges
    Vector2 CUR_TOP = LerpV2(TL, TR, t);
    Vector2 CUR_BOT = LerpV2(BL, BR, t);

    // filled quad: TL -> CUR_TOP -> CUR_BOT -> BL
    DrawTriangle(TL, CUR_TOP, CUR_BOT, colFill);
    DrawTriangle(TL, CUR_BOT, BL,     colFill);
}


void DrawTrapezoidBar(float x, float y, float value, float maxValue, const BarStyle& style) {
    //reusable function for making trapezoid health/mana/stamina bars. 
    float t = (maxValue > 0.0f) ? Clamp01(value / maxValue) : 0.0f;

    // Build trapezoid corners (vertical on one side, slanted on the other)
    Vector2 TL, TR, BR, BL;
    if (style.slantSide == SlantSide::Right) {
        TL = { x,               y };
        BL = { x,               y + style.height };
        TR = { x + style.width, y };
        BR = { x + style.width - style.slant, y + style.height };
    } else { // left slanted
        TL = { x + style.slant, y };
        BL = { x,               y + style.height };
        TR = { x + style.width, y };
        BR = { x + style.width, y + style.height };
    }

    // Drop shadow (simple offset re-draw)
    if (style.shadow) {
        Vector2 sTL = {TL.x + style.shadowOffset.x, TL.y + style.shadowOffset.y};
        Vector2 sTR = {TR.x + style.shadowOffset.x, TR.y + style.shadowOffset.y};
        Vector2 sBR = {BR.x + style.shadowOffset.x, BR.y + style.shadowOffset.y};
        Vector2 sBL = {BL.x + style.shadowOffset.x, BL.y + style.shadowOffset.y};
        DrawTrapezoidFill(sTL, sTR, sBR, sBL, 1.0f, ColorAlpha(BLACK, style.shadowAlpha), BLANK);
    }

    // Base fill color from low→high gradient
    Color fill = ColorLerpFast(style.lowColor, style.highColor, t);

    // Optional low-HP (or low resource) pulse
    if (style.pulseWhenLow && t < style.pulseThreshold) {
        float p = sinf(GetTime()*style.pulseRate)*0.5f + 0.5f; // 0..1
        fill = ColorLerpFast(fill, style.pulseTarget, p*0.6f);
    }

    // Draw bar (back + filled portion)
    DrawTrapezoidFill(TL, TR, BR, BL, t, fill, style.back);

    // Optional gloss (top band)
    if (style.gloss) {
        float gh = style.height * 0.45f;
        // a simple rounded rect gloss looks good enough on top
        Rectangle gloss = { fminf(TL.x, BL.x), y, style.width - 2, gh };
        DrawRectangleRounded(gloss, 0.85f, 8, ColorAlpha(WHITE, 0.25f));
    }

    // Outline
    DrawLineEx(TL, TR, style.outlineThickness, style.outline);
    DrawLineEx(TR, BR, style.outlineThickness, style.outline);
    DrawLineEx(BR, BL, style.outlineThickness, style.outline);
    DrawLineEx(BL, TL, style.outlineThickness, style.outline);
}

void DrawHUDBars(const Player& player) {

    float stamina = player.stamina;
    float staminaMax = player.maxStamina;
    float mana = player.currentMana;
    float manaMax = player.maxMana;

    // ---------- persistent display values ----------
    static bool  init = true;
    static float hpDisp   = 0.0f;
    static float manaDisp = 0.0f;
    static float stamDisp = 0.0f;

    float dt = GetFrameTime();

    if (init) { //Start the game with bars filled
        hpDisp   = (float)player.currentHealth;
        manaDisp = mana;
        stamDisp = stamina;
        init = false; //now lerp the bars if the values change. 
    }

    // Clamp targets in case something goes out of range
    float hpTarget   = (float)player.currentHealth;
    hpTarget   = fminf(hpTarget, (float)player.maxHealth);
    float manaTarget = fminf(mana,    manaMax);
    float stamTarget = fminf(stamina, staminaMax);

    // ---------- smoothing ----------
    // Pick lambdas per bar (feel tuning):
    hpDisp   = LerpExp(hpDisp,   hpTarget,   12.0f,  dt);   
    manaDisp = LerpExp(manaDisp, manaTarget, 12.0f, dt);   
    stamDisp = LerpExp(stamDisp, stamTarget, 18.0f, dt);   

    //snap when super close to kill shimmer
    auto snapClose = [](float &v, float t, float eps){ if (fabsf(v - t) < eps) v = t; };
    snapClose(hpDisp,   hpTarget,   0.1f);
    snapClose(manaDisp, manaTarget, 0.1f);
    snapClose(stamDisp, stamTarget, 0.1f);

    // Health (full height)
    BarStyle hp;
    hp.width   = 300.0f;
    hp.height  = 15.0f;         
    hp.slant   = 14.0f;
    hp.slantSide = SlantSide::Right;
    hp.lowColor  = (Color){220,40,40,255};
    hp.highColor = (Color){50,220,90,255};
    hp.pulseWhenLow = true;
    hp.outlineThickness = 2.0f;
    hp.outline = hp.highColor;

    // Mana 
    BarStyle manaBar = hp;
    manaBar.height  = hp.height;     
    manaBar.slant   = hp.slant;       
    manaBar.lowColor  = (Color){0,150,255,120};
    manaBar.highColor = (Color){0,150,255,255};
    manaBar.pulseWhenLow = false;
    manaBar.outlineThickness = 1.5f;         
    manaBar.outline = manaBar.highColor;

    // Stamina 
    BarStyle stam = hp;
    stam.height  = hp.height;
    stam.slant   = hp.slant;
    stam.lowColor  = (Color){200,200,200, 120};
    stam.highColor = (Color){200,200,200,255};
    stam.pulseWhenLow = false;
    stam.outlineThickness = 1.5f;
    stam.outline = stam.highColor;

    //Position the bars on screen
    float baseY   = GetScreenHeight() - 80.0f; // top of the stack, aligned with inventory
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float xCenter = 550;
 
    // Vertical spacing between bars 
    float gap = 8.0f;

    // Compute left x so bars are centered around xCenter
    auto leftX = [&](float width){ return xCenter - width * 0.5f; };

    // Order: Health (top) → Mana (middle) → Stamina (bottom), all slanted RIGHT
    float yHP     = baseY;
    float yMana   = yHP   + hp.height   + gap;
    float yStam   = yMana + manaBar.height + gap;

    float flash = Clamp01(player.hitTimer / 0.15f);   
    // Shift the whole gradient toward red for the flash duration, when taking damage
    hp.lowColor  = ColorLerpFast(hp.lowColor,  RED, flash);
    hp.highColor = ColorLerpFast(hp.highColor, RED, flash);

    DrawTrapezoidBar(leftX(hp.width),   yHP,   hpDisp,   (float)player.maxHealth, hp);
    DrawTrapezoidBar(leftX(manaBar.width), yMana, manaDisp, manaMax,manaBar);
    DrawTrapezoidBar(leftX(stam.width), yStam, stamDisp, staminaMax,stam);
}

static Rectangle FitTextureDest(const Texture2D& tex, int screenW, int screenH, bool cover)
{
    float sw = (float)screenW, sh = (float)screenH;
    float tw = (float)tex.width, th = (float)tex.height;

    float scale = cover ? fmaxf(sw/tw, sh/th)  // fill (crop)
                        : fminf(sw/tw, sh/th); // fit (bars)

    float dw = tw * scale;
    float dh = th * scale;
    float dx = (sw - dw) * 0.5f;  // center
    float dy = (sh - dh) * 0.5f;

    // (optional) avoid subpixel blur
    dx = floorf(dx); dy = floorf(dy); dw = floorf(dw); dh = floorf(dh);

    return Rectangle{ dx, dy, dw, dh };
}


inline void DrawTextExShadowed(Font font,
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

inline void DrawTextExShadowed(Font font, const std::string& s, Vector2 pos,
                               float fontSize, float spacing, Color color,
                               int shadowPx = -1, Color shadowCol = {0,0,0,190})
{
    DrawTextExShadowed(font, s.c_str(), pos, fontSize, spacing, color, shadowPx, shadowCol);
}

void DrawMenu(int selectedOption, int levelIndex) {

    ClearBackground(BLACK);
   
    Texture2D backDrop = R.GetTexture("backDrop");
    // choose contain or cover
    bool cover = false; // true = fill screen (crop), false = fit inside (letterbox)

    Rectangle src  = { 0, 0, (float)backDrop.width, (float)backDrop.height };
    Rectangle dest = FitTextureDest(backDrop, GetScreenWidth(), GetScreenHeight(), cover);

    if (fade <= 0) DrawTexturePro(backDrop, src, dest, Vector2{0,0}, 0.0f, WHITE);

    const char* title = "MAROONED";
    int fontSize = 128;

    // center X
    int titleX = GetScreenWidth()/2 - MeasureText(title, fontSize)/2;
    int titleY = 128;
    int menuX = titleX+100;
    // shadow settings
    int shadowPx = std::max(1, fontSize/18);          // scales with size (≈3–4 px here)
    Color shadowCol = {0, 0, 0, 180};                 // semi-transparent black

    auto& pieces = R.GetFont("Pieces"); 
  

    // draw shadow, then title
    DrawTextEx(pieces, title, Vector2 {(float)titleX+ shadowPx, (float)titleY+shadowPx}, fontSize, 1.0f, shadowCol);
    DrawTextEx(pieces, title, Vector2 {(float)titleX, (float)titleY}, fontSize, 1.0f, GREEN);
    

    // --- Title ---
    float titleFontSize = (float)fontSize;       // your computed size
    float titleSpacing  = 0.5f;                  // tweak if you want looser kerning
    DrawTextExShadowed(pieces, title,
                    {(float)titleX, (float)titleY},
                    titleFontSize, titleSpacing, GREEN,
                    shadowPx, shadowCol);

    // --- Menu items ---
    float menuFontSizeF = 60.0f;
    float menuSpacing   = 1.0f;
    int   menuShadowPx  = std::max(1, (int)(menuFontSizeF/18.0f));
    float verticalSpacing = 75.0f;

    char startBuf[64];
    snprintf(startBuf, sizeof(startBuf), "%sStart", (selectedOption==0?"> ":"  "));
    DrawTextExShadowed(pieces, startBuf, {(float)menuX, 300.0f},
                    menuFontSizeF, menuSpacing, WHITE, menuShadowPx, shadowCol);

    std::string levelLine = TextFormat("%s Level: %s",
                                    (selectedOption==1?">":" "),
                                    levels[levelIndex].name.c_str());

    DrawTextExShadowed(pieces, levelLine, {(float)menuX, 360.0f},
                    menuFontSizeF, menuSpacing, YELLOW, menuShadowPx, shadowCol);

    char fullBuf[64];
    const char* label = "Toggle Fullscreen";//isFullscreen ? "windowed" : "fullscreen";  
    snprintf(fullBuf, sizeof(fullBuf), "%s%s",
            (selectedOption == 2 ? "> " : "  "), label);

    DrawTextExShadowed(pieces, fullBuf, {(float)menuX, 420.0f},
                    menuFontSizeF, menuSpacing, WHITE, menuShadowPx, shadowCol);

    char quitBuf[64];
    snprintf(quitBuf, sizeof(quitBuf), "%sQuit", (selectedOption==3?"> ":"  "));
    DrawTextExShadowed(pieces, quitBuf, {(float)menuX, 480.0f},
                    menuFontSizeF, menuSpacing, WHITE, menuShadowPx, shadowCol);


    float alpha = 1.0f; // 0..1
    if (fade > 0) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
    //when we switch from menu, it was flashing the menu screen for 1 frame durring fade out. 
    //So draw a black rectangle over the menu when fading. For some reason this function needs to run for the lights to work. 

}

void UpdateMenu(Camera& camera){
    //Main Menu - level select 
    int optionsCount = 4;
    if (currentGameState == GameState::Menu) {

        if (IsKeyPressed(KEY_ESCAPE) && levelLoaded) currentGameState = GameState::Playing;
        if (IsKeyPressed(KEY_UP)) selectedOption = (selectedOption - 1 + optionsCount) % optionsCount; //loop
        if (IsKeyPressed(KEY_DOWN)) selectedOption = (selectedOption + 1) % optionsCount;

        if (IsKeyPressed(KEY_ENTER)) {
            if (selectedOption == 0) {
                InitLevel(levels[levelIndex], camera);
                currentGameState = GameState::Playing;
                levelLoaded = true;
            } else if (selectedOption == 1) {
                levelIndex = (levelIndex + 1) % levels.size(); // Cycle through levels
            } else if (selectedOption == 2) {
                ToggleFullscreen();
                if (!isFullscreen){
                    isFullscreen = true;
                }else{
                    isFullscreen = false;
                }

            } else if (selectedOption == 3) {
                currentGameState = GameState::Quit;
            }
        }

    }
}

void DrawTimer(float ElapsedTime){
    int minutes = (int)(ElapsedTime / 60.0f);
    int seconds = (int)ElapsedTime % 60;

    char buffer[16];
    sprintf(buffer, "Time: %02d:%02d", minutes, seconds);

    DrawText(buffer, GetScreenWidth()-150, 30, 20, WHITE); 
}

