Marooned on an island full of dinosaurs 06/18/2025

Set in the 1700s in the Caribbean. Some one marooned you on purpose. You lost your memory? 
Trapped on an island lost to time. Find a treasure chest with a gun in it. Add a sword at some point. 
Need a way of making structures like lost temple ruins. caves. Huts. villagers? hostile locals? Go full Turok? 
Need to keep the scope small enough to finish. It's already hugely complicated. 

Level switching. Change heightmaps from a menu, destroy all objects, regenerate vegetation, and raptors. A menu screen that shows before the level loads? For development it makes sense to be able to swap heightmaps before and during runtime. 

Terrain has a hard limit to how big it can be. 4k textures scaled to 16k. sounds like a lot but everything else is scaled big. Need serpenteen paths that loop around. Need a way to block player besides water. invisible walls where thick jungle is. Create bounding boxes around trees. make areas where trees are too dense to pass. 

invisible wall at the edge of water plane. make a boundary at 30k radius from center for all maps?  

create bounding boxes around trees that collide with the player. and raptors? x

Bridges.

Treasure chest model. Use the mimic from fools path. It's fully rigged and animated. it would be good to know how to do that with raylib. 

Day/Night cycle?

foot steps x

Player Take Damage x

raptors should make more noise. when exiting idle maybe.  x

positional sound - volume attenuation only. raylib can't do 3d audio. or it can but I would have to rewrite sound manager. which I should do anyway. 

raptor agro vision should be higher, sounds heard from further away as well. 

make a handleHeightmap.h or something where we can switch between heightmaps. 

made level select menu.

made dungeon generator. need a way to clear the dungeon, when switching levels. x

dungeons are generated from a 32x32 png image. Where black pixels are walls and white pixels are floor tiles. Red is skeletons, blue is barrels, green is starting position, yellow is light source. 

Need a way to make doors work. Need some kind of barrier that can be removed. making the door arch model fit and be rotated right sounds like a nightmare. 

-Doorways are purple pixels. DoorWays spawn doors which are centered on the doorway arch. Doors can also lead to other levels. Teal pixels are exits that lead to the previous level. Orange pixels are next level doors that lead to the next level. Terrain generated levels have dungeonEnterance struct that spawns a archway with a door that leads to dungeon1. We keep a vector of entrances in level data so we can have multiple entrances to different dungeons from the same over world. 


runaway time for raptors. x 

added skeletons + pathfinding. Simple BFS with smoothPath function. Skeletons have vision by marching through the pixel map. it's not very reliable, but raylib doesn't have built in 3D raycasts. I would need to implement it myself. maybe later.
-Improved ray marching to be mostly reliable. Skeletons now have a alertNearbySkeleton() that looks for other skele's within vision and sets them to chase when they gain visibility of the player, or if they take damage. 

separated raptor AI and skeleton AI into there own functions. So they each have there own switch. Raptors are now only for outdoor use. and skeletons are for dungeons only. I tried raptors in dungeon but their run away behavior messed everything up.
So raptors follow the heightmap and skeletons follow the dungeon Image. 

Add a chest. 

game pad inputs for shoot/swing/block/jump/run/swap weapon

add simple inventory. 4 boxes in the bottom left. It's a map of strings and amounts. so you can have multiple potions in 1 inventory slot. We should make another consumable to justify this type of inventory. like food? maybe you can have multiple keys. and there's just generic keys that opens locked doors and are used up. lock picks? lock picking mini game? 

limit number of health potions you can have? nah. 

Mana potions? Maybe player can throw fireballs. 

stealth? We have working vision cones. sort of. We could make them cones. then draw the cones to show the player the enemies sight. Lean into the grid based nature of the game maybe. If we could highlight floor tiles with a shader we could highlight vision that way. I just want the ability to turn a specific floor tile red. But since it's .glb with baked in texture I can't easily do that. and I never figured out how to apply a texture to some mesh. Everything is just solid colors with shaders or baked in texture of the dungeon. Maybe just make a cube and try to apply a texture to it. a box, a breakable box?

We have multiple models of the barrel. One of which is the barrel smashed. We could give the barrels health and when you hit them with a sword we swap in the broken model. barrels should be like 30 percent bigger. they don't line up with the walls. I want barrels in the corners of the dungeon but since the wall is in the middle of the tile there a gap. could push walls in somehow. 

Doors have a member variable isLocked...implemented locked doors. Locked doors are Aqua colored pixels. The doorway gets set to locked, then the door sets isLocked to dw.isLocked. 

Implemented keys and updated inventory. Keys are consumables. They open locked doors. You can carry keys between levels. We can do some puzzle type stuff with this later. Keys are billboard just like health pot. need to increase the size, just for keys. 
if (key.type == CollectableType::Key) scale = 100

teleporters, fast travel, Ride a big boat to a new over world map. 

Maybe you need to find things to repair your damaged boat. Wood planks, cloth for a sail, rudder, supplies
There is a model of a ship wreck right off the coast of the starting island. swim out to it and collect stuff to build a raft. The goal is to get off the island. We could show the outline of a raft model that slowly gets filled in, as you find more things. 

dungeon 1-1 would be middle islands first dungeon, middle island could have 2 more dungeons. dungeon 1-2, and 1-3, after 1-3 you have collected all the supplies for the raft and sail to the next overworld where you enter dungeon 2-1. islands could have different flora and fauna. 

Take a stab at UI art. Pirate themed, wood with gold trim. Steam punk health bar? Health and stam should be on the bottom, as to not blend in with sky. Could make a console type interface. 

pirates should stay tinted red on death

fill out the rest of map 7.  Big room with just 3 pirates. L shaped room around starting corner. A door on either end. both lead to a locked door. There is a maze between the 2 doors where you find the key. So you might end up on the other side of the map or vise versa depending on where you start. Make the path of least resistance the longest. 

Dungeon 3 should exit back up to surface. on the little island in the back. make another entrance you can get to dungeon 3 by boat or swimming to the other island. 

make dungeon 4 and 5 and put an entrance on one of the side islands. left 4 5 6, right 7 8 9. center 1 2 3 then switch to a different island chain. second island chain should be fat perlin noise. 

would the left island dungeons exit out on the right side? Maybe we could make small island exit and right exit locked from the outside. 

Gold. buys magic scrolls. Lightning, fireball, ice, barrels can contain gold, like diablo. 

what to do about webs? We need to draw webs as a flat quad, they can't be a billboard. but everything else is a billboard and needs to be sorted to not occlude each other. we can't put the flat quad into a vector of billboards. could we make a billboard that always faces a certain direction. 

Made a struct called drawRequest. and a vector of drawRequests. drawRequests contain all the data needed to draw an animated billboard, or flatquad, or decal, everything 2d in the game. Then we sort the vector of drawRequests based on distance to camera, and draw in that order. We have functions called GatherEnemies, GatherDecals ect. that copy the data from the character class to the drawReqest. In the final draw function we have a switch case that draws either flat quads or billboards depending on drawRequestType, an enum class in transparentDraw.h this and transparentDraw.cpp contains all the code to make this happen. we just call gatherAllbillboards, then draw the requests. -this solves all the occlusion problems. 
-doors are not included in this. but it doesn't seem to matter. they don't occlude anything. Should probably add draw flat door to the render pipeline anyway. 

I never explained tint based lighting. Since the dungeons are constructed out of floor tiles and wall segments, we can light each one individually by changing the tint property of the model. We can iterate all the walls and measure the distance to a light source or sources, and change the tint brighter or darker the further away. This makes a really low res lighting system. We can even do dynamic lights by just attaching a light source to a moving object like bullet, and then check for it each frame.


Added line of sight to tint based lighting. each wall segment checks distance to light source AND does a line of sight check. That way light doesn't bleed through walls. Could add double walls to stop light bleeding to other side of lit wall. Player has simpler light source without raycasts. It looks better that way. -Started using worldLOS check instead of dungeon map. 3d raycasts better spread the light if I give it an epsilon. where the ray goes further than it should so it actualy hits the walls. Needed to do this for floor ceiling doors and walls. The ray was stopping at the colliders i guess. not making it to wall.position. 

Changed pirates vision to be just a worldLOS check, instead of both dungeon map and world ray cast. They can now see through open doors better and notice the player when he's around a corner but still in eye sight. Probably can do this for skeleton as well. 

Make a fireball type in bullet. 

bullets get occluded by billboards x fixed. bullets are 3d, draw them before transparent 2d stuff. 

started on a revolver, but Now i'm not too sure I want one. finding a good model was impossible, and it wouldn't fit the pirate era theme. Then what else? Another slow firing black powder weapon is too similar to the blunderbuss. A magic staff? that could shoot magic projectiles. maybe you have a book in your left hand and a staff in your right. You do some dark magic rituals to cast spells. 

Added magic staff, and a bullet parameter fireball. fireballs are just big yellow spheres with a light source attached. Need to apply a shader with a fire texture. maybe work on 3d particle system. Maybe fireball bullets have gravity and fly in an arc until they hit the ground and explode. like a grenade launcher! grenade.h? no it would be a bullet. 
-added arcing to fireballs. still need to apply a shader with a moving texture. 
-firballs need to explode when they hit the ground. Explosions would require particles no? we could do a 3d model of an explosion quake 2 style. both particles and model would be ideal. 

3d particle system. It was easy enough in 2d. Try it in 3d for blood and smoke. Your limited with just raylib. particles is one thing that you can do. so you should do it. 

Added fireballs and particles. Fireball is a model I made in blender with a fire texture attached. it slowly rotates when fired. when it hits the ground or an enemy it spawns a animated explosion decal. While the fireball is in the air it emits dark grey smoke particles and when it explodes it emits orange spark particles. I also add blood particles that get emitted from bullets when they hit a non skeleton enemy. -possibly do half orange half yellow particles on explosion, might look like confetti. 

Add blood particles to sword hits. Would need to add emitter to enemy. spawn blood on take damage. Removes need for spawn blood in bullet. -Moved blood emitter to character. Emit blood on take damage if your not a skeleton. 

make staff not available until you pick it up? Where do you pick it up? Level 4? make it a color on the dungeon map and spawn the model in world at that position. make it a collectable? It's lame if you have it from the beginning. for now just make a hasStaff member variable. then you can only switch to it if you hasStaff. 

-Made an std::vector of weaponType, we can than % vector.size() to scroll through weapons. No matter how many weapons you may have unlocked. We simply add the weaponType to the vector when you pick up the weapon. 

         player.currentWeaponIndex = (player.currentWeaponIndex + 1) % player.collectedWeapons.size();
        activeWeapon = player.collectedWeapons[player.currentWeaponIndex];

sword is bloody when starting in dungeon for mysterious reasons. 

Use dinoBite for spider attack sound. current one is weird sounding. 

make another release with the staff sitting on little island. Maybe we could take a picture of the staff and show a billboard of the staff instead of instancing the 3d model. that seems like more work than just draw model. We would need to worry about sorting if it were a billboard. The model should be scaled correctly to look good at a distance. It would have to be a struct though. we can't just have some one off model floating around, it needs  to be a collectable...it can't be a collectable. It needs to be a one off thing. What if you needed to pick up the blunderbuss or sword. We should have in world pickupable weapons. Start with nothing. you already have the vector of weapon types. What would you call this class? CollectableWeapon.
-made CollectableWeapon class. Uses existing WeaponType enum. instance and add to worldWeapons vector. update and draw the 3d models hovering and spinning. For now start with blunderbuss and sword. find the staff on level 4. 

Staff isn't too overpowered. Should maybe slow the fire rate. Self damage seems good, you can accidently hurt yourself with fireballs if too close, but it's not that much damage, 50 max that falls off with distance. Should the staff have different projectiles? How could we do that? FireFireball(), FireIceball(), FireLightning(). Freezing enemies and them hitting them with the sword shaders them. erase the sprite and spawn ice particles, lots of them with high gravity. Ice ball would emit blue particles when in the air instead of smoke. Ice explosion on hit. Could reuse existing explosion just add an ice flag and change the color. 

Implement mana. Player member var. Mana bar. maxMana, currentMana. animate the mana going down. Mana potions are spawn from barrels, and can be spawned in world with a dark blue pixel maybe. Shooting a fireball only costs like 10 mana. shoot 10 times before you need more mana. May need to make fireballs more powerful if they are going to be limited. 
-implemented mana. 10 mana to cast fireball maybe to low. Barrels can drop mana potions 10 percent of the time. 


Make the light range super far but the intensity low. it blends things out wayyyy better. Well way better for fireball anyway. 
I got it a little bit smoother. 

the gun should bob up and down while you walk. the sword should swing up and down. the staff should do something as well when moving. -added weapon bob to all weapons. bobbing every 2 steps looks most like a how a video game should be. x

make staff shoot slower. - slowed from 0.5 to 1.0

chest open animation seems way too slow. x

bullets now kill skeles in one hit. they seems to be penetrating them. how is that possible? we are early returning? -fixed but I dont remember what was wrong. 

Improved bullet light code.

Character has rotationY var that gets set when moving. We never use this. Why would we need rotationY on character? bullet gets direction from player.position - position. billboards always face camera. Could we use it for something in the future? 

make skeletons attach slightly faster so they actually hit each swing not every other. -feels much better should have fixed this ages ago. They attack slightly faster but way more consistent. Attack anim was 4 frames, 0.2 frame time = 0.8 attack cool down. 


added magic icon for fire and ice. T switches between magic spells when holding the magic staff. 

Refactored Resources.h/cpp to ResourceManager.h/cpp. We no longer have a global variable for every resource. We just look up the resource in a map, by string key when we need it. This allows us to unload all resources automatically, and makes adding a new resource one line of code instead of like 3 in 3 different places. It replaces about 100 global vars. 

Refactored main.cpp. Moved everything out of main.cpp into world. Things in world.cpp control the overWorld map, and some global functions that need to get called. DungeonGeneration.h handles all things dungeon. 

Consider moving your render pipeline out of main and into it's own file. x

Fireball traps. Iceball traps

make a controls menu page. 

Moved all the rendering code to render_pipline.cpp. It's getting harder to remember where I put things. 

Made camera_system.h/cpp. changed camera to a singelton camera = CameraSystem::Get().Active(), we could have a cinematic camera in addition to player/freecam. Imagine cut scenes where we animate the camera flying over the terrain and focusing on an objective or something. 

NPC: friendly Character. An old hermit again? A friendly skeleton. Friendly pirate. Talking animal, like a parrot, or monkey. 

dont generate trees around starting area. x

Go through each function and erase parameters we no longer need to pass. 


sharks in the water. 

enemy ghost. x -Uses skeletonAI. maybe it should shoot projectiles. or at least some kind of animation when it attacks, a decal? 
-maybe it could siphon health to regain it's own health, it slowly turn more and more invisible when it's moving and not attacking. On take damage it would become fully opaque and when attacking it would be fully visible when not doing these things it would fade out alpha. 

Added trapezoid UI bars for health, mana, and stamina. We lerp the values to smoothly animate them. Health bar color lerps toward red when player is hit. -another idea was pill shaped health bars. Maybe that would fit the theme better, it would be less code than constructing trapezoids. 

Made many changes to lighting. Shortened lightsource range for floors and lengthened the wall range. looks better. added a "saturation" variable. Fixed light.intensity to actually reflect changes to the struct. It was stuck at 1.0f when I thought I was changing it. 

Enemy stamina? 

Added fireball launcher model and got it spawning on vermilion colored pixels. Mark down vermilion color on legend. How will it work? Make a new map. One with lots firball traps. Long hallways with launchers set to different timings between launches. Already have a LauncherTrap struct. Needs timing vars on creation. How would we set timings on creation? 
void GenerateLauncherTraps, takes level data? Because what if we want multiple firetrap levels, it would have to be level data. 

Me and chat GPT came up with a ingenious encoding method for fireball traps. There is a vermilion colored pixel which is the trap itself. A yellow direction pixel that is an orthogonal neighbor to the trap that marks the direction the trap shoots, and a darker yellow timing pixel that can be one of 3 different shades of yellow. darkest yellow = 3 second delay, then 2 second and brightest is 1 second delay. The timing pixel is always on the opposite side of the direction pixel, that way it is easier to look up and it looks like a 3x1 line of red and yellow pixels that look like little launchers on the map. it took 2 attempts to make it work but LauncherTrap struct now contains all the info needed to launch fireballs. - Consider a blueish pixel for ice traps. use the same direction and timing colors. 

spider death animation not running. x - we never implemented a case for spiders. 

only enter free cam if in debug mode. ~ x -debugInfo must be true for tab button to work. 

Character patrol not working -(if !isWalkable) continue; was set to if (isWalkable) continue. So if it found a path it just skipped it. This was effect all characters. Wonder how that ! got erased. 

I tried desaturating the wall model texture and floor model texture. Making it neutral gray and trying to color it with just the tint based lighting. Color would only be applied to models within the light range so unlit tiles would be a flat grey that looked bad. I tried giving the wall textures a blue gray tint, but in the end it looked best with the default wall texture which is slightly reddish purple, almost maroon....marooned. I changed the "warm tint" for floor tiles to be even more yellow which mixes with the default blueish color of the floor tile to make a pretty neutral gray that works and is a slightly different color than the walls so it's not just one uniform color like before with the gray dungeon. I though about making my own wall models. 

Apparently we can do per pixel lighting with a projected light map and a shader. Say a texture twice the size of the the 32x32 pixel image. Would have lighting twice the resolution we have currently. why not a texture 10x as big? The baked lighting pass would "fill up" this texture map with light contribution values. I guess according to the larger light map we would light each pixel based on the light contribution values we saved while baking. Light contribution values come from the 3d raycast from light sources. Would we keep per model lighting for dynamic lights? because raycasting to every tile then calculating the contribution every frame. It would only need to be 400 pixels distance. Wait... we ray cast to each model. This gives 1 value. maybe we wouldn't rely on the raycast. all the info would be baked into the lightmap image. Need to ask some clarifying questions when the internet comes back. Maybe a transparent to white gradiant starting from the center of the yellow pixel. How would you do LOS lighting. actually paint in the lighting pixel by pixel. At a higher resolution to get sub model size lighting.
Need an example of a projected light map. Need internet...

internet was backup for a minute there. Apparently we generate the light map with code. but it would only work for the XZ plane. we could make a second lightmap for walls ZY plane by doing some math on the first lightmap to generate the second i guess. Lightmap doesn't need to be a texture, just an array that is filled with light values. say a 128x128 array of vec3? would be 4 times better resolution than per model. That means 1 tile would be lit down to 1/4 of the tile. perfect for the mini floor tiles. We would light the pixels of a 200x200 tile. We might need to do something fancy to light the mini tiles. 

Could we make wall tiles occlude the light for floor tiles? iterate the light map and skip anything beyond the range of the light. then iterate the 2nd lightmap for walls. skip tiles without raymarched line of sight? 

Implemented floor and ceiling lighting based on 128x128 generated light map. A shader reads the lightmap and colors the models per pixel. 4x the resolution we had before. Still working on getting dynamic fireball lights to look good. Stil over saturates the tiles and turns them green. Next we can try doing the XY for walls instead of XZ

Tried to make dynamic shader lights. made another light map for dynamic lights that gets checked everyframe. and updates the shader. If we try to combine baked light with dynamic light it fucks itself. 

-Try having only dynamic light map. just add static lights to it then we'd be reading off of 1 map not trying to combine. We were only baking lights because we were doing world LOS checks but we aren't doing that for floors right now anyway. 

-Tried and succeeded so far. For now we use a single dynamic lightmap, we load both dynamic lights and static lights and update it every frame. We aren't doing LOS checks on floors so it's fine and runs fast. LLM was saying we could do dynamic occlusion stamping or some such. We would iterate every light source and do a LOS check for walls and only stamp the light on places that aren't occluded. 

-we now run the lighting with occlusion check once for static lights. We save those values and copy them to the dynamic light map every frame, no need to recalculate every frame for static lights. Dynamic fireball lights just don't have occlusion because it would be too slow. Maybe it would be slow you should try it and see. it might give cool effects. it took 3 days and we tried like 3 different approaches. What worked was first just adding worldLOS checks to the stamplight function. It was super slow, so then we tried to optimize. but it didn't get much better. Then I realized how we could bake the lighting and still use the dynamic map. We run the LOS checks once and save the light values to a seprate GStaticBase vector of colors i think. and then when we run the dynamic lighting we copy gstaticbase to gDynamic so it has all the static lights before stamping the dynamic ones. I also made a lightsample at the players position so we can have shader light on the player. Looks way better than the tint based player light. 
-Consider sub tile lighting for static lights close to walls. 


When traveling to the next level by door, The next level's lighting is completely off, like dark. When you go to the menu and load the level from there it works. -made a hack solution where we quicly switch to menu when fading out. Then if in menu, if switchFromMenu = true we call init level and it loads correctly. My guess is that when we switch to menu, no other code runs but the menu code allowing us to cleaning switch light maps. I made another state called loading level. and switch to that instead of menu just to make things more clear and it doesn't work. The lights are broke, even though when loadingLevel no other code runs, just like menu state. 

-what is different about being in menu state that allows the light map to switch correctly? -we are drawing the menu, we are updating the menu, we are setting level index, It's not fadeout...we may never know. 

chests are broken. opening one chest means all other chests are open from then on. replaced all chests with keys until we can fix it somehow.

lava tiles with their own custom shader and light source attachment. 

implemented lava tiles. made a new floor tile in blender out of a cube matched to the size of the floor tile but 1/4 has high. Attached lava texture to model. sample lava texture somehow with shader and use world UV coords to makes the texture applied across all tile seamlessly. Had a bug where we were drawing a giant lava texture in the sky the tiled infinitely or at least to clipping plane. Just added a pixel discard to shader if world.y is over 500. Caused by world UVs and the world being very large. Still need to add lights to lava somehow. bloom doesn't pick it up. 

Spent two days trying to make the tiles above lava tiles glow red. Was successful in the end, but the effect is meh. Maybe the tiles around the lava should also glow red. But if it's gonna be meh, I don't want to try it. Anyway the lava glow can be used for more than just lava. We can now color an arbitrary tile any color we want. Our light map uses RGB for light color and occlusion, and uses A, the alpha channel to add glow to tiles above lava, or for example light a path along the floor. Maybe for tutorial reasons, or maybe a boss advertises his attacks and the floor lights up beneath you before an attack. It would take more work to do it dynamically. 

I think we need a better way to get floor tiles, we get what tiles are lava by making a lava map. Shouldn't there be an easier way? We already have a map. 

It's always sort of looked like the enemies were floating above the dungeon floor. I gave them a shadow model with a shader and it improved it a little, I needed to move all the characters down like 20 units so it looked like they were on the floor, but instead I just moved the dungeon floor up to 100, instead of 80. Luckily everything is tied to that one parameter floorHeight. So now the enemies look like they are properly grounded and with shadows to sell it more. It also made the player height shorter. Which I think is better because pirates were too short, this makes player more at eye level with them.

Implemented lava pits. Before we were faking it by dropping the camera when you stepped in lava making it looked like you took one step down. I lowered the lava tile height to -200. So there is a 1 tile drop down into the pit. This made a black void in between the floor tiles and the lower lava tiles. So using the lava mask we make a "skirt" by spawning walls on the edge of lava tiles. the walls are dropped by 400 so they cover the void. The next problem was adding collision to the subterranean walls. We ended up using the old makeWallBoundingBox along with a lot more rigamarole to get them positioned correctly. but Now everything lines up perfect and it looks great. There is an extra 50 units you can stand on near the edge of the pit where the skirts are but the bounding check makes the player stay at the right height perfectly. Once you fall into the pit you need to jump to get back out. Which is what I was going for. 
-I spoke too soon. We don't use MakeWallBoundingBox any more. It was 90 degrees of for differently shaped pits. We now generate the bounding boxes from the wall skirt once we place them. Had to flip the x and z when generating for some reason but it now all lines up. 

lava still doesn't glow. Edit the post process shader to make bloom pickup the lava. maybe a separate Glow shader. 

Consider making deeper "death" pits. 

-clean up dungeonGeneration.cpp, it's loaded with static inline funcs and vars defined right about the funcs that use them. very sloppy. Limit dungeonGeneration.cpp to just the generation of dungeons. We have 2 main places that control the game. DG.cpp and world.cpp. World was supposed to control the over world and dungeonGeneration was supposed to control the dungeon. Oh and main.cpp which "updates context", consider moving all these update functions to world, updateContext func that just calls all the other funcs. 

A sky box for dungeons? Night sky, stars, moon, maybe add on to current skybox shader, could we do a star field with just a shader? -implemented skybox shader night time branch. if is dungeon generate noise and only show the very tops of it. makes it look like lots of points of light. 

-added tone map to bloom shader. turned contrast down to 0. we now add contrast via ACES tone map plus 1.0 exposure. 

make fireball emitter a sphere not a point for fireball trail. x

bullets should hit lava not stop at floor above it. x

make UI bars centered for all resolutions. 

When exiting dungeon to over world the dungeon entrances are colored black. 

other island entrances now start locked and unlock after dungeon 3. 

If your going to make a demo, you need a way to communicate the controls to the player. How would you do this? Do it like you did the last few times. text pop ups before each new thing. Start with "WASD TO MOVE", than once they move "Left click to attack" "Q to switch weapons" Then do a controls page in the menu later. Ok how would we do popup text? 

inside UI.h make Class Popup.it has a position on screen, a width and height, a std::string "message". Maybe a popup could have some kind of state. Like showing first message. Maybe a table of messages.
-we have a vector of messages inside a class hintManager.h/cpp We instance hints in UI, setup tutorial text, then iterate the vector of std::strings depending on the players action. We update the messages inside hintManager because all we need is the player or input keys. Should probably be in UI.cpp. It's called UpdateTutorial and could be put anywhere I'm just afraid it's going to be hard to find later. 

Do we need any more popups? The staff. The interact popup should popup when you are close to the dungeon entrance. 

Test other styles of font, don't just take the first one. 

38 days left. What is it missing? 

How hard would it be to shrink the world? Very. We sized the tiles to 200 on a whim. and the floor tiles are x700 before that. Could I make like a global size factor. 

make a new island. make a portal out the the archway with a shader that animates a multicolored door. The portal could be at the end of the last dungeon and your teleported to a new island. Make a boss. 

09/25/2025

Fixed tree shadows. We now stamp the shadow onto the terrain with a shader. So shadow quads are no longer floating in the air sometimes.

More sounds, sound of bullets hitting wall. jump sound effect. ghost sound effects. enemy hurt sound effect should be positional. 

River level was crashing because there were no entrances. over world level must have entrances. or account for this. 

Can we just apply the lighting shader to the launcher model and it will just work? Yup it just works. That means barrels and chests can be lit by the shader as well we don't need to tint them. -need to ad more dungeon props if we can light things so easily. 

maybe we could tint the gun and sword model with the same shader? it would probably make it too dark in dark areas. My guess is using our current tint method would work better. 

Take another stab at adding a texture to the grass some how. Can we apply a texture to pixels that are over a certian height on the heightmap? you can sample a texture with a shader surely. You tried it before and it wouldn't work. Maybe it was something to do with the vertex shaders UVs or something. 

-This time it worked. I think because of all the experience with lighting shader. 

Added fog for distant terrain. Then added treeShader which is applied to trees, bushes, entrances, and doors. So it should probably be called fog shader or something. It just blends the colors toward background sky color depending on distance. 

make swimming work. -added swim sound effect, works the same as foosteps but for water. 

Make ghost slowly turn invisible when not attacking. It should start and remain visible until player enters vision. then slowly fade out, intantly fade back in if attacking, then slowly fade out. Upon taking damage instantly fade back in. 

Commision an artist to redraw all the enemy sprites. 

Commision a musician for an ambient music track. 


make a boss. A larger sized sprite. A big Dinosaur, A T-Rex. It would have to use character code. I guess same as raptor so it's not just tracking and following player at all times. Character size is hard coded? It's a 200x200 image. scale scales it like it should, I made raptor scaled to 2.0 instead of 0.5 and it's as big as a t-rex. Needs new art. CharacterType::T-Rex

generate the art as real art not pixel art. Resizing them to 200x200 can pixelate them enough. Do this with pirate and skeleton as well. Could be just what it needs. -AI is still trash at reproducing an image. Find a human source. 

LEVEL 4 staff level. nuke it. -edited it, took away the staff until we fix the issue. there are 3 keys on two locked doors now. So you don't have to visit every corner of the map. 

Freezing an enemy than killing them, has a chance to make them invincible some how. maybe they get stuck in stagger? but they can still attack and move so they must be switching states okay, just impervious to bullets and melee and fireballs. I don't want to go looking through the character AI code. 

maybe give the staff at level 9. If you have been playing that long, you will forgive the bugginess. 

-make has staff stay on after you pick it up. How? global var in world. isn't reset by level switch. 

Played for 20 minutes, had 1 bad bug. Where skeleton was invincible. and game breaking bug where level 7 or something didn't have an exit. -moved the staff to a later level, once you pick it up you have it from then on. investigate the freeze bug. 

push then make a secret release. 

Could we make blunderbuss fire make the players light intensity and range get higher for .25 seconds? -Made intenstity and range change for the 0.1f seconds we show the muzzle flash. Triple the range and I actually lower the intensity, because it looked pretty jarring. over time I think I grew to like the effect. 

Had the option to have light leak through closed doors or light stop at closed doors. Chose to let it leak. It only ever looks bad when walking up to a closed door that has a light on the other side. Most of the time you get a cool effect of the light coming through the open door. I check for side colliders on doorways so the light only comes through the middle door part. Let it leak, until you can figure out how to recalculate baked lighting live. 

replaced smoothPath with SmoothWorldPath, where we smooth the world path instead of the tile path. Using the worldLOS checker, this is more reliable than the ray marching and theoretically would make the enemies use diagonals more and not be so rigid looking. In practice it looks about the same. Next try adding diagonal steps to the BFS. 

Finaly switched over to using the depth test and mask to sort the billboards. I was hanging on to my custom billboard sorter just because I put so much work into it, when all I needed to do was add an alpha cutoff shader to all billboards. So I went ahead and did that and it works, and now the T-rex isn't occluding bullets. I thought I might need the custom sorter for blended toward alpha edges on sprites, but I haven't needed semi transparent or blended billboards yet and don't think I will ever need that. We still manualy sort the billboards for no reason. When we put all the billboards into one vector of struct drawRequest. Having all the draws in one vector seems like a good idea, instead of character, fire, web, door, ect all having their own draw function. Although we have the gather function for all those things. I don't really want to go back and dismantel it, maybe there will be a reason to sort the billboards manualy in the future. 


14 days, it's basically done. There is even a boss. Take out some levels? Maybe skip some levels. Like level 3 could jump to 5. Make like a story board or something, and reorder things. 

it goes: 0, 2,3,4, 0, 6, 8, 9, 10, 11, exit to river (1) 
-should we exit after level 10 and make another entrance for the last 3? I think there is an island that doesn't have an entrance - for the demo keep it as is. The user would have to do more exploration to find the last door, and we don't explain anything. 

maybe make an entrance on the last little island, it starts locked, but once you have completed say level 6, it turns into a portal door that leads to river boss level. 

Play through the whole thing again and time it. -27 minute speed run. No game breaking bugs, oh one where I reentered dungeon 3, the entrance is suppose to be locked upon exiting dungeon 3. entrance 2 should remain locked at all times, wonder how that happended. Level 13 is super baren of enemies, it's an ok level but I just skip it, the portal on dungeon12 leads to river level with t-rex. maybe skip dungeon 12 as well? mmm yeah mmm no, it's set up with a portal already. It's kind of a shitty level because you are just randomly opening doors to little rooms until you find the right one. placing a portal is as easy as changing the color on the png map. Portals are just differently colored next level doors. -changed last dungeon to be lava level. skiping 12 13, these levels are still available from the menu. 

1. give t-rex more health
2. lock entrance 2. 
3. push
4. make release. 

Make resolution changable from main menu. -Resolution start fullscreen for whatever monitor. By simply using raylibs ToggleFullscreen() which makes it borderless window fullscreen automatically. When you select the fullscreen toggle from the menu, it drops out of full screep to 1600x900 window. Maybe the menu button should read windowed and when you toggle it, it would read fullscreen. It reads "Toggle Fullscreen" at all times now. The word toggle works for both situations. 

Added Kingthing font for tutorial text, had to redo the shadows to properly show. Looks better IMO, maybe the text could be bigger. 

-redo the art using ImgToImg. Generate a idle pose and use imgtoimg to make other poses. can only do one image a day. better make them count. -Tried comic book style but when I shrink the size down to 200x200 it becomes a muddy mess. If I try pixel art style it's way to simple and what I have now is better. So what to do...

    -edit existing sprites pixel by pixel, there is only 5 frames each or so. 
    -pay for an image generating software beyound chatGPT. 
    -pay for an artist on fivr. 

Use the skeleton walking away animation somehow. If the distance to player is rising, play row 5. else play row 2. distant skeleton would be seen wandering away half the time. Would we need a whole new case? Check if the current animation is walking, measure distance over time, switch animation based on isLeaving boolean. isLeaving could be a member of Character and could be used for pirates and spiders. It would work best for pirates who actively run away. Maybe implement it for pirates first. If pirate patrol measure distance and switch anim. 

Added isLeaving bool to character. It is updated to be true if the character is moving away from the player. We can then play the leaving animation when isLeaving is true. It works for pirates, sometimes it looks a little weird but at least they are not moon walking backward all the time. Raptors were trickier because they already have run away behavior. I just added isLeaving check to the runway case. So only show your ass when state == runaway and you are leaving. This makes them show their rear less often wich makes less moon walks still not perfect though. It's hard to judge becuase of the hysteresis.

we could add isleaving to skeletons but I think it might look bad. and skeletons are like never leaving. they always chase. if you see them from a distance patroling they could be moon walking. Redo skeleton walking away art by doing the same thing you did with pirates. Just pain over the face to look like the back of their head and so on. 

for a split second it's showing a rectangular frame when killing an enemy with the gun. it must be decal? or muzzle flash. -The solution was to just remove smoke decals. We no longer spawn smoke decals when hitting enemies. It looks better and doesn't show the bug. I guess we were getting z fighting becuase I upped the amount of bullets to 8 instead of 6. that is 8 smoke decales if they all hit. Any way it looks better now. 

Implemented attack/block decals. Give way more visceral punch to the attacks. Also added stab sound effect that should have been on the skeles. We spawn an animated decal of an arc, like a swoop comic book action line 100 units in front of the attacking skeleton. It swoops down in the same arc as their new sword swing animation that I also made. This works for multiple skeletons because each spawn their own attack swoop. Also added block decal for when blocking these attacks with a sword. It's just a white icon of two swords crossed. This also works for multiple skeles and it looks good blocking two enemies at a time. Also added a bite animated decal that plays when raptors or trex is biting the player. works the same way. Just an icon of teeth colored red that animate in a biting motion while alpha fades out. Spiders also use the bite decal. Consider a decal for spider bites, maybe they could be green and poision you. 

-spawn a decal in front of the player when he is hit with a bullet. The decal should be a bullet whole in glass?

what else needs decals. fireballs? spider bite decal, bullet hole decal, player melee attack? Should we play a decal when you swing the sword? It would need to be positioned offset to the right. Need a forward swoop. First frame is the full swoop shape. Then erase from top to bottom while fading out alpha. Or start frame is empty and animate the swoop being filled in. Look at examples some where. -Added a semi transparent animated streak that plays when you swipe sword. Looks kind of lame. but I guess it's better than nothing. Maybe it should have more frames because it's not exactly smooth looking. 

Go through and rename all maps to their level names, for sanities sake. name map4 -> map1, That way dungeon1 is map1. Dungeon2 = map2 and so on. Consider giving them code names? Would make it easier to remember, like I know lava level is level 10. dungeon1 can be called dungeon1. dungeon2 would be called. . . 3 doors. Dungeon3 would be called, 3 pirates, dungeon4 would be called, Room maze, dungeon 5 would be called. . . 

6 days. Is there anything that looks unprofessional. Pirate sprite is bad, needs to be redone. Trex fight could be cooler but how many people will get that far. sword swing is lame. The way it snaps back to the starting position looks bad. Trees and bushes can still spawn on slopes making it look bad. Shift to run is never explained. 1 and 2 to use health and mana potions is never explained. It's explained when health is low but mana isn't. Maybe the use can infer. Right click for magic staff fire isn't explained neighther is T for switch spell. Should we brighten the dungeon? Nah don't touch it. It looks better dark, only brighten for videos. Re do the readme. That looks very unprofessional. It's AI slop. Write it your self. 

Maybe a decal that plays when an enemy is hit by bullets. We did play smoke before but it just got in the way. Instead of smoke play small bullet whole texture for each bullet, projected 100 unit backward toward player. -It's hardly noticable but I guess I will keep it. 

Made some decals have a velocity. When skelton swipes a sword at you the decal move toward the player as fading out. So it looks like it's coming at ya. Player slash decal move away from player as it fades out. This maybe looks worse than having it just stay in one spot but I haven't decided yet. Use this technology on ghost attack. 

-idea: make decals have a collision box. Ghost could shoot decals at player. decals are billboards so the projectile would look the same from any angle. Maybe make a new enemy that shoots decals. 

Get a different tile texture for ceilings. 

take away decal velocity. It looks dumb, and it doesn't make sense. They attack is localized to the one area while swinging it doesn't travel forward. It was a good idea to add velocity to decals however. The velocity parameters will be needed if I make ghost projectile a decal. 

Make ghost projectile use decal velocity. 

Does pirate attack only happen on a certain frame? is that why his first hit doesn't seem to connect?

Holding block with sword and switching to blunderbuss while still holding block lets you block with the blunderbuss. x

Skeleton can still become invincable if you freeze them. fix this. I think it's fixed but could it happen for pirates?

Make another skelton death frame where he is half way falling over. 

fixed pirate body disappearing immediately after death. death animation was set to 3 frames instead of 2

pirates are still missing their first attack. 

Dungeon lights not working for person testing it on Linux. It could be his hardware. He said he was on a laptop. Maybe doesn't support GLSL. Maybe slow load time effects the timing, and skips the frame we calculate lights? We need to load in all the geometry before we calculate the lights. Maybe wait like 0.2 seconds after loading geomtery then do lights? Impossible to test. 

You can use potions while dead. x

Gold uses raylib font. x

you can sprint indefinitely. Consider removing stamina and just keep infinite run. 

on laptop bullets seem to tunnel through enemies. maybe turn the speed back down to 2k

island level on laptop is unplayable. the terrain shader is broken. Maybe I'm running bloom shader outside when I don't need to be. 

Made the demo only the dungeon levels. At least it's something. Fix bullet tunneling asap though. For people with low end PCs. I didn't even think of people with laptops playing the game. I just assume every one has a 3080 by now. I figured my 1070 was way low end. Integrated grafics havn't gotten any better in 20 years. 

Hiting start game should fade out to loading screen. How the hell would we do that?

bring back destroy all vegitation button. -tried it on laptop. Makes 0 difference. the slow down isn't comming from vegitation. 

If I switch to windows 11 is the terrain shader gonna be broke on this machine too? or is it because of the integrated graphics on the laptop just can't cut it. 


working on terrain chunking may have found why lighting was broken unless loading from menu. Need to investigate further. Just added EndTextureMode() at the beginning of initLevel. If we were switching levels mid game loop. EndTextureMode() never got called and it polluted the next render texture. 

Huge optimization win. Drawing the terrain as one big model turns out was a bad idea. The game should have been running like 30 percent faster without that draw call. So we now do terrainChunking where the mesh gets split up into seperate models. Some really complicated code I don't understand breaks up the terrain mesh into model chunks that get saved to a vector of struct terrain chunk that has a model member. iterate the chunks and draw each one. They slightly overlap by one texel so there are no seams. 

We were also loading this massive terrain model on dungeon levels for no reason. We have a dummy heightmap on dungeon levels. Some character code still references the heightmap, so we kinda still need the dummy unless I go through and find all the places we reference it and do a if isDungeon check. So i just made the blank dummy heightmap 64x64 instead of 4k. So we are loading a tiny little plane that never gets drawn just as a placeholder. 

All these optimizations make the load times like instant. I am glad to know it was my stupid self that was fucking up and not a hardware problem. This restores my faith in computers and C++ as a fast language. If I tried to make this thing in Godot there is no way in hell it would run as fast as this, and I can still optimize way further. I don't even frustum cull yet. 

chunk draw distance is set to 15k. this hides things poping in pretty well. The laptop only gets 20 frames with this draw distance and improves as it gets lower. Maybe I could make 2 builds, or a setting in the menu, like a slider for draw distance. I've seen that in games before. 

Spider boss on level 7. the one with the big room and a single spider in it. Maybe this boss could become just another enemy in later dungeons. just take the existing sprite and scale it to 2.0? Still got plenty of tokens on ImgToImg to make a better looking boss spider. It could leave a trail of web. it could shoot web decals that freeze you in place allowing it to move in and attack. It would work like pirate where it moves into shooting range, stops and shoots. If there is a hit, only then move in for an attack. otherwise keep shitting out little spiders at a distance. Thats a way better boss fight than the t-rex. 

Spider boss would need it's own state machine. We would need a spawn spider at position function. Maybe the hatch from eggs. That's an idea. spiders in general hatch from eggs on a timer once you enter a certain range. Killing the eggs immediatly would become the task when entering a room. Plus a cool hatching animation, and egg exploding animation when you destroy them. 

Terrain textures aren't working since we switched to chunking the terrain. Maybe the vertex shader needs to use world UVs or something like that. I remember for lava we had to change the vertex shader to get the texture to spread across all tiles. Texture not spreading wasn't the problem with terrain tiles though. It was all black IIRC. Still the vertex shader would need to be changed. 

-fixed terraint textures for chunked terrain. It looks glorious and it works on the laptop and frame rate improved I think. It also doesn't turn black when switching back and forth between dungeon and island a bunch. We asign each texture to a material slot like we were doing before. LLM kept lying to me saying that it was overkill. It's the only way it would work. I think before we just didn't have the correct vertex shader. I remember glancing at the vs and saying to myself looks good to me without really checking. 

Raptor death is slightly too fast. x

Optimized the water plane. Got rid of the bottom plane. Found it was not neccesary. saves drawing a 16k plane. 

3d door models, that use the current door sprite. Very thin rectangle. It has a curved top, would transparency work?
The door would rotate 90 degrees when opened. The open model wouldn't have collision, it could though. Your making a bounding box anyway basically when generating the model. Door is already a struct. Just add another member model to the door struct. generate the model first? Once on generateDoorways so we have the rotation. Where would the pivot be? it would have to rotate on Y 90 degrees but origin needs to be bottom corner not middle. 

chatGPT is no good at spiders. 

Got a decent comic book style giant spider. Maybe a little to cartoony but was the best I could do. Made a whole sprite sheet. Need to rig it up. Idle case should be different. He should not chase unless, he's laid an egg, and is within a distance for a certain amount of seconds then he will chase and attack, then back off and lay another egg and repeat. Could reuse reposition state to reposition to another random tile like 1000 units away. 5 tiles away. 

generate art of a spider egg hatching a baby spider. and also exploding like in the movie aliens.


spider web projectile decal. 
spider egg that hathes normal spiders. 
regular barrier spider web spawner? in 1x1 hallways the spider could place web barriers. Make the boss fight inside a maze. when the spider runs away it doubles it's speed and takes off to the outside of the maze faster than the player can chase. 

a vector of retreat positions we can randomly choose from for the spider to run to when retreating. Nope. Needs to be level agnostic. get random tile a certain distance away. 

Implemented the run away, and chase behavior of giant spiders. Added spiderAgro bool and spiderAgroTimer, and accumulateDamage to Character class. Spider starts out not agro and will run away when you first see it. It picks a random distant tile, it tries to build a path to that tile, if it finds a path and the length of the path is less than 30 tiles, the state = runaway and it follows the path. It will continue to run away until it takes 200 damage, or the spiderAgroTimer is greater than 10 seconds. float accumulate damage tracks how much damage has been taken before switching to runaway or agro. if accumulateDamage > 400 spiderAgro = false, if accumulateDamage > 200 spiderAgro = true depending on agro state. 

-we could add a distance check and timer check to runaway case. if agroTimer < 10 and !hasLayedEgg: lay egg, agro = true. You would need to chase him down and kill the eggs he...she is laying. Maybe boss spider can lay eggs and normal Giant spiders dont. So we could use generic giant spiders in later levels and have a boss one that lays eggs. 

The boss arena is filled with little corridors and choke point with little nooks and crannies the spider can run to. There are spider webs blocking the paths. Spider can walk through spider webs, and player can't. 

Spider runs away too much. make run away time 5 seconds. or maybe it's good. Maybe we should add little spiders to the mix for the time being and see how that feels. 

make little spiders minature version of the big spider sprite? If we shink giant spider less than 300 it will get distorted. frameHeight...make the frame height of spiders 300, make scale 0.25? or make another spider sprite. 

Fireballs hitting skeletons doesn't play explosion animation and does little damage to skeletons. Make it one shot them. x

Solved level switch lighting bug. The gDyanmic.tex was assigned to texture0 by the shader. When loading the level not from menu, texture0 was being overridden by a 2d draw call. When we loaded from menu texture0 was set to backdrop texture, then gDynamic.tex got set to something other than 0 making it not get overwritten. or something. I guess we were assigning the lightmap texture to the first texture slot which is a bad idea. That bug has been there since the invention of the lighting system. We can now rebuild the light map live while the game is running. It still stutters and flahes dark for a frame so recalculating lighting on door opens doesn't seem doable. 

fixed isLeaving flag. We now account for player movement vs enemy movement when determining the animation. The relative motion. When back peddaling with dinos chasing you it was showing their rear end. becuase player speed is faster than dino's speed. So it was like the dino was moving away from the player when player was moving away. This solves for all characters not just dino.

Could add skeleton rear animation. But skeletons never retreat. same with little spiders. They reposition 1 tile when bunched up. It would look better for patrolling skeletons, they wouldn't be moonwalking. Low priority. 

shooting the Giant spider should close the entrance to the arena. and lock it. Easy on take damage if giant spider lock door ....how to get door. just close all doors. lock all doors. void CloseAndLockAllDoors();...or just get the dang door position some how. DO we iterate top to bottom? when building the level? -hard coded doors[5] we iterate the dungeons top to bottom and it was the 6th one down. it sets the door to closed and sets it to eventLocked. 

When entering the Giant spider boss arena, once the spider takes damage the door closes and even locks. trapping you in there with the spider. What if the spider exits the room before you deal any damage to him. Maybe it should trigger on first sight. x  -Spider now triggers on first sight. inside idle case. spider doesn't patrol, so we can better control the encounter. 

Added bootleg frustum culling. We use math to project a cone in front of the camera and cull all chunks outside of it. Chunks beneath player were also being culled so added a non cull zone around the camera. It's speeds up the lap top on island levels by minimum 5 frames up to like 30 frames faster. Depending on how many chunks are being culled. This might make it a pain to play with an inconsistant frame rate. If your standing on the side of the island looking out to sea, it gets almost 60. So I don't think culling the tree this way as well would make much of a difference. We could drawModelInstanced the terrain as well I think. So it would be like 1 draw call instead of 250. Oh we also added a hard limit to the number of chunks we render. closest chunks get drawn first out until the limit. This is what gives it that minimum 5 frame boost because we are culling the island on the opposite side of middle island because we run out of chunks, not because distance. 

I tried making the chunk super big. As to make less draw calls. but if the tiles get too big they don't conform to the terrain. So you get huge seams. I made the chunks size 193 instead of 129 was the best I could do. The number needs to be a multiple of something I forget. 

Redo little spider sprite. 

make spider egg. 

started on spider egg. It is it's own struct. not a part of Character class. PNG image color is slime green 
(128, 255, 0) spiderEggSheet has 3 frames for each row. needs to be filled with frames because we use 1 frames per row variable. So idle would be 3 of the same idle frame. eggs have their own bounding box we need to check against bullets and sword and staff. Need to try again to genereate exploding egg animation. Have "dormant" frame and "hatching". 

Eggs have health. If egg health <= 0; change state to destroy, which plays destroy anim then changes state to husk. 
Eggs have a trigger and hatch delay timer. They should be triggered when player is within a radius and has LOS. Timer runs to 0 then chagne state to hatching, spawn spider at the center of the egg's tile in world coords. 

would spawning a spider in vision of player be a problem? shoudn't be. 

Need to make boss spider lay eggs. At the spiders retreat tile, if can lay egg, lay egg. 

Eggs start dormant, when player is closer than 300, the start "awakening", after five seconds they are "hatching" after the hatching animation play we switch to state "husk". No matter the state, if the egg's health drops below 0 we switch state to destroyed. The destroyed animation is just 1 frame for now. We would need another timer to play the animation. Maybe just reuse hatching timer. 

Had a bug where standing on top of the egg destroyed it. Because we weren't checking if melee hitbox was active. if melee hitbox isn't active, it's extents shrink to 0 right at players pos. do it can still deal damage at the exact player position if we don't check active. Basically telefragging anything on top of player. Since I added collision to the eggs the player can never be right on top of it, but I check for melee hitbox active anyway, consider doing this for all enemies, and barrels. 

Make the eggs activate on LOS. x

Make the giant spider lay eggs. x - giant spider can lay an egg after running away for 10 seconds. only if the 10 seconds are up does it lay not if it takes enough damage in that time. So it seems to work pretty well. I thought I needed a egg laying timer, but it seems pretty good as is. We spawn the egg based on tile x, y so the egg is always centered. Easier than I thought it would be. 

Tried one more lighting fix for linux. I now assign a dummy texture before setting lightmap uniform. The dummy draw assigns the dummy texture to slot 0, making the lightmap texture use a slot > 0. So then we don't have to us rl calls to assign it every frame. Although if this were the issue it should have worked being loaded from menu. LLM says there is nothing wrong shader side. says it has to be uniforms. 

blood decals are slightly occluding the enemy sprite, mostly spiders. Do we spawn the decal ontop of the enemy position or slightly ahead of it? -We were spawning decals on bullet hit from bullet code. Moved the decal spawning to happen once on character takeDamage. Solves occlusion problem and simplifies bullet code.

spawn goo particles on egg hatch. x

made player melee attack id that gets incremented every swing. we check if the enemies last attack id != player attack id as to only apply damage once per swing. This is better than checking if hitTimer <= 0. For both enemies and eggs.
Could give enemies their own attack ids for hitting player once. Maybe a unique id for each bullet. 

Removed glow on ceiling tiles above lava. Lava levels all have no ceiling anyway. It simplifies the lighting. and the alpha channel stuff could be the problem. 

In debug mode we draw the lightmap texture in the corner of the screen. debugging was showing a problem with alpha channel lava glow potentially. Ceilings were all tinted red when lights were not working. means alpha was being set to 1. Because the texture wasn't correct? If we were setting the dyn.tex.alpha to 0, but using a different texture in the texture slot, the other texture's alpha would be 1 making the ceilings red. That means on linux the texture was getting squashed even when loading from menu. Is it because we are doing 2d calls before and after 3d. like the lightmap tex gets crushed by the 2d draws at the end of the render frame? Either way the debug lightmap draw should show if the lightmap is getting crushed. the size of the texture would not be 128x128 and the debug texture would be all black? all white? transparent? it's alpha would be 1 and it's rbg would be whatever the texture that is crushing it is. 

set pirate to only apply damage on frame 2. still missing his first attack.  


Ok we will try with limited sprint. Make infinite sprint on debug mode, maybe even a button for speed increase like running in debug makes max speed even faster. x - made sprinte drain 50 percent less. recharge stays the same. So drain and recharge are both 20 * deltaTime. 

added culling to underwater chunks. x

testing on laptop found that dungeon 3 still spits you out on little island, not dugneon entrance 2. fix this now

Giant spider turned invincible on death some how. 

Finaly found a solution to lighting not working on Linux. Just make a whole new lighting system from scratch.
Forward Lighting. We no long have to bind a texture that the shader can read, which was the problem. I'm sure I could have found a solution to the bug if I kept looking for another week or so, but I had a fix that would work now so I did it. We still use a texture as an occlusion map. It's bound to the materials slot 3 on each model, then the shader reads from the material slot? not a custom texture. and that's why it works I guess.

The new forward lighting is worse resolution that what we had before. This is the price we pay for cross platform. Maybe one day I will find out what we were doing wrong with the lightmap but until then we have working occluded lights for both windows and linux. 

I made the range of the lights 3000 instead of 1600. I think it makes it look more natural. Light travels forever in real life not just 1600 units. With the new forward lighting it looks batter that way. We don't get as cool of shadows on the floor tiles, but we get occluded walls pretty well with the longer range. 

All told it took about 8 hours. The original lighting took me 3 weeks. All the scaffolding was in place it just needed a new shader which LLM writes all of. 

    sh.locs[SHADER_LOC_MAP_DIFFUSE]   = GetShaderLocation(sh, "texture0");
    sh.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(sh, "texture3");

not setting these kept me up till 2am debugging until it worked. 

We could have just bound the original lightmap texture to a material slot like we do here. -yes

Reverted to lightmap using emissive material slot on all the dungeon materials. 
We set wallModel.material[4] = lightmapTexture instead of binding the texture to a custom unit texture. That's how it is supposed to be done. LLM was just binding it to a custom unit texture because it didn't have the context to see the models I was using. I even got a second opinion from another LLM and it never suggested binding to material slot. Even though that's the way it's done. Should have just read the documentation.

Reverting back to lightmap texture gives us much better resolution shadows, and works on Linux. 

fixed fireball light staying lit too long. -Added increase in fireball light range when explosion happens. 

Start making new and improved maps. Double wide hallways with lots of pillars next to light sources for good shadows. 
Uneven, non symetrical maps, less long hallways, more like level 12. 

Keepers: Dungeon1, Dungeon2, Dungeon7, Dungeon9, Dungeon10, Dungeon12, 

make 3d door model with door texture applied swing open on hinge. 

selective light map occlusion rebuild on affected door open. As to not cause a hitch in the game on door open. 

work on ghost. Make updateGhostAI. generate a better ghost sprite. 

raptors still show their ass while running toward player. Favor forward facing even more. x

Added small transparent smoke particles to default bullets. Added BulletParticleBounce function that takes a bullet and a color as arguments. sets alive to false exploded to true. keep updating particles if exploded, just don't draw or update bullet position. Creates particles that bounce off the opposite direction by getting the negated vec3


Added particles that bounce back from the wall when you shoot it. We get the normal direction from the AABB of th wall collider. Since everything is on a grid we can just know the normal of the wall collider. Ceilings and floors and vec3(0,1,0) and vec3(0, -1, 0) so it's super easy to make things bounce. That gives me the idea to make the blunderbuss bullets ricochet. 

make blunderbuss into a proper flack cannon. x
hasBounced 
bounceCount

make bullets damage based on bullet velocity. a single bouncing pellet could finish off a skeleton. that's fine. We wouldn't need to count bounces to set damage. just halve the velocity = half the damage. 

    float speed = Vector3Length(velocity);
    float vFactor = speed / maxSpeed;

    vFactor = Clamp(vFactor, 0.0f, 1.0f);

    return baseDamage * vFactor;

Since we can know the normal of all the walls in the dungeon is was easy to make bullets bounce. We can also get the normal of the enemy bounding box so bullets can ricochet of enemies, but only if they are grazing shots. We can control this because of the cosign threshold. if cosign = 1 it's a direct head on hit, anything less is more and more grazing, we can tweek this number until it looks good. 

Added flak explosion to fireballs. We just spawn bullets that fire away from center in a sphere. Already implemented bouncing bullets so it looks goods. Made the size 10 for flak bullets instead of 3. Bullet explosion is raised 50 units off the ground so the sphere doens't fire half underground. It's still not that overpowered. Does effective area damage on crowds of skeletons and spiders. I lowered player bullet damage to 15 instead of 25 because they can bounce now. 

Maybe spider hitbox should be lower. 

Enemies can sometimes get stuck even though I turned off wall collision. It must be smoothpath. Smooth path allows enemies to travel diagonally, Maybe we need a proxitmity check on walls. or maybe its a state machine thing.

fixed raptors running backward I think. By just setting 3 ticks for turning toward and 30 ticks for turning away. 30 ticks is 0.5 seconds, not 6/60..0.1 seconds like before.

Forgot to write down for future reference. Clamping deltaTime to 0.05 stops the game from stuttering and player falling through floor and fire animation starting fast.

float deltaTime = GetFrameTime();
if (deltaTime > 0.05) deltaTime = 0.05; Do this at the top of the main loop and pass dt everywhere from this. Consider fixing fast animation in DCN by this method. 






























