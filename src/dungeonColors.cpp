#include "dungeonColors.h"
#include <unordered_map>
#include <string>
#include <algorithm>

namespace dungeon {

static std::string normalize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == ' ' || c == '_' || c == '-') continue;
        out.push_back((char)std::tolower((unsigned char)c));
    }
    return out;
}

// Canonical names you can use in tools/debug UI.
// Add aliases freely (e.g., "door", "purpledoor", etc.)
static const std::unordered_map<std::string, Code> kNameToCode = {
    {"void",                    Code::Void},
    {"transparent",             Code::Void},
    {"wall",                    Code::Wall},
    {"black",                   Code::Wall},

    {"floor",                   Code::Floor},
    {"white",                   Code::Floor},

    {"playerstart",             Code::PlayerStart},
    {"start",                   Code::PlayerStart},
    {"green",                   Code::PlayerStart},

    {"barrel",                  Code::Barrel},
    {"blue",                    Code::Barrel},

    {"skeleton",                Code::Skeleton},
    {"red",                     Code::Skeleton},

    {"light",                   Code::Light},
    {"yellow",                  Code::Light},

    {"doorway",                 Code::Doorway},
    {"purple",                  Code::Doorway},

    {"exitteal",                Code::ExitTeal},
    {"teal",                    Code::ExitTeal},

    {"nextlevelorange",         Code::NextLevelOrange},
    {"orange",                  Code::NextLevelOrange},

    {"lockeddooraqua",          Code::LockedDoorAqua},
    {"aqua",                    Code::LockedDoorAqua},
    {"cyan",                    Code::LockedDoorAqua},

    {"piratemagenta",           Code::PirateMagenta},
    {"magenta",                 Code::PirateMagenta},

    {"healthpotpink",           Code::HealthPotPink},
    {"pink",                    Code::HealthPotPink},

    {"keygold",                 Code::KeyGold},
    {"gold",                    Code::KeyGold},

    {"chestskyblue",            Code::ChestSkyBlue},
    {"skyblue",                 Code::ChestSkyBlue},

    {"spiderdarkgray",          Code::SpiderDarkGray},
    {"darkgray",                Code::SpiderDarkGray},

    {"giantSpider",             Code::GiantSpider},

    {"eventLocked",             Code::EventLocked},

    {"spiderweblightgray",      Code::SpiderWebLightGray},
    {"lightgray",               Code::SpiderWebLightGray},

    {"magicstaffdarkred",       Code::MagicStaffDarkRed},
    {"darkred",                 Code::MagicStaffDarkRed},

    {"ghostverylightgray",      Code::GhostVeryLightGray},
    {"ghost",                   Code::GhostVeryLightGray},

    {"launchertrapvermillion",  Code::LauncherTrapVermillion},
    {"vermillion",              Code::LauncherTrapVermillion},

    {"directionyellowish",      Code::DirectionYellowish},
    {"yellowish",               Code::DirectionYellowish},

    {"timingmediumyellow1",     Code::TimingMediumYellow1},
    {"timing1",                 Code::TimingMediumYellow1},

    {"timingmediumyellow2",     Code::TimingMediumYellow2},
    {"timing2",                 Code::TimingMediumYellow2},

    {"timingmediumyellow3",     Code::TimingMediumYellow3},
    {"timing3",                 Code::TimingMediumYellow3},
};

std::optional<Code> CodeFromName(std::string_view name) {
    const std::string key = normalize(name);
    auto it = kNameToCode.find(key);
    if (it == kNameToCode.end()) return std::nullopt;
    return it->second;
}

std::optional<Color> ColorFromName(std::string_view name) {
    if (auto c = CodeFromName(name)) {
        return ColorOf(*c);
    }
    return std::nullopt;
}

} // namespace dungeon
