#include "journalData.h"
#include <unordered_set>
#include <optional>

namespace JournalData
{
    namespace
    {
        bool hasNewEntry = false;
        std::unordered_set<CreatureEntryID> discoveredCreatures;
        std::unordered_set<JournalEntryID> discoveredJournalEntries;
        std::optional<CreatureEntryID> lastDiscoveredCreature;

        const std::vector<JournalEntry> journalEntries = {
            {
                JournalEntryID::WashedAshore,
                "Washed Ashore",
                "I've washed ashore on some godforsaken island. "
                "My ship is gone, and something is moving beyond the trees. "
                "Luckily I found this crossbow and sword "
                "I still have my journal. I should keep a record of my encounters on this strange island."
            },
            {
                JournalEntryID::MetHermit,
                "The Hermit",
                "I spoke with an old castaway near the campfire. "
                "He seems half-mad, but he knows these islands better "
                "than anyone."
            },
            {
                JournalEntryID::FoundRuins,
                "Beneath The Island ", 
                "I found a passage beneath the old ruins. "
                "The cells suggest this place was once a prison. "
                "though the tunnels seem far older than the fortress above. Perhaps there is another way out below. "
            },
            {
                JournalEntryID::FoundBlunderbuss,
                "The Blunderbuss",
                "I found an old blunderbuss. "
                "It is slow to reload and nearly useless at a distance, but at close range "
                "its spread shot should tear through almost anything."
            },


            {
                JournalEntryID::DeeperStill,
                "Deeper Still",
                "How deep do these dungeons go? "
                "I thought I had found the exit, but the passage only led farther underground. "
                "Theres nothing to do now but keep descending."
            },


            {
                JournalEntryID::FoundHarpoon,
                "The Harpoon",
                "I found an attachment for the crossbow. "
                "It appears to be a harpoon with a rope. "
                "I could pull enemies toward me, or pull myself toward grapple points! "
            },

            //return to surface
            {
                JournalEntryID::Resurface,
                "Resurface",
                "I found the exit, and made my way to the surface."
                "I came out on the other side of the island. "
                "I should investigate the other islands.  "
            },

           {
                JournalEntryID::DoubleShot,
                "Double Load",
                "I can now double load the blunderbuss. "
                "It's dangerous, and takes some extra time to load. "
                "But it should shred anything in its path.  "
            },
                

            {
                JournalEntryID::River,
                "The Jungle",
                "I've returned to the surface. I'm on a completely different island. "
                "The local wild life seems hostile.  "
                "I need to find the exit before these reptiles tear me apart. "
            },

            {
                JournalEntryID::FoundStaff,
                "The Magic Staff",
                "This old stick is imbued with some kind of elemental magic. "
                "Casting magic consumes mana. "
                "Press T to switch from fire to ice. "
            },

    
        };

        const std::vector<CreatureEntry> creatureEntries = {
            {
                CreatureEntryID::Raptor,
                "Raptor",
                "A fast and highly aggressive predator.Circles its prey before attacking and is rarely found alone.",
                "",
                "raptorTexture",
                512,
                512
            },
            {
                CreatureEntryID::Skeleton,
                "Skeleton",
                "The animated remains of some long-dead sailor. Carries a sword and relentlessly pursues its target",
                "", //looks better as one wrapped line. 
                "skeletonSheet",
                200,
                200
            },
            {
                CreatureEntryID::Pirate,
                "Pirate",
                "A pirate likely searching the dungeons for treasure. fires a musket and attempts to reposition during combat. ",
                "",
                "pirateSheet",
                200, 
                200
            },
            {
                CreatureEntryID::Bat,
                "Bat",
                "A vampire bat. These things infest the dungeons. Flying targets are harder to hit. ",
                "",
                "batSheet",
                200, 
                200
            },

            {
                CreatureEntryID::Zombie,
                "Zombie",
                "The animated corps of some poor soul. Zombies may reanimate after death. Unless they are missing a head.",
                "",
                "zombieSheet",
                200, 
                200
            },
            {
                CreatureEntryID::BloatBat,
                "Bloat Bat",
                "A mutated form of the vampire bat. Explodes on death, or within proximity of player. ",
                "",
                "bloatBatSheet",
                200, 
                200
            },

            {
                CreatureEntryID::Spider,
                "Spider",
                "An abnormally large spider. Spiders have a nasty bite, and may hatch from eggs. ",
                "",
                "spiderSheet",
                200, 
                200
            },
            {
                CreatureEntryID::GiantSpider,
                "Giant Spider",
                "An extremely large spider. Giant spider can lay eggs, and retreats when in trouble.",
                "",
                "GiantSpiderSheet",
                300, 
                300
            },
            {
                CreatureEntryID::Wizard,
                "Wizard",
                "Insane cultists who practice fire magic. Wizards throw fireballs, and attack with a staff when close. ",
                "",
                "wizardSheet",
                400, 
                400
            },

            {
                CreatureEntryID::Dactyl,
                "Pterodactyl",
                "A flying dinosaur. Pterodactyls dive bomb you, then retreat back up to altitude.",
                "",
                "dactylSheet",
                512, 
                512
        
            },
            {
                CreatureEntryID::Trex,
                "Tyrannosaurus Rex",
                "A terrible lizard. Keep your distance from this fearsome predator. ",
                "",
                "trexSheet",
                300, 
                300
            },

            {
                CreatureEntryID::bossMob,
                "Boss Monsters",
                "Some of the creatures on this island appear to be overgrown. ",
                "",
                "skeletonSheet",
                200, 
                200,
                1.35 //bigger scale for boss mob image
            },

            {
                CreatureEntryID::kraken,
                "Kraken",
                "A giant squid, intent on sinking the ship. ",
                "",
                "krakenShot",
                860, 
                860,
            },

            // { //dont show ghost, it looks ugly
            //     CreatureEntryID::Ghost,
            //     "Ghost",
            //     "The spirit of some one long dead. Ghosts attacks siphon health.",
            //     "",
            //     "Moderate",
            //     "Moderate",
            //     "High",
            //     "Use the sword",
            //     "ghostSheet",
            //     200, 
            //     200
        
            // },

        };
    }



    std::vector<const JournalEntry*> GetDiscoveredJournalEntries()
    {
        std::vector<const JournalEntry*> discoveredEntries;

        for (const JournalEntry& entry : journalEntries)
        {
            if (Progress::IsJournalEntryDiscovered(entry.id))
            {
                discoveredEntries.push_back(&entry);
            }
        }

        return discoveredEntries;
    }

    std::vector<const CreatureEntry*> GetDiscoveredCreatureEntries()
    {
        std::vector<const CreatureEntry*> discoveredEntries;

        for (const CreatureEntry& entry : creatureEntries)
        {
            if (Progress::IsCreatureDiscovered(entry.id))
            {
                discoveredEntries.push_back(&entry);
            }
        }

        return discoveredEntries;
    }


    const std::vector<JournalEntry>& GetJournalEntries()
    {
        return journalEntries;
    }



    bool DiscoverCreature(CreatureEntryID id)
    {
        bool newlyDiscovered = discoveredCreatures.insert(id).second;

        if (newlyDiscovered)
        {
            lastDiscoveredCreature = id;
            hasNewEntry = true;
        }

        return newlyDiscovered;
    }

    const std::vector<CreatureEntry>& GetCreatureEntries()
    {
        return creatureEntries;
    }

    const JournalEntry* GetJournalEntry(JournalEntryID id)
    {
        for (const JournalEntry& entry : journalEntries)
        {
            if (entry.id == id)
                return &entry;
        }

        return nullptr;
    }

    const CreatureEntry* GetCreatureEntry(CreatureEntryID id)
    {
        for (const CreatureEntry& entry : creatureEntries)
        {
            if (entry.id == id)
                return &entry;
        }

        return nullptr;
    }


    namespace Progress
    {

        bool HasNewEntry()
        {
            return hasNewEntry;
        }

        void MarkEntriesSeen()
        {
            hasNewEntry = false;
        }


        void UnlockAll()
        {
            for (const JournalEntry& entry : journalEntries)
            {
                discoveredJournalEntries.insert(entry.id);
            }

            for (const CreatureEntry& entry : creatureEntries)
            {
                discoveredCreatures.insert(entry.id);
            }

            if (!creatureEntries.empty())
            {
                lastDiscoveredCreature = creatureEntries.back().id;
            }
        }



        bool DiscoverJournalEntry(JournalEntryID id)
        {
            bool newlyDiscovered = discoveredJournalEntries.insert(id).second;

            if (newlyDiscovered){
                hasNewEntry = true;
            }

            return newlyDiscovered;
        }

        bool IsJournalEntryDiscovered(JournalEntryID id)
        {
            return discoveredJournalEntries.count(id) != 0;
        }

        bool DiscoverCreature(CreatureEntryID id)
        {

            bool newlyDiscovered = discoveredCreatures.insert(id).second;

            if (newlyDiscovered)
            {
                lastDiscoveredCreature = id;
                hasNewEntry = true;
            }

            return newlyDiscovered;
    
        }

        bool IsCreatureDiscovered(CreatureEntryID id)
        {
            return discoveredCreatures.count(id) != 0;
        }

        const CreatureEntry* GetLastDiscoveredCreature()
        {
            if (!lastDiscoveredCreature.has_value())
            {
                return nullptr;
            }

            return GetCreatureEntry(*lastDiscoveredCreature);
        }

    }
}