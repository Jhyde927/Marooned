#include "saveGame.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include "player.h"
#include "world.h"
#include "JournalData.h"

SaveData save;

namespace SaveGame
{
    constexpr const char* SAVE_FILE = "save.txt";

    bool StringToBool(const std::string& value)
    {
        return value == "1" || value == "true";
    }

    std::vector<int> ParseIntList(const std::string& value)
    {
        std::vector<int> result;
        std::stringstream stream(value);
        std::string item;

        while (std::getline(stream, item, ','))
        {
            try
            {
                result.push_back(std::stoi(item));
            }
            catch (...)
            {
                // Ignore invalid entries.
            }
        }

        return result;
    }

    void WriteIntList(
        std::ofstream& file,
        const char* key,
        const std::vector<int>& values)
    {
        file << key << '=';

        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
            {
                file << ',';
            }

            file << values[i];
        }

        file << '\n';
    }

    void LoadJournalData()
    {
        for (int value : save.discoveredJournal)
        {
            JournalData::Progress::DiscoverJournalEntry(
                static_cast<JournalData::JournalEntryID>(value));
        }

        for (int value : save.discoveredCreatures)
        {
            JournalData::Progress::DiscoverCreature(
                static_cast<JournalData::CreatureEntryID>(value));
        }
    }

    void StoreJournalData()
    {
        save.discoveredJournal.clear();
        save.discoveredCreatures.clear();

        for (const JournalData::JournalEntry* entry :
            JournalData::GetDiscoveredJournalEntries())
        {
            save.discoveredJournal.push_back(
                static_cast<int>(entry->id));
        }

        for (const JournalData::CreatureEntry* entry :
            JournalData::GetDiscoveredCreatureEntries())
        {
            save.discoveredCreatures.push_back(
                static_cast<int>(entry->id));
        }
    }

    void StorePlayerData()
    {
        save.healthPotions = player.inventory.GetItemCount("HealthPotion");
        save.manaPotions = player.inventory.GetItemCount("ManaPotion");
        save.gold = player.gold;

        save.swordUnlocked = true;
        save.crossbowUnlocked = hasCrossbow;
        save.blunderbussUnlocked = hasBlunderbuss;
        save.magicStaffUnlocked = hasStaff;
        save.harpoonUnlocked = hasHarpoon;
        save.doubleShotUnlocked = hasDoubleShot;
        save.currentPowerUp = static_cast<int>(player.currentPowerUp);
        save.entrancesUnlocked = unlockEntrances;
    }

    void LoadPlayerData(){

        player.inventory.AddItemAmount("HealthPotion", save.healthPotions);
        player.inventory.AddItemAmount("ManaPotion", save.manaPotions);
        player.gold = save.gold;

        hasCrossbow = save.crossbowUnlocked;
        hasBlunderbuss = save.blunderbussUnlocked;
        hasStaff = save.magicStaffUnlocked;
        hasDoubleShot = save.doubleShotUnlocked;
        hasHarpoon = save.harpoonUnlocked;

        if (save.currentPowerUp >= static_cast<int>(PowerUpType::None) &&
            save.currentPowerUp <= static_cast<int>(PowerUpType::DoubleShot))
        {
            player.currentPowerUp =
                static_cast<PowerUpType>(save.currentPowerUp);
        }
        else
        {
            player.currentPowerUp = PowerUpType::None;
        }

        unlockEntrances = save.entrancesUnlocked;
    }

    void DiscoverLevel(int levelIndex)
    {
        if (std::find(
                save.discoveredLevels.begin(),
                save.discoveredLevels.end(),
                levelIndex) == save.discoveredLevels.end())
        {
            save.discoveredLevels.push_back(levelIndex);
        }
    }

    void UnlockAllLevels()
    {
        save.discoveredLevels.clear();
        save.discoveredLevels.reserve(levels.size());

        for (int i = 0; i < static_cast<int>(levels.size()); ++i)
        {
            save.discoveredLevels.push_back(i);
        }

        Save(save);

    }


}

SaveData SaveGame::Load()
{
    SaveData data; // Starts with the defaults from saveGame.h

    std::ifstream file(SAVE_FILE);

    if (!file.is_open())
    {
        return data;
    }

    std::string line;

    while (std::getline(file, line))
    {
        std::size_t separator = line.find('=');

        if (separator == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator + 1);

        try
        {
            if (key == "version")
                data.version = std::stoi(value);
            else if (key == "levelIndex")
                data.levelIndex = std::stoi(value);
            else if (key == "healthPotions")
                data.healthPotions = std::stoi(value);
            else if (key == "manaPotions")
                data.manaPotions = std::stoi(value);
            else if (key == "gold")
                data.gold = std::stoi(value);
            else if (key == "swordUnlocked")
                data.swordUnlocked = StringToBool(value);
            else if (key == "crossbowUnlocked")
                data.crossbowUnlocked = StringToBool(value);
            else if (key == "blunderbussUnlocked")
                data.blunderbussUnlocked = StringToBool(value);
            else if (key == "magicStaffUnlocked")
                data.magicStaffUnlocked = StringToBool(value);
            else if (key == "harpoonUnlocked")
                data.harpoonUnlocked = StringToBool(value);
            else if (key == "doubleShotUnlocked")
                data.doubleShotUnlocked = StringToBool(value);
            else if (key == "entrancesUnlocked")
                data.entrancesUnlocked = StringToBool(value);
            else if (key == "currentPowerUp")
                data.currentPowerUp = std::stoi(value);
            else if (key == "discoveredJournal")
                data.discoveredJournal = ParseIntList(value); 
            else if (key == "discoveredCreatures")
                data.discoveredCreatures = ParseIntList(value);
            else if (key == "discoveredLevels")
                data.discoveredLevels = ParseIntList(value);

            

        }
        catch (...)
        {
            // Keep the default value if this particular line is damaged.
        }
    }

    // Prevent damaged or edited saves from loading negative quantities.
    data.levelIndex = std::max(0, data.levelIndex);
    data.healthPotions = std::max(0, data.healthPotions);
    data.manaPotions = std::max(0, data.manaPotions);
    data.gold = std::max(0, data.gold);

    return data;
}

void SaveGame::Save(const SaveData& data)
{
    std::ofstream file(SAVE_FILE);

    if (!file.is_open())
    {
        return;
    }

    file << "version=" << data.version << '\n';
    file << "levelIndex=" << data.levelIndex << '\n';

    file << "healthPotions=" << data.healthPotions << '\n';
    file << "manaPotions=" << data.manaPotions << '\n';
    file << "gold=" << data.gold << '\n';

    file << "swordUnlocked=" << data.swordUnlocked << '\n';
    file << "crossbowUnlocked=" << data.crossbowUnlocked << '\n';
    file << "blunderbussUnlocked=" << data.blunderbussUnlocked << '\n';
    file << "magicStaffUnlocked=" << data.magicStaffUnlocked << '\n';
    file << "harpoonUnlocked=" << data.harpoonUnlocked << '\n';
    file << "doubleShotUnlocked=" << data.doubleShotUnlocked << '\n';
    file << "currentPowerUp=" << data.currentPowerUp << '\n';

    file << "entrancesUnlocked=" << data.entrancesUnlocked << '\n';

    WriteIntList(file, "discoveredJournal", data.discoveredJournal);

    WriteIntList(file, "discoveredCreatures", data.discoveredCreatures);

    WriteIntList(file, "discoveredLevels", data.discoveredLevels);
}