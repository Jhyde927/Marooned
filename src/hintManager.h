#pragma once

#include <string>
#include <vector>
#include "raylib.h"



class HintManager {
public:
    HintManager();

    // Sequence management
    void AddHint(const std::string& text);
    void Reset();          // Clears override and resets to first hint (if any)
    void Advance();        // Advances to next queued hint; clears override if active

    // Override message (not part of the queue)
    void SetMessage(const std::string& text); // Shows immediately (overrides queue)
    void Clear();                              // Hides any message

    // Frame hooks
    void Update(float dt);
    void Draw() const;

    // Queries
    bool HasActiveHint() const;
    std::string GetCurrentHint() const;
    void UpdateTutorial();
   
    void SetAnchor(Vector2 normalizedAnchor);      // default {0.5f, 0.85f}
    void SetMaxWidthFraction(float f);             // 0..1, default 0.70f
    void SetPadding(float px);                     // default 12
    void SetFontScale(float s);                    // relative to screen height; default 0.030 (~3% of H)
    void SetColors(Color text, Color bg, Color shadow);
    void SetLetterSpacing(float px); // default 1.0f

    int GetCurrentIndex() const;
    int GetHintCount() const;  

    // Fade speeds (units: 1/sec). Higher = snappier.
    void SetFadeSpeeds(float fadeInPerSec, float fadeOutPerSec);

private:
    // Data
    std::vector<std::string> hints;
    int currentIndex;
    std::string overrideMessage;
    bool hasOverride;
    bool harpoonPickup;
    // Visual config
    Vector2 anchor;           // normalized [0..1] of screen
    float maxWidthFrac;       // fraction of screen width for wrapping
    float paddingPx;          // box padding
    float fontScale;          // % of screen height -> font px
    Color textColor;
    Color bgColor;
    Color shadowColor;
    float fadeInSpeed;
    float fadeOutSpeed;
    float letterSpacing;

    // State
    float alpha;              // 0..1 current visibility

    // Helpers
    std::string WrapText(const std::string& text, Font font, float fontSize, float spacing, float maxWidth) const;
    Vector2 MeasureMultiline(const std::string& text, Font font, float fontSize, float spacing) const;
    void DrawMultilineText(const std::string& text, Font font, float fontSize, float spacing, Vector2 pos, Color tint, float alpha) const;
};
