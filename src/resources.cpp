#include "resources.h"
#include "world.h"
#include "sound_manager.h"
#include "raymath.h"
#include <iostream>
#include "rlgl.h"
#include "custom_rendertexture.h"
#include "dungeonGeneration.h"

RenderTexture2D sceneTexture, postProcessTexture, depthEffectTexture;

Texture2D bushTex, shadowTex, raptorFront, raptorTexture, gunTexture, muzzleFlash, backDrop, smokeSheet, bloodSheet, skeletonSheet, 
doorTexture, healthPotTexture, keyTexture, swordBloody, swordClean, fireSheet, pirateSheet, coinTexture;

Shader fogShader, skyShader, waterShader, terrainShader, shadowShader, simpleFogShader, bloomShader, depthShader;

Model terrainModel, skyModel, waterModel, shadowQuad, palmTree, palm2, bush, boatModel, floorTile2, floorTile3,chestModel,
bottomPlane, blunderbuss, floorTile, doorWay, wall, barrelModel, pillarModel, swordModel, lampModel, brokeBarrel;

Image heightmap;
Mesh terrainMesh;

int sceneTextureLoc;
int sceneDepthLoc; 
Vector3 terrainScale = {16000.0f, 200.0f, 16000.0f}; //very large x and z, after testing, 16k is what felt natural. and it's till solid 60 fps

Vector2 screenResolution; //global shader resolution


void LoadAllResources() {
    screenResolution = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };
    sceneTexture = LoadRenderTextureWithDepthTexture((int)screenResolution.x, (int)screenResolution.y);
    depthEffectTexture = LoadRenderTexture((int)screenResolution.x,(int) screenResolution.y);
    
    postProcessTexture = LoadRenderTexture((int)screenResolution.x,(int) screenResolution.y);
    raptorTexture = LoadTexture("assets/sprites/raptorSheet.png");
    skeletonSheet = LoadTexture("assets/sprites/skeletonSheet.png");
    gunTexture = LoadTexture("assets/sprites/flintlock.png");
    muzzleFlash = LoadTexture("assets/sprites/muzzleFlash.png");
    backDrop = LoadTexture("assets/Screenshot4.png");
    smokeSheet = LoadTexture("assets/sprites/smokeSheet.png");
    bloodSheet = LoadTexture("assets/sprites/bloodSheet.png");
    doorTexture = LoadTexture("assets/sprites/door.png");
    healthPotTexture = LoadTexture("assets/sprites/Healthpot.png");
    keyTexture = LoadTexture("assets/sprites/key.png");
    swordBloody = LoadTexture("assets/textures/swordBloody.png");
    swordClean = LoadTexture("assets/textures/swordClean.png");
    fireSheet = LoadTexture("assets/sprites/fireSheet.png");
    pirateSheet = LoadTexture("assets/sprites/pirateSheet.png");
    coinTexture = LoadTexture("assets/sprites/coin.png");
    

    // Models
    palmTree = LoadModel("assets/models/bigPalmTree.glb");
    palm2 = LoadModel("assets/models/smallPalmTree.glb");
    bush = LoadModel("assets/models/grass(stripped).glb");
    boatModel = LoadModel("assets/models/boat.glb");
    blunderbuss = LoadModel("assets/models/blunderbus.glb");
    bushTex = LoadTexture("assets/bush.png");
    floorTile = LoadModel("assets/models/floorTile.glb");
    doorWay = LoadModel("assets/models/doorWay.glb");
    wall = LoadModel("assets/models/wall1.glb");
    barrelModel = LoadModel("assets/models/barrel.glb");
    pillarModel = LoadModel("assets/models/pillar.glb");
    swordModel = LoadModel("assets/models/sword.glb");
    lampModel = LoadModel("assets/models/lamp.glb");
    brokeBarrel = LoadModel("assets/models/brokeBarrel.glb");
    floorTile2 = LoadModel("assets/models/floorTile2.glb");
    floorTile3 = LoadModel("assets/models/floorTile3.glb");
    chestModel = LoadModel("assets/models/chest.glb");


    terrainShader = LoadShader("assets/shaders/height_color.vs", "assets/shaders/height_color.fs");

    // Shaders

    //Post processing shader. AO shader + red vignette + fade to black
    fogShader = LoadShader(0, "assets/shaders/fog_postprocess.fs");

    sceneTextureLoc = GetShaderLocation(fogShader, "sceneTexture");
    //sceneDepthLoc = GetShaderLocation(fogShader, "sceneDepth");
    //SetShaderValueTexture(fogShader, sceneDepthLoc, sceneTexture.depth);

    depthShader = LoadShader(0, "assets/shaders/depth_shader.fs");







    // Sky
    skyShader = LoadShader("assets/shaders/skybox.vs", "assets/shaders/skybox.fs");
    skyModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    skyModel.materials[0].shader = skyShader;

    // Shadow ////unused. 
    shadowTex = LoadTexture("assets/shadow_decal.png");
    shadowShader = LoadShader("assets/shaders/shadow_decal.vs", "assets/shaders/shadow_decal.fs");
    shadowQuad = LoadModelFromMesh(GenMeshPlane(1.0f, 1.0f, 1, 1));
    shadowQuad.materials[0].shader = shadowShader;
    SetMaterialTexture(&shadowQuad.materials[0], MATERIAL_MAP_DIFFUSE, shadowTex);

    // Water
    waterShader = LoadShader("assets/shaders/water.vs", "assets/shaders/water.fs");
    SetShaderValue(waterShader, GetShaderLocation(waterShader, "waterLevel"), &waterHeightY, SHADER_UNIFORM_FLOAT);
    waterModel = LoadModelFromMesh(GenMeshPlane(30000, 30000, 1, 1));
    bottomPlane = LoadModelFromMesh(GenMeshPlane(30000, 30000, 1, 1));
    waterModel.materials[0].shader = waterShader;
    bottomPlane.materials[0].shader = waterShader;

    //simple fog //post process second pass 
    simpleFogShader = LoadShader(0, "assets/shaders/simple_fog.fs");

    int locFogColor = GetShaderLocation(simpleFogShader, "fogColor");
    int locFogStrength = GetShaderLocation(simpleFogShader, "fogStrength");
    int locVerticalFade = GetShaderLocation(simpleFogShader, "verticalFade");

    SetShaderValue(simpleFogShader, locFogColor, (float[3]){ 0.1f, 0.1f, 0.1f }, SHADER_UNIFORM_VEC3);
    SetShaderValue(simpleFogShader, locFogStrength, (float[]){0.01f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(simpleFogShader, locVerticalFade, (float[]){ 0.0f }, SHADER_UNIFORM_FLOAT);

    bloomShader = LoadShader(0, "assets/shaders/bloom.fs");
    float bloomStrengthValue = 0.3f;
    //vignetteStrengthValue = 0.2f; //set globaly for black vignette strength
    
    float bloomColor[3] = { 1.0f, 0.1f, 0.9f }; // Slight reddish-pink glow
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "resolution"), &screenResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "bloomStrength"), &bloomStrengthValue, SHADER_UNIFORM_FLOAT);


    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "vignetteStrength"), &vignetteStrengthValue, SHADER_UNIFORM_FLOAT);
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "bloomColor"), bloomColor, SHADER_UNIFORM_VEC3);




    //Sounds
    SoundManager::GetInstance().LoadSound("dinoHit", "assets/sounds/dinoHit.ogg");
    SoundManager::GetInstance().LoadSound("dinoDeath", "assets/sounds/dinoDeath.ogg");
    SoundManager::GetInstance().LoadSound("dinoTweet", "assets/sounds/dino1.ogg");
    SoundManager::GetInstance().LoadSound("dinoTarget", "assets/sounds/dino2.ogg");
    SoundManager::GetInstance().LoadSound("dinoTweet2", "assets/sounds/dino3.ogg");
    SoundManager::GetInstance().LoadSound("dinoBite", "assets/sounds/bite.ogg");
    SoundManager::GetInstance().LoadSound("reload", "assets/sounds/reload.ogg");
    SoundManager::GetInstance().LoadSound("shotgun", "assets/sounds/shotgun.ogg");

    SoundManager::GetInstance().LoadSound("step1", "assets/sounds/step1.ogg");
    SoundManager::GetInstance().LoadSound("step2", "assets/sounds/step2.ogg");
    SoundManager::GetInstance().LoadSound("step3", "assets/sounds/step3.ogg");
    SoundManager::GetInstance().LoadSound("step4", "assets/sounds/step4.ogg");

    SoundManager::GetInstance().LoadSound("phit1", "assets/sounds/PlayerHit1.ogg");
    SoundManager::GetInstance().LoadSound("phit2", "assets/sounds/PlayerHit2.ogg");

    SoundManager::GetInstance().LoadSound("doorOpen", "assets/sounds/doorOpen.ogg");
    SoundManager::GetInstance().LoadSound("doorClose", "assets/sounds/doorCLose.ogg");
    SoundManager::GetInstance().LoadSound("swipe1", "assets/sounds/swipe1.ogg");
    SoundManager::GetInstance().LoadSound("swipe2", "assets/sounds/swipe2.ogg");
    SoundManager::GetInstance().LoadSound("swipe3", "assets/sounds/swipe3.ogg");
    SoundManager::GetInstance().LoadSound("swordHit", "assets/sounds/swordHit.ogg");
    SoundManager::GetInstance().LoadSound("swordBlock", "assets/sounds/swordBlock.ogg");
    SoundManager::GetInstance().LoadSound("swordBlock2", "assets/sounds/swordBlock2.ogg");
    SoundManager::GetInstance().LoadSound("bones", "assets/sounds/bones.ogg");
    SoundManager::GetInstance().LoadSound("bones2", "assets/sounds/bones2.ogg");
    SoundManager::GetInstance().LoadSound("gulp", "assets/sounds/gulp.ogg");
    SoundManager::GetInstance().LoadSound("clink", "assets/sounds/clink.ogg");
    SoundManager::GetInstance().LoadSound("lockedDoor", "assets/sounds/lockedDoor.ogg");
    SoundManager::GetInstance().LoadSound("unlock", "assets/sounds/unlock.ogg");
    SoundManager::GetInstance().LoadSound("key", "assets/sounds/KeyGet.ogg");
    SoundManager::GetInstance().LoadSound("barrelBreak", "assets/sounds/barrelBreak.ogg");
    SoundManager::GetInstance().LoadSound("musket", "assets/sounds/musket.ogg");
    SoundManager::GetInstance().LoadSound("chestOpen", "assets/sounds/chestOpen.ogg");




}

void UpdateShaders(Camera& camera){
    float t = GetTime();
    Vector3 camPos = camera.position;
    SetShaderValue(waterShader, GetShaderLocation(waterShader, "time"), &t, SHADER_UNIFORM_FLOAT);

    int camLoc = GetShaderLocation(waterShader, "cameraPos");
    SetShaderValue(waterShader, camLoc, &camPos, SHADER_UNIFORM_VEC3);

 
    int camPosLoc = GetShaderLocation(terrainShader, "cameraPos");
    SetShaderValue(terrainShader, camPosLoc, &camPos, SHADER_UNIFORM_VEC3);
    //float time = GetTime();
    SetShaderValue(skyShader, GetShaderLocation(skyShader, "time"), &t, SHADER_UNIFORM_FLOAT);

    int isDungeonLoc = GetShaderLocation(skyShader, "isDungeon");
    int dungeonFlag = isDungeon ? 1 : 0;
    SetShaderValue(skyShader, isDungeonLoc, &dungeonFlag, SHADER_UNIFORM_INT);

    //red vignette intensity over time
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "vignetteIntensity"), &vignetteIntensity, SHADER_UNIFORM_FLOAT);

    //dungeonDarkness
    float dungeonDarkness = 0.0f;//darkened 5 percent. it darkens the gun model as well, so go easy. negative number 
    float dungeonContrast = 1.35f; //makes darks darker. 

    float aaStrengthValue = 0.5f; //maybe this does something? 

    int isDungeonVal = isDungeon ? 1 : 0;
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "resolution"), &screenResolution, SHADER_UNIFORM_VEC2);

    SetShaderValue(fogShader, GetShaderLocation(fogShader, "isDungeon"), &isDungeonVal, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "dungeonDarkness"), &dungeonDarkness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader, GetShaderLocation(fogShader, "dungeonContrast"), &dungeonContrast, SHADER_UNIFORM_FLOAT);

    //check every frame
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "vignetteStrength"), &vignetteStrengthValue, SHADER_UNIFORM_FLOAT);

    
    SetShaderValue(bloomShader, GetShaderLocation(bloomShader, "aaStrength"), &aaStrengthValue, SHADER_UNIFORM_FLOAT);

    int sceneTextureLoc = GetShaderLocation(depthShader, "sceneTexture");
    int sceneDepthLoc   = GetShaderLocation(depthShader, "sceneDepth");

    int cameraNearLoc = GetShaderLocation(depthShader, "cameraNear");
    int cameraFarLoc  = GetShaderLocation(depthShader, "cameraFar");

    int fogNearLoc = GetShaderLocation(depthShader, "fogNear");
    int fogFarLoc  = GetShaderLocation(depthShader, "fogFar");
    int fogAmountLoc = GetShaderLocation(depthShader, "fogAmount");

    // Camera near/far match render pass
    float cameraNearClip = 60.0f;
    float cameraFarClip = 10000.0f;

    float fogAmount = 1.0f; // 0.0 = no fog, 1.0 = full fog
    SetShaderValue(depthShader, fogAmountLoc, &fogAmount, SHADER_UNIFORM_FLOAT);

    SetShaderValue(depthShader, cameraNearLoc, &cameraNearClip, SHADER_UNIFORM_FLOAT);
    SetShaderValue(depthShader, cameraFarLoc, &cameraFarClip, SHADER_UNIFORM_FLOAT);

    // Fog control
    float fogNear = 60.0f;
    float fogFar  = 61.0f;

    SetShaderValue(depthShader, fogNearLoc, &fogNear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(depthShader, fogFarLoc, &fogFar, SHADER_UNIFORM_FLOAT);

 


}

void UnloadAllResources() {
    //textures
    UnloadRenderTexture(sceneTexture);
    UnloadTexture(bushTex);
    UnloadTexture(shadowTex);
    UnloadTexture(raptorTexture);
    UnloadTexture(gunTexture);
    UnloadTexture(smokeSheet);
    UnloadTexture(bloodSheet);
    UnloadTexture(skeletonSheet);
    UnloadTexture(doorTexture);
    UnloadTexture(healthPotTexture);
    UnloadTexture(keyTexture);
    UnloadTexture(pirateSheet);
    UnloadTexture(coinTexture);
    //shaders
    UnloadShader(fogShader);
    UnloadShader(skyShader);
    UnloadShader(waterShader);
    UnloadShader(terrainShader);
    UnloadShader(shadowShader);
    UnloadShader(bloomShader);
    //models
    UnloadModel(skyModel);
    UnloadModel(waterModel);
    UnloadModel(shadowQuad);
    UnloadModel(palmTree);
    UnloadModel(palm2);
    UnloadModel(bush);
    UnloadModel(boatModel);
    UnloadModel(wall);
    UnloadModel(floorTile);
    UnloadModel(doorWay);
    UnloadModel(barrelModel);
    UnloadModel(pillarModel);
    UnloadModel(swordModel);
    UnloadModel(chestModel);
    UnloadModel(lampModel);
    

}
