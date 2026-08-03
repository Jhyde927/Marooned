#pragma once

#include <string>
#include <vector>
#include "raylib.h"

namespace JournalData
{
    enum class JournalEntryID
    {
        WashedAshore,
        MetHermit,
        FoundRuins,
        FoundBlunderbuss,
        DeeperStill,
        FoundHarpoon,
        Resurface,
        DoubleShot,
        FoundStaff,

        Count
    };

    enum class CreatureEntryID
    {
        Raptor,
        Skeleton,
        Pirate,
        Bat,
        Zombie,
        Spider,
        GiantSpider,
        Wizard,
        Trex,
        Dactyl,
        Ghost,




        Count
    };

    struct JournalEntry
    {
        JournalEntryID id;
        std::string title;
        std::string body;
    };

    struct CreatureEntry
    {
        CreatureEntryID id;

        std::string name;
        std::string description;
        std::string behavior;
        std::string durability;
        std::string speed;
        std::string threat;
        std::string weakness;

        // ResourceManager texture name or path.
        std::string textureName;

        int frameWidth;
        int frameHeight;

    };


    std::vector<const CreatureEntry*> GetDiscoveredCreatureEntries();
    std::vector<const JournalEntry*> GetDiscoveredJournalEntries();

    const std::vector<JournalEntry>& GetJournalEntries();
    const std::vector<CreatureEntry>& GetCreatureEntries();

    const JournalEntry* GetJournalEntry(JournalEntryID id);
    const CreatureEntry* GetCreatureEntry(CreatureEntryID id);

    

    namespace Progress
    {
        bool HasNewEntry();
        void MarkEntriesSeen();

        void UnlockAll();
        bool DiscoverJournalEntry(JournalEntryID id);
        bool IsJournalEntryDiscovered(JournalEntryID id);

        bool DiscoverCreature(CreatureEntryID id);
        bool IsCreatureDiscovered(CreatureEntryID id);
        const CreatureEntry* GetLastDiscoveredCreature();
  
    }

    
}