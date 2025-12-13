#include "hintManager.h"
#include <algorithm>
#include <sstream>
#include "world.h"

// ------------------------- Utilities -------------------------
static float clampf(float x, float a, float b) { return (x < a) ? a : (x > b) ? b : x; }

// ------------------------- Public API -------------------------
HintManager::HintManager()
: currentIndex(-1), //stays at -1 in dungeons. use setMessage()
  hasOverride(false),
  anchor{0.5f, 0.85f},
  maxWidthFrac(0.70f),
  paddingPx(12.0f),
  fontScale(0.025f), // ~3% of screen height
  textColor{255,255,255,255},
  bgColor{0,0,0,0},
  shadowColor{0,0,0,200},
  fadeInSpeed(100.0f),
  fadeOutSpeed(100.0f),
  alpha(0.0f)
{}

void HintManager::AddHint(const std::string& text) {
    hints.push_back(text);
    if (currentIndex < 0) currentIndex = 0;
}

void HintManager::Reset() {
    hasOverride = false;
    overrideMessage.clear();
    currentIndex = -1;
    SetMessage("");
}

void HintManager::Advance() {
    if (hasOverride) {
        hasOverride = false;
        overrideMessage.clear();
        return;
    }
    if (currentIndex >= 0 && currentIndex + 1 < (int)hints.size()) {
        currentIndex++;
    } else {
        currentIndex = -1; // no more hints
    }
}

void HintManager::SetLetterSpacing(float px){
    letterSpacing = px;
}

void HintManager::SetMessage(const std::string& text) {
    overrideMessage = text;
    hasOverride = true;
}

void HintManager::Clear() {
    hasOverride = false;
    overrideMessage.clear();
    currentIndex = -1;
}

int HintManager::GetCurrentIndex() const {
    return currentIndex;
}

int HintManager::GetHintCount() const {    
    return (int)hints.size();
}

void HintManager::Update(float dt) {
    const bool targetVisible = HasActiveHint();
    const float target = targetVisible ? 1.0f : 0.0f;
    const float speed = targetVisible ? fadeInSpeed : fadeOutSpeed;
    alpha += (target - alpha) * clampf(speed * dt, 0.0f, 1.0f);
    alpha = clampf(alpha, 0.0f, 1.0f);


}

void HintManager::UpdateTutorial(){

    if (player.isMoving && currentIndex == 0){//movement check
        Advance();
    }
    Vector2 delta = GetMouseDelta();
    if (currentIndex == 1 && delta.x != 0 && delta.y != 0){ //mouse check
        Advance();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && currentIndex == 2){ //fire or swing check
        Advance();
    }

    if (IsKeyPressed(KEY_Q) && currentIndex == 3){ //switch weapon check
        Advance();
    }

    if (player.activeWeapon == WeaponType::Sword && player.blocking && currentIndex == 4){ //block check
        Advance();
    }

    if (currentIndex == 5 && !player.grounded){ //jump check
        Advance();
    }

    if (currentIndex == 6 && IsKeyPressed(KEY_LEFT_SHIFT)){ //run check
        Advance();
    }

    //checked even in dungeons, should work as a cue to use a health pot. 
    if (player.currentHealth < 30 && currentIndex == -1 && player.inventory.HasItem("HealthPotion")){ //Low health pot check
        SetMessage("PRESS F TO USE HEALTH POTION");
    }

    if (player.currentMana < 30 && currentIndex == -1 && player.inventory.HasItem("ManaPotion")){
        SetMessage("Press G TO US MANA POTION");
        
    }

    if (player.deathTimer >= 1.4){
        Clear(); //erase message if you die before taking health potion. 
    }

    if (IsKeyPressed(KEY_ONE) && currentIndex == -1){ //clear on use healthpot
        Clear(); //clears message and override and sets current index to -1
    }

    if (IsKeyPressed(KEY_E)){ //clears tutorial
        Clear();
        
    }

    if (IsKeyPressed(KEY_F)){
        Clear();
    }

    if (IsKeyPressed(KEY_G)){
        Clear();
    }
    
    if (!isDungeon){ // hint e to interact when close to dungeon entrance. -it should disappear when further away.
        DungeonEntrance& e = dungeonEntrances[0];
        float distance = Vector3Distance(player.position, e.position);
        if (distance < 500 && currentIndex < 0){
            SetMessage("E TO INTERACT");
        }

    }

}

void HintManager::Draw() const {
    if (alpha <= 0.01f) return;

    const std::string msg = GetCurrentHint();
    if (msg.empty()) return;

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    Font font = R.GetFont("Kingthings");//GetFontDefault();
    const float fontPx = clampf(fontScale * (float)sh, 16.0f, 100.0f);
    //const float spacing = 1.0f;

    const float maxWidthPx = std::max(64.0f, maxWidthFrac * (float)sw);
    const std::string wrapped = WrapText(msg, font, fontPx, letterSpacing, maxWidthPx);
    Vector2 textSize = MeasureMultiline(wrapped, font, fontPx, letterSpacing);

    // Box rect centered around anchor
    const float cx = anchor.x * (float)sw;
    const float cy = anchor.y * (float)sh;
    const float boxW = textSize.x + paddingPx * 2.0f;
    const float boxH = textSize.y + paddingPx * 2.0f;
    const float x = cx - boxW * 0.5f;
    const float y = cy - boxH * 0.5f;

    // Background
    Color bg = bgColor; bg.a = (unsigned char)(bg.a * alpha);
    DrawRectangleRounded({ x, y, boxW, boxH }, 0.15f, 8, bg);

    // Text position (top-left inside padding)
    Vector2 pos{ x + paddingPx, y + paddingPx };

    // Colors with alpha baked in
    Color shadow = { 0, 0, 0, (unsigned char)(220 * alpha) };        // near-black shadow
    Color text   = { 255, 255, 255, (unsigned char)(255 * alpha) };  // white (or your UI color)

    // Pixel-snap positions helps avoid bleed:
    Vector2 shadowPos{ floorf(pos.x + 2.0f), floorf(pos.y + 2.0f) };
    Vector2 textPos  { floorf(pos.x),        floorf(pos.y)        };

    // Draw shadow, then text
    DrawMultilineText(wrapped, font, floorf(fontPx), floorf(letterSpacing), shadowPos, shadow, 1.15f);
    DrawMultilineText(wrapped, font, floorf(fontPx), floorf(letterSpacing), textPos,   text,   1.15f);
}

bool HintManager::HasActiveHint() const {
    if (hasOverride && !overrideMessage.empty()) return true;
    return (currentIndex >= 0 && currentIndex < (int)hints.size());
}

std::string HintManager::GetCurrentHint() const {
    if (hasOverride) return overrideMessage;
    if (currentIndex >= 0 && currentIndex < (int)hints.size()) return hints[currentIndex];
    return std::string();
}

void HintManager::SetAnchor(Vector2 normalizedAnchor) {
    anchor.x = clampf(normalizedAnchor.x, 0.0f, 1.0f);
    anchor.y = clampf(normalizedAnchor.y, 0.0f, 1.0f);
}

void HintManager::SetMaxWidthFraction(float f) {
    maxWidthFrac = clampf(f, 0.2f, 1.0f);
}

void HintManager::SetPadding(float px) {
    paddingPx = std::max(0.0f, px);
}

void HintManager::SetFontScale(float s) {
    fontScale = clampf(s, 0.015f, 0.06f); // guardrails: 1.5%..6% of screen height
}

void HintManager::SetColors(Color text, Color bg, Color shadow) {
    textColor = text;
    bgColor = bg;
    shadowColor = shadow;
}

void HintManager::SetFadeSpeeds(float fadeInPerSec, float fadeOutPerSec) {
    fadeInSpeed = std::max(0.0f, fadeInPerSec);
    fadeOutSpeed = std::max(0.0f, fadeOutPerSec);
}

// ------------------------- Private helpers -------------------------

// Simple greedy word-wrapping that respects MeasureTextEx width
std::string HintManager::WrapText(const std::string& text, Font font, float fontSize, float spacing, float maxWidth) const {
    std::istringstream iss(text);
    std::string word;
    std::string line;
    std::string out;

    auto widthOf = [&](const std::string& s) {
        return MeasureTextEx(font, s.c_str(), fontSize, spacing).x;
    };

    while (iss >> word) {
        std::string test = line.empty() ? word : (line + " " + word);
        if (widthOf(test) <= maxWidth) {
            line = std::move(test);
        } else {
            if (!out.empty()) out += '\n';
            if (line.empty()) {
                // Single very long word: hard break
                out += word;
                line.clear();
            } else {
                out += line;
                line = word;
            }
        }
    }
    if (!line.empty()) {
        if (!out.empty()) out += '\n';
        out += line;
    }
    return out;
}

Vector2 HintManager::MeasureMultiline(const std::string& text, Font font, float fontSize, float spacing) const {
    float maxW = 0.0f;
    float totalH = 0.0f;
    std::istringstream ss(text);
    std::string line;
    const float lineGap = fontSize * 0.25f;

    while (std::getline(ss, line)) {
        Vector2 m = MeasureTextEx(font, line.c_str(), fontSize, spacing);
        maxW = std::max(maxW, m.x);
        totalH += m.y + lineGap;
    }
    if (totalH > 0.0f) totalH -= lineGap; // no extra gap after last line
    return { maxW, totalH };
}

void HintManager::DrawMultilineText(const std::string& text, Font font, float fontSize, float spacing, Vector2 pos, Color tint, float alphaScale) const {
    std::istringstream ss(text);
    std::string line;
    float y = pos.y;
    const float lineGap = fontSize * 0.25f;

    //texColor no longer used. pass the colors when you call this function
    Color col = tint;
    //col.a = (unsigned char)(col.a * alphaScale); // no alpha, just white text and black shadow. 

    while (std::getline(ss, line)) {
        DrawTextEx(font, line.c_str(), { pos.x, y }, fontSize, spacing, col);
        Vector2 m = MeasureTextEx(font, line.c_str(), fontSize, spacing);
        y += m.y + lineGap;
    }
}
