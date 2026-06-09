#pragma once

#include "raylib.h"

#include <string>
#include <vector>

namespace DebugConsole
{
    void Init();
    void Update(float dt);
    void Draw();

    void Toggle();
    void Open();
    void Close();

    bool IsOpen();

    // Lets other systems print to the console later if needed.
    void Log(const std::string& message);

    // Placeholder command hooks.
    // You can replace the bodies in debug_console.cpp with real game effects.
    void CommandSky(float duration); //default durration
    void CommandGod();
    void CommandVegetation();
    void CommandPosition();
    void CommandHealth(int amount);
    void CommandMana(int amount);
    void CommandDoors();
    void CommandStamina();
    void CommandEnemies();
    void CommandStart();
    void CommandEnd();
    void CommandKill();
    void CommandKeys();
    void CommandQuadDamage();

    void CommandHaste();
    void CommandOverHealth();
    void CommandStats();
    void CommandProps();
    void CommandFreecam();
    void CommandCeiling();
    void CommandWeapons();
    void CommandClear();
    void CommandExit();


}