#include "level.h"
#include "dungeonGeneration.h"
#include <vector>

std::vector<PropSpawn> overworldProps;

DungeonEntrance entranceToDungeon1 = {
    {0, 200, 0}, // position
    2, // linkedLevelIndex
    false, //islocked
};


//Player Position: X=-5653.32 Y=289.804 Z=6073.24
DungeonEntrance entranceToDungeon3 = {
    {-5653, 150, 6073}, 
    4,
    true
};

//Player Position: X=6294.27 Y=299.216 Z=5515.47
DungeonEntrance entranceToDungeon4 = {
    {6294, 150, 5515},
    6,
    true
};

DungeonEntrance entranceToDungeon11 = {
    {6294, 150, 5515},
    0, //go back to start
    false 

};


std::vector<LevelData> levels = {
    {

        "Dungeon1", 
        "", "", {0,0,0}, 0.0f, {0,0,0}, 0,
        false, {}, 0, 0, {}, true
    },
    {
        "PlaceHolder:dungeon1", 
        "", "", {0,0,0}, 0.0f, {0,0,0}, 0,
        false, {}, 1, 0, {}, true
    },

    {
        "Dungeon1",
        "assets/heightmaps/blank.png", //big blank heightmap incase we want water underneath the dungeon. 
        "assets/maps/map4.png",
        {0.0f, 300.0f, 0.0f}, //overwritten by green pixel 
        -90.0f, //starting look direction
        {0.0f, 0.0f, 0.0f}, //raptor spawn center
        0, //raptor count
        true, //isDungeon is true
        {}, //dungeons don't have level entrances
        2, //current level index
        3, //next level index 
        {}, //outdoor props
        true,

    },
    {
        "Dungeon2",
        "assets/heightmaps/blank.png",
        "assets/maps/map6.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        3, 
        4,
        {},
        true,
       

    },

    {
        "Dungeon3",
        "assets/heightmaps/blank.png",
        "assets/maps/map7.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        4, 
        5,
        {},
        true,
    },
    {
        "Dungeon4",
        "assets/heightmaps/blank.png",
        "assets/maps/map8.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        5, //skip, was the first dungeon from entrance 2
        6,
        {},
        true,
    },
        {
        "Dungeon5",
        "assets/heightmaps/blank.png",
        "assets/maps/map9.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        6, //start here from entrance 3. 
        8, //skip dungeon 6/map10/index7
        {}, 
        true, 
    },
        {
        "Dungeon6",
        "assets/heightmaps/blank.png",
        "assets/maps/map10.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
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
        "Dungeon7",
        "assets/heightmaps/blank.png",
        "assets/maps/map11.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        8, 
        9,
        {}, 
        true,
    },
        {
        "Dungeon8", //fireball level
        "assets/heightmaps/blank.png",
        "assets/maps/map12.png",
        {0.0f, 300.0f, 0.0f},
        180.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        9, 
        10,
        {},
        false, 
    },
        {
        "Dungeon9",
        "assets/heightmaps/blank.png",
        "assets/maps/map14.png",
        {0.0f, 300.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        10, 
        11,
        {}, 
        true, 
    },
        {
        "Dungeon10", //lava level
        "assets/heightmaps/blank.png",
        "assets/maps/map15.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        11, 
        12, //go from portal to river, skip last 2 for demo
        {}, 
        false, 
    },
        {
        "Dungeon11",
        "assets/heightmaps/blank.png",
        "assets/maps/map16.png",
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
        "Dungeon12", //skipped
        "assets/heightmaps/blank.png",
        "assets/maps/map17.png",
        {0.0f, 300.0f, 0.0f},
        -90.0f,
        {0.0f, 0.0f, 0.0f},
        0, 
        true, //isDungeon is true
        {},
        13, 
        2, //change for demo to start back at dungeon1 index 2
        {}, 
        true,// ceiling
    },
    {

        "(Middle Island)", //display name
        "assets/heightmaps/MiddleIsland.png", //heightmap
        "",//dungeon path
        {0.0f, 300.0f, 0.0f}, //starting position
        -90.0f, //starting player rotation
        {0, 0, 0}, //raptor spawn center
        5, //raptor count
        false, //isDungeon
        {entranceToDungeon1, entranceToDungeon3, entranceToDungeon4}, //add entrance struct to level's vector of entrances. 
        14, //current level
        2, //next level, index 2
        {
            { PropType::FirePit,  5200.f, -5600.f,  0.f, 100.0f }, // outdoor props
            { PropType::Boat,     6000.0f, -20.0f, 0.0f},
        
        },
        true, //ceiling default is true. doesn't matter for islands. 
        
    },
    {
        "River", 
        "assets/heightmaps/River.png",
        "",
        {5475.0f, 300.0f, -5665.0f},
        180.0f,
        {0.0f, 0, 0.0f},
        7,//raptor count
        false,
        {entranceToDungeon11},
        15, 
        0,
        {{ PropType::Boat,-1343.15, 103.922, -1524.03}},
        true, //ceiling
   
    },
    
};

