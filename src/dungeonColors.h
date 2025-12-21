#pragma once
#include "raylib.h"
#include <string_view>
#include <optional>
#include "raymath.h"

namespace dungeon {

// All PNG-encoded legend entries.
// Note: FLOOR may not actually appear in the PNG (world is filled with floor),
// but we keep it here for completeness / clarity.
enum class Code {
    Void,               // Transparent
    Wall,               // Black (0,0,0)
    Floor,              // White (255,255,255) - informational
    LavaTile,
    PlayerStart,        // Green (0,255,0)
    Barrel,             // Blue (0,0,255)
    Skeleton,           // Red (255,0,0)
    Light,              // Yellow (255,255,0)
    Doorway,            // Purple (128,0,128)
    DoorPortal,            
    ExitTeal,           // Teal (0,128,128)
    NextLevelOrange,    // Orange (255,128,0)
    LockedDoorAqua,     // Aqua (0,255,255)
    PirateMagenta,      // Magenta (255,0,255)
    HealthPotPink,      // Pink (255,105,180)
    KeyGold,            // Gold (255,200,0)
    ChestSkyBlue,       // Sky Blue (0,128,255)
    SpiderDarkGray,     // Dark Gray (64,64,64)
    SpiderWebLightGray, // Light Gray (128,128,128)
    GiantSpider,        // Medium Gray (96, 96, 96)
    MagicStaffDarkRed,  // Dark Red (128,0,0)
    GhostVeryLightGray, // Very light grey (200,200,200)
    LauncherTrapVermillion, // Vermillion (255,66,52)
    DirectionYellowish,     // Yellowish (200,200,0)
    TimingMediumYellow1,    // (200,50,0)
    TimingMediumYellow2,    // (200,100,0)
    TimingMediumYellow3,     // (200,150,0)
    EventLocked,            // (0, 255, 128) spring green
    SlimeGreen,             // (128, 255, 0)
    Blunderbuss,             // (204, 204, 255) Periwinkle
    SecretDoor,              // (64, 0, 64) very dark purple
    SilverDoor,             // (0, 64, 64) dark cyan
    SilverKey,               // (180, 190, 205) Cool Silver
    ManaPotion,               // (0, 0, 128) dark blue
    Harpoon,                  // (96, 128, 160) //gun metal blue
};

// Exact RGB constructors (raylib Color channels are unsigned char)
constexpr Color Make(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    return Color{r, g, b, a};
}

// Exact color for each Code (constexpr friendly)
constexpr Color ColorOf(Code c) {
    switch (c) {
        case Code::Void:                   return Make(0, 0, 0, 0);          // transparent
        case Code::Wall:                   return Make(0, 0, 0);
        case Code::Floor:                  return Make(255, 255, 255);
        case Code::PlayerStart:            return Make(0, 255, 0);
        case Code::Barrel:                 return Make(0, 0, 255);
        case Code::Skeleton:               return Make(255, 0, 0);
        case Code::Light:                  return Make(255, 255, 0);
        case Code::Doorway:                return Make(128, 0, 128);
        case Code::DoorPortal:             return Make(200, 0, 200);
        case Code::ExitTeal:               return Make(0, 128, 128);
        case Code::NextLevelOrange:        return Make(255, 128, 0);
        case Code::LockedDoorAqua:         return Make(0, 255, 255);
        case Code::PirateMagenta:          return Make(255, 0, 255);
        case Code::HealthPotPink:          return Make(255, 105, 180);
        case Code::KeyGold:                return Make(255, 200, 0);
        case Code::ChestSkyBlue:           return Make(0, 128, 255);
        case Code::SpiderDarkGray:         return Make(64, 64, 64);
        case Code::GiantSpider:            return Make(96, 96, 96);
        case Code::SpiderWebLightGray:     return Make(128, 128, 128);
        case Code::MagicStaffDarkRed:      return Make(128, 0, 0);
        case Code::GhostVeryLightGray:     return Make(200, 200, 200);
        case Code::LauncherTrapVermillion: return Make(255, 66, 52);
        case Code::DirectionYellowish:     return Make(200, 200, 0);
        case Code::TimingMediumYellow1:    return Make(200, 50, 0);
        case Code::TimingMediumYellow2:    return Make(200, 100, 0);
        case Code::TimingMediumYellow3:    return Make(200, 150, 0);
        case Code::EventLocked:            return Make(0, 255, 128);
        case Code::SlimeGreen:             return Make(128, 255, 0);
        case Code::Blunderbuss:            return Make(204, 204, 255);
        case Code::SecretDoor:             return Make(64, 0, 64);
        case Code::SilverDoor:             return Make(0, 64, 64);
        case Code::SilverKey:              return Make(180, 190, 205);
        case Code::ManaPotion:             return Make(0, 0, 128);
        case Code::Harpoon:                return Make(96, 128, 160);
        case Code::LavaTile:               return Make(200, 0, 0);
    }
    // Fallback (should not happen)
    return Make(255, 255, 255);
}

// Exact-equality checker (PNG legend uses exact RGB; alpha often 255 or 0)
inline bool Equals(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
// Sometimes you may want to ignore alpha in comparisons:
inline bool EqualsRGB(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// Optional: small tolerance matcher (if you ever resave with compression/filters)
inline bool NearRGB(const Color& a, const Color& b, int tol = 0) {
    auto ad = [](unsigned char x, unsigned char y) { return (int)x - (int)y; };
    return std::abs(ad(a.r,b.r)) <= tol &&
           std::abs(ad(a.g,b.g)) <= tol &&
           std::abs(ad(a.b,b.b)) <= tol;
}

// Lightweight predicates for your most-frequent checks
inline bool IsWallColor(Color c) 
{
    // existing checks...
    if (EqualsRGB(c, ColorOf(Code::Wall))) return true;
    // etc...

    // NEW:
    if (EqualsRGB(c, ColorOf(Code::SecretDoor))) return true;

    return false;
}
inline bool IsSecretDoorColor(Color c) {
    return EqualsRGB(c, ColorOf(Code::SecretDoor)); // e.g. (64, 0, 64)
}
inline bool IsBarrelColor(const Color& c)                 { return EqualsRGB(c, ColorOf(Code::Barrel)); }
inline bool IsDoorwayColor(const Color& c)                { return EqualsRGB(c, ColorOf(Code::Doorway)); }
inline bool IsExitTeal(const Color& c)                    { return EqualsRGB(c, ColorOf(Code::ExitTeal)); }
inline bool IsNextLevelOrange(const Color& c)             { return EqualsRGB(c, ColorOf(Code::NextLevelOrange)); }
inline bool IsLockedDoorAqua(const Color& c)              { return EqualsRGB(c, ColorOf(Code::LockedDoorAqua)); }
inline bool IsKeyGold(const Color& c)                     { return EqualsRGB(c, ColorOf(Code::KeyGold)); }
inline bool IsHealthPotPink(const Color& c)               { return EqualsRGB(c, ColorOf(Code::HealthPotPink)); }
inline bool IsLauncherTrapVermillion(const Color& c)      { return EqualsRGB(c, ColorOf(Code::LauncherTrapVermillion)); }

// Name-based lookups (case-insensitive, spaces/underscores/hyphens tolerated)
std::optional<Code> CodeFromName(std::string_view name);
std::optional<Color> ColorFromName(std::string_view name);

} // namespace dungeon
