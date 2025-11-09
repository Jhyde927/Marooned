# Marooned  

**Marooned** is a first-person adventure/shooter set in the 1700s pirate era on a chain of Caribbean islands. The game combines open-world exploration with dungeon crawling. Fully 3d environments with 2D animated billboards. 
https://jhyde.itch.io/marooned

![Gameplay Screenshot](assets/screenshots/TitleScreen.png)
![Gameplay demo](assets/screenshots/demo.gif)

## Features  

- 🌴 **Overworld Exploration**  
  - Islands generated from a **grayscale heightmap**  
  - Procedural vegetation (trees, bushes)  
  - Dynamic water and post-processing shaders  

- 🏰 **Dungeon Crawling**  
  - Dungeons generated from **PNG images**  
    - White pixels → floor tiles  
    - Black pixels → walls  
    - Special colored pixels mark **entrances/exits/doors**  
  - Tile-based wall/floor models with baked lighting  
  - 2D billboard enemies (pirates, raptors, ghosts, etc.)  

- ⚔️ **Combat System**  
  - Melee weapons 
  - Blunderbuss
  - Enemies with different AI behaviors (chase, flee, wander)  

- 🎮 **Player Controller**  
  - Switchable **free camera** / **first-person player mode**  
  - Swimming and boat-riding support  

## Tech Notes  

- Built with **C++17** and **Raylib 5.5**  
- Shader-based effects (bloom, AO, foliage alpha)  
- Development in **VS Code** with Makefile builds  

