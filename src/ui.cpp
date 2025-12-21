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
#include "render_pipeline.h"
#include "main_menu.h"


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



void UpdateMenu(Camera& camera, float dt)
{
    if (currentGameState != GameState::Menu) return;

    // Keep your Escape behavior here (since it touches CameraSystem + game state)
    if (IsKeyPressed(KEY_ESCAPE) && levelLoaded)
    {
        DisableCursor();
        currentGameState = GameState::Playing;
        CameraSystem::Get().StopCinematic();
        return;
    }

    // MUST use the same constants as in main_menu.cpp
    float baseY = 375.0f;
    float gapY  = 75.0f;
    float btnW  = 320.0f;
    float btnH  = 66.0f; 
    int menuX = GetScreenWidth() / 2.0f - btnW / 2.0f;

    MainMenu::Layout layout = MainMenu::ComputeLayout(menuX, baseY, gapY, btnW, btnH);

    MainMenu::Action a = MainMenu::Update(gMenu, dt, levelLoaded, 5, levelIndex, (int)levels.size(), layout);

    if (a == MainMenu::Action::StartGame)

    {
        
        InitLevel(levels[levelIndex], camera);
        currentGameState = GameState::Playing;
        levelLoaded = true;
    }
    else if (a == MainMenu::Action::Quit)
    {
        currentGameState = GameState::Quit;
    }
}


void DrawTimer(float ElapsedTime){
    int minutes = (int)(ElapsedTime / 60.0f);
    int seconds = (int)ElapsedTime % 60;

    char buffer[16];
    sprintf(buffer, "Time: %02d:%02d", minutes, seconds);

    DrawText(buffer, GetScreenWidth()-150, 30, 20, WHITE); 
}

void DrawKeySlotUI(const Player& player)
{
    Rectangle slot = { 20, 44, 64, 64 }; 
    Rectangle slot2 = {20, 128, 64, 64};
    // Slot background + border
    DrawRectangleRec(slot, (Color){20,20,20,200});
    DrawRectangleLinesEx(slot, 2, (Color){200,180,120,255});

    DrawRectangleRec(slot2, (Color){20,20,20,200});
    DrawRectangleLinesEx(slot2, 2, (Color){200,180,120,255});

    // Only show key icon if you have it
    if (player.hasGoldKey) {
        Texture2D keyIcon = ResourceManager::Get().GetTexture("keyTexture"); 
        DrawTexturePro(
            keyIcon,
            Rectangle{0,0,(float)keyIcon.width,(float)keyIcon.height},
            Rectangle{slot.x-2, slot.y-2, slot.width+4, slot.height+4},
            Vector2{0,0},
            0.0f,
            WHITE
        );
    }

    // Only show key icon if you have it
    if (player.hasSilverKey) {
        Texture2D keyIcon = ResourceManager::Get().GetTexture("silverKey"); 
        DrawTexturePro(
            keyIcon,
            Rectangle{0,0,(float)keyIcon.width,(float)keyIcon.height},
            Rectangle{slot2.x-2, slot2.y-2, slot2.width+4, slot2.height+4},
            Vector2{0,0},
            0.0f,
            WHITE
        );
    }
}


