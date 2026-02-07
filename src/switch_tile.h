#pragma once

#include "raylib.h"     // Color, Vector3, BoundingBox
#include <cstdint>
#include <vector>
#include "player.h"
#include "box.h"



enum class LockType {None, Gold, Silver, Skeleton, Event};

enum class TriggerMode { OnEnter, WhileHeld };

//floor switch
enum Activator : uint32_t {
    Act_Player = 1 << 0,
    Act_Box    = 1 << 1,
    Act_Fireball  = 1 << 2,
};

enum class SwitchKind {
    FloorPlate,      // visible pressure pad, overlap/held
    InvisibleTrigger,// invisible overlap trigger (your old switches)
    FireballTarget   // hit by fireball (impact), maybe wall-mounted rune
};


struct SwitchTile {
    Vector3 position;
    BoundingBox box;

    LockType lockType = LockType::None;

    TriggerMode mode = TriggerMode::OnEnter;
    uint32_t activators = Act_Box | Act_Player; // default behavior matches your current invisible switch

    SwitchKind kind = SwitchKind::FloorPlate;

    bool triggered = false;      // for OnEnter
    bool isPressed = false;      // for WhileHeld

    float delay = 0.0f;
};

extern std::vector<SwitchTile> switches;

BoundingBox MakeSwitchBox(const Vector3& pos, SwitchKind kind, float tileSize, float baseY);
bool IsSwitchPressed(const SwitchTile& st, const Player& player, const std::vector<Box>& boxes);
void DeactivateSwitch(const SwitchTile& st);
void ActivateSwitch(const SwitchTile& st);
void GenerateSwitches(float baseY);