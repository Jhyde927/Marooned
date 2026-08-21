#pragma once

#include <vector>
#include "powerUps.h"

struct SaveData
{
    int version = 1;
    int levelIndex = 0;

    // Inventory
    int healthPotions = 0;
    int manaPotions = 0;
    int gold = 0;

    // Weapons and upgrades
    bool swordUnlocked = true;
    bool crossbowUnlocked = false;
    bool blunderbussUnlocked = false;
    bool magicStaffUnlocked = false;
    bool harpoonUnlocked = false;
    bool doubleShotUnlocked = false;
    bool iceMagicUnlocked = false;

    bool raftBodyUnlocked = false;
    bool raftMastUnlocked = false;
    bool raftSailUnlocked = false;

    //Powerups
    int currentPowerUp = static_cast<int>(PowerUpType::None);

    //world
    bool entrancesUnlocked = false;

    std::vector<int> discoveredJournal;
    std::vector<int> discoveredCreatures;

    std::vector<int> discoveredLevels = { 0 };
};

extern SaveData save;

namespace SaveGame
{
    SaveData Load();
    void Save(const SaveData& data);

    void StorePlayerData();
    void LoadPlayerData();

    void StoreJournalData();
    void LoadJournalData();

    void DiscoverLevel(int levelIndex);
    void UnlockAllLevels();
}