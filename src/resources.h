#pragma once
#include "raylib.h"
#include <string>


extern RenderTexture2D sceneTexture, postProcessTexture, depthEffectTexture;
extern Texture2D bushTex, shadowTex, raptorTexture, gunTexture, muzzleFlash, backDrop, smokeSheet, bloodSheet, skeletonSheet, doorTexture, 
healthPotTexture, keyTexture, swordBloody, swordClean, fireSheet, pirateSheet, coinTexture;
extern Shader fogShader, skyShader, waterShader, terrainShader, shadowShader, simpleFogShader, bloomShader, depthShader;
extern Model terrainModel, skyModel, waterModel, shadowQuad, palmTree, palm2, bush, boatModel, floorTile2,floorTile3,chestModel, 
bottomPlane, blunderbuss, floorTile, doorWay, wall, barrelModel, pillarModel, swordModel, lampModel, brokeBarrel;
extern Image heightmap;
extern Mesh terrainMesh;
extern Vector3 terrainScale;

extern int sceneTextureLoc;
extern int sceneDepthLoc; 

extern Vector2 screenResolution;

void UpdateShaders(Camera& camera);
void LoadAllResources();
void UnloadAllResources();

