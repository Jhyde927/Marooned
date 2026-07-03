#include "level.h"
#include "dungeonGeneration.h"
#include <vector>
#include "resourceManager.h"
#include <iostream>
#include <fstream>

std::vector<PropSpawn> overworldProps;



int LoadLastLevel()
{
    std::ifstream file("save.txt");

    int level = 0;
    if (file >> level)
    {
        return level;
    }

    return 0; // default level if no save exists
}

void SaveLastLevel(int levelIndex)
{
    std::ofstream file("save.txt");
    file << levelIndex;
}



PreviewInfo MakePreviewInfoFromLevel(const LevelData& level)
{
    PreviewInfo p;
    p.levelIndex  = level.levelIndex;
    p.displayName = level.name;

    // Decide what kind of preview this level gets
    if (level.isDungeon)
    {
        if (!level.dungeonPath.empty())
        {
            p.kind        = PreviewKind::DungeonMap;
            p.previewPath = level.dungeonPath;
        }
    }
    else
    {
        if (!level.heightmapPath.empty())
        {
            p.kind        = PreviewKind::OverworldHeightmap;
            p.previewPath = level.heightmapPath;
        }
    }

    // If neither condition matched, kind stays None
    if (p.kind != PreviewKind::None)
    {
        // Stable, predictable resource key
        // (avoid using raw paths as keys)
        p.textureKey = "preview_level_" + std::to_string(level.levelIndex);
    }

    return p;
}

std::vector<PreviewInfo> BuildLevelPreviews(bool preloadTextures)
{
    std::vector<PreviewInfo> previews;
    previews.reserve(levels.size());

    for (const LevelData& level : levels)
    {
        PreviewInfo preview = MakePreviewInfoFromLevel(level);

        if (preview.IsValid())
        {
            if (preloadTextures)
            {
                // Register/load texture into ResourceManager
                // Safe even if called multiple times, assuming RM guards duplicates
                R.LoadTexture(preview.textureKey, preview.previewPath);

                // Optional: touch it once so fallback errors show immediately
                Texture2D& tex = R.GetTexture(preview.textureKey);
                (void)tex;
            }
        }

        previews.push_back(preview);
    }

    return previews;
}



DungeonEntrance entranceToDungeon1 = {
    {0, 180, 0}, // position
    1, // linkedLevelIndex 
    false, //islocked
    45.0f * DEG2RAD,
};


//Player Position: X=-5653.32 Y=289.804 Z=6073.24
DungeonEntrance entranceToDungeon3 = {
    {-5484.0, 180, -5910.67},//{-5653, 150, 6073}
    4,
    true,
};

//Player Position: X=6294.27 Y=299.216 Z=5515.47
DungeonEntrance entranceToDungeon4 = {
    {6294, 150, 5515},
    5,
    true
};

DungeonEntrance entranceToDungeon11 = {
    //(-6146.0f, 319.0f, 5360.0f)
    {-6146.0f, 150.0f, 5360.0f},
    9, 
    false 

};

DungeonEntrance entranceToDungeon24 = {

    
    {-6226, 180, 462},
    24, 
    false 

};

std::vector<LevelData> levels = {
    {
        //
        "MiddleIsland", //display name
        "assets/heightmaps/MiddleIsland.png", //heightmap
        "",//dungeon path
        {-5404.0f, 318.0f, -6064.0f}, //Entrance 2 position
        -90.0f, //starting player rotation
        {0, 0, 0}, //raptor spawn center
        5, //raptor count
        false, //isDungeon
        {entranceToDungeon1, entranceToDungeon3, entranceToDungeon4}, //add entrance struct to level's vector of entrances. 
        0, //current level
        1, //next level, index 1 for new levels
        {
            { PropType::FirePit,  Vector3{5200.f, 0.0f, -5600.f},  0.f, 100.0f }, // outdoor props
            { PropType::Boat,     Vector3{-4368.62, -20, -4036.75}}, //Vector3{-3767.0f,-20, 5199.0f}}
            
        },
        true, //ceiling default is true. doesn't matter for islands. 
        
    },

    {
        "Dungeon1",
        "assets/heightmaps/blank.png", //big blank heightmap incase we want water underneath the dungeon. 
        "assets/maps/dungeon1.png",
        {0.0f, 300.0f, 0.0f}, //overwritten by green pixel 
        -90.0f, //starting look direction
        {0.0f, 0.0f, 0.0f}, //raptor spawn center
        0, //raptor count
        true, //isDungeon is true
        {}, //dungeons don't have level entrances
        1, //current level index
        2, //next level index 
        {}, //outdoor props
        true,

    },

    {
        "Dungeon2",
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon2.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        2, 
        3,
        {},
        true,
       

    },

    {
        "Dungeon3",
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon3.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        3, 
        0, 
        {},
        true,
    },

        {
        "Dungeon4", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon4.png",
        {0.0f, 300.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        4, 
        5, //skip dungeon 6/map10/index7
        {}, 
        true, 
    },

        {
        "Dungeon5",
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon5.png",
        {0.0f, 300.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        5, 
        6,
        {}, 
        true, 
    },

        {
        "Dungeon6",
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon6.png",
        {0.0f, 300.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        6, 
        7,
        {}, 
        true,
    },
        {
        "Dungeon7", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon7.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        7, 
        8,
        {},
        true, 
    },

    {
        "River", 
        "assets/heightmaps/River.png",
        "",
        {5475.0f, 300.0f, -5665.0f},
        0.0f,
        {0.0f, 0, 0.0f},
        15,//raptor count
        false,
        {entranceToDungeon11}, //entrance needs to exit to 9
        8, 
        9,
        {{ PropType::Boat,-1343.15, 103.922, -1524.03}},
        false, //ceiling
   
    },

        {
        "Dungeon8",  
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon8.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        9, 
        10, 
        {}, 
        true,// ceiling
    },

        {
        "Dungeon9", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon9.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        10, 
        11, 
        {}, 
        true,// ceiling
    },

        {
        "Dungeon10", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon10.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        11, 
        12, 
        {}, 
        false,// ceiling
    },

        {
        "Dungeon11", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon11.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        12, 
        13, 
        {}, 
        false,// ceiling
    },
        {
        "Dungeon12", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon12.png",
        {0.0f, 300.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        13, 
        14, 
        {}, 
        false,// ceiling
    },

        {
        "Dungeon14", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon14.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        14, 
        15, 
        {}, 
        true,// ceiling
    },
        {
        "Dungeon15", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon15.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        15, 
        16, 
        {}, 
        false,// ceiling
    },
        {
        "Dungeon16", 
        "assets/heightmaps/blank.png",
        "assets/maps/dungeon16.png",
        {5475.0f, 300.0f, -5665.0f}, //original player start position
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        16, 
        17, //exit to middle island
        {}, 
        false,// ceiling
    },

        {
        "Ship", 
        "assets/heightmaps/blank.png",
        "assets/maps/bigShip.png",
        {5475.0f, 300.0f, -5665.0f}, //original player start position
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //ship is technically a dungeon
        {},
        17, 
        0,
        {}, 
        false,// ceiling
    },

    //     {
    //     "Dungeon17", 
    //     "assets/heightmaps/blank.png",
    //     "assets/maps/dungeon17.png",
    //     {5475.0f, 300.0f, -5665.0f}, //original player start position
    //     0.0f,
    //     {0.0f, 0.0f, 0.0f},
    //     0, 
    //     true, //isDungeon is true
    //     {},
    //     18, 
    //     0, //exit to middle island
    //     {}, 
    //     false,// ceiling
    // },

};


    //     {
    //     "Swamp", 
    //     "assets/heightmaps/Noise4.png",
    //     "",
    //     {5475.0f, 300.0f, -5665.0f},
    //     0.0f,
    //     {0.0f, 0, 0.0f},
    //     5,//raptor count
    //     false,
    //     {entranceToDungeon24},
    //     19, 
    //     0,
    //     {{ PropType::Boat,-1343.15, 103.922, -1524.03}},
    //     false, //ceiling
   
    // },




