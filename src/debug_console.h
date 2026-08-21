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

    void CommandSky(float duration); //default durration
    void CommandGod();
    void CommandVegetation();
    void CommandPosition();
    void CommandChangeLevel(int idx);
    void CommandHealth(int amount);
    void CommandMana(int amount);
    void CommandDoors();
    void CommandOpenDoors();
    void CommandStamina();
    void CommandForceAgro();
    void CommandEnemies();
    void CommandStart();
    void CommandEnd();
    void CommandKill();
    void CommandThirdPerson();
    void CommandKeys();
    void CommandQuadDamage();
    void CommandUnlockJournal();
    void CommandUnlockLevels();
    void CommandUnlockEverything();
    void CommandFog();
    void CommandHaste();
    void CommandOverHealth();
    void CommandDoubleShot();
    void CommandStats();
    void CommandProps();
    void CommandFreecam();
    void CommandCeiling();
    void CommandWeapons();
    void CommandFreezeAI();
    void CommandUnlockRaft();
    void CommandClearSave();
    void CommandClear();
    void CommandExit();

}