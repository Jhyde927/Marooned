#pragma once

#include <string>
#include <vector>
#include "raylib.h"

namespace JournalData
{
    enum class JournalEntryID
    {
        WashedAshore     = 0,
        MetHermit        = 1,
        FoundRuins       = 2,
        FoundBlunderbuss = 3,
        DeeperStill      = 4,
        FoundHarpoon     = 5,
        Resurface        = 6, //found raft body
        DoubleShot       = 7,
        RaftMast         = 8,
        River            = 9,
        FoundStaff       = 10,
        FoundIce         = 11,
        RaftSail         = 12, 

        Count            = 13
    };

    enum class CreatureEntryID
    {
        Raptor      = 0,
        Skeleton    = 1,
        Pirate      = 2,
        Bat         = 3,
        Zombie      = 4,
        BloatBat    = 5,
        Spider      = 6,
        GiantSpider = 7,
        Wizard      = 8,
        Dactyl      = 9,
        Trex        = 10,
        bossMob     = 11,
        kraken      = 12, 

        Count       = 13
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

        // ResourceManager texture name or path.
        std::string textureName;

        int frameWidth;
        int frameHeight;

        float imageScale = 1.0f;

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