// render_pipeline.cpp
#include "render_pipeline.h"
#include "resourceManager.h"
#include "ui.h"
#include "world.h"
#include "transparentDraw.h"
#include "boat.h"
#include "rlgl.h"
#include "dungeonGeneration.h"
#include "player.h"
#include "input.h"
#include "camera_system.h"
#include "lighting.h"
#include "shadows.h"
#include "terrainChunking.h"
#include "main_menu.h"


static int lastW = 0;
static int lastH = 0;

static void EnsureRenderTargetsMatchWindow(RenderTexture2D& rt)
{
    //resize RenderTextures to match new fullscreen size
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    int curW = rt.texture.width;
    int curH = rt.texture.height;

    if (rt.id != 0 && curW == w && curH == h) return;

    if (rt.id != 0) UnloadRenderTexture(rt);
    rt = LoadRenderTexture(w, h);

}




void RenderMenuFrame(Camera3D& camera, Player& player, float dt) {
    //Render level background for menu
    RenderTexture2D& sceneTexture = R.GetRenderTexture("sceneTexture");
    RenderTexture2D& postTexture = R.GetRenderTexture("postProcessTexture");

    EnsureRenderTargetsMatchWindow(sceneTexture);
    EnsureRenderTargetsMatchWindow(postTexture);

    // --- 3D scene to sceneTexture ---
    BeginTextureMode(R.GetRenderTexture("sceneTexture")); //MENU FRAME
        ClearBackground(SKYBLUE);
        float farClip = isDungeon ? 50000.0f : 100000.0f;
        float nearclip = 30.0f;
        CameraSystem::Get().BeginCustom3D(camera, nearclip, farClip);

        //skybox
        rlDisableBackfaceCulling(); rlDisableDepthMask(); rlDisableDepthTest();
        DrawModel(R.GetModel("skyModel"), camera.position, 10000.0f, WHITE);
        rlEnableDepthMask(); rlEnableDepthTest();
        BeginBlendMode(BLEND_ALPHA);

        if (!isDungeon){
            float maxDrawDist = 50000.0f; //Higher for menu cam
            DrawTerrainGrid(terrain, camera, maxDrawDist); //draw the chunks
            DrawBoat(player_boat);
            HandleWaves(camera); //update water plane bob. 

        }

        BeginShaderMode(R.GetShader("cutoutShader"));
        DrawTrees(trees, camera); 
        DrawBushes(bushes); //alpha cuttout bushes as well as tree leaf
        DrawOverworldProps();
        EndShaderMode();
        DrawDungeonGeometry(camera, 20000);

        DrawDungeonPillars();
        DrawDungeonBarrels();
        DrawLaunchers();
        DrawTransparentDrawRequests(camera);

        EndBlendMode();
        EndMode3D();
        rlDisableDepthTest();
    EndTextureMode();

    // --- post to postProcessTexture ---
    BeginTextureMode(R.GetRenderTexture("postProcessTexture"));
    {
        BeginShaderMode(R.GetShader("fogShader"));
            auto& sceneRT = R.GetRenderTexture("sceneTexture");
            Rectangle src = { 0, 0,
                            (float)sceneRT.texture.width,
                            -(float)sceneRT.texture.height }; // flip Y!
            Rectangle dst = { 0, 0,
                            (float)GetScreenWidth(),
                            (float)GetScreenHeight() };
            DrawTexturePro(sceneRT.texture, src, dst, {0,0}, 0.0f, WHITE);
        EndShaderMode();
    }
    EndTextureMode();



    // --- final to backbuffer + UI ---
    BeginDrawing();
        ClearBackground(WHITE);
        BeginShaderMode(R.GetShader("bloomShader"));
            auto& post = R.GetRenderTexture("postProcessTexture");
            Rectangle src = { 0, 0,
                            (float)post.texture.width,
                            -(float)post.texture.height }; // flip Y!
            Rectangle dst = { 0, 0,
                            (float)GetScreenWidth(),
                            (float)GetScreenHeight() };
            DrawTexturePro(post.texture, src, dst, {0,0}, 0.0f, WHITE);
        EndShaderMode();

        if (gFadePhase != FadePhase::FadingOut) MainMenu::Draw(gMenu, levelIndex, levels.data(), (int)levels.size());
        
        
    EndDrawing();
    }




void RenderFrame(Camera3D& camera, Player& player, float dt) {
    //Main Render frame
    RenderTexture2D& sceneTexture = R.GetRenderTexture("sceneTexture");
    RenderTexture2D& postTexture = R.GetRenderTexture("postProcessTexture");

    EnsureRenderTargetsMatchWindow(sceneTexture);
    EnsureRenderTargetsMatchWindow(postTexture);
    // --- 3D scene to sceneTexture ---
    BeginTextureMode(R.GetRenderTexture("sceneTexture"));
        ClearBackground(SKYBLUE);
        float farClip = isDungeon ? 50000.0f : 100000.0f;
        float nearclip = 30.0f;
        CameraSystem::Get().BeginCustom3D(camera, nearclip, farClip);

        //skybox
        rlDisableBackfaceCulling(); rlDisableDepthMask(); rlDisableDepthTest();
        DrawModel(R.GetModel("skyModel"), camera.position, 10000.0f, WHITE);
        rlEnableDepthMask(); rlEnableDepthTest();
        BeginBlendMode(BLEND_ALPHA);
        

        if (!isDungeon) {

            float maxDrawDist = 15000.0f; //lowest it can be before terrain popping in is noticable. 

            DrawTerrainGrid(terrain, camera, maxDrawDist); //draw the chunks

            HandleWaves(camera); //update water plane bob. 
 
            rlEnableDepthTest();
            rlDisableDepthMask();         // don’t write depth for transparent water
            if (!debugInfo) DrawModel(R.GetModel("waterModel"), {0,0,0}, 1.0f, WHITE); // Don't draw water model, shader colors the water
            rlEnableDepthMask();
            
            DrawBoat(player_boat);
            BeginShaderMode(R.GetShader("cutoutShader"));
            DrawTrees(trees, camera); 
            DrawBushes(bushes); //alpha cuttout bushes as well as tree leaf
            EndShaderMode();
            //DrawDungeonDoorways();          
            DrawDungeonGeometry(camera, 10000);
            DrawOverworldProps();
        } else {
            //draw the dungeon
            DrawDungeonGeometry(camera, 8000);
            //DebugDrawGrappleBox();
            DrawDungeonBarrels();
            DrawLaunchers();
            DrawDungeonChests();
            DrawDungeonPillars();
            
            // for (WallRun& b : wallRunColliders){ //debug draw wall colliders
            //     DrawBoundingBox(b.bounds, WHITE);
            // }

        }

        DrawPlayer(player, camera);
        //DrawWeapons(player, camera);  

        DrawEnemyShadows();
        DrawBullets(camera);
        DrawCollectableWeapons(player, dt);

        // transparency last

        DrawTransparentDrawRequests(camera);
        rlDisableDepthMask();
        DrawBloodParticles(camera);
        rlEnableDepthMask();


        EndBlendMode();
        EndMode3D();

        rlDisableDepthTest();
    EndTextureMode();

    // --- post to postProcessTexture ---
    BeginTextureMode(R.GetRenderTexture("postProcessTexture"));
    {
        BeginShaderMode(R.GetShader("fogShader"));
            auto& sceneRT = R.GetRenderTexture("sceneTexture");
            Rectangle src = { 0, 0,
                            (float)sceneRT.texture.width,
                            -(float)sceneRT.texture.height }; // flip Y!
            Rectangle dst = { 0, 0,
                            (float)GetScreenWidth(),
                            (float)GetScreenHeight() };
            DrawTexturePro(sceneRT.texture, src, dst, {0,0}, 0.0f, WHITE);
        EndShaderMode();
    }
    EndTextureMode();

    // --- final to backbuffer + UI ---
    BeginDrawing();
        ClearBackground(WHITE);
        BeginShaderMode(R.GetShader("bloomShader"));
            auto& postRT = R.GetRenderTexture("postProcessTexture");
            Rectangle src = { 0, 0,
                            (float)postRT.texture.width,
                            -(float)postRT.texture.height }; // flip Y!
            Rectangle dst = { 0, 0,
                            (float)GetScreenWidth(),
                            (float)GetScreenHeight() };


            DrawTexturePro(postRT.texture, src, dst, {0,0}, 0.0f, WHITE);


            rlDisableDepthTest();
            BeginMode3D(camera);
                if (!player.dying) DrawWeapons(player, camera);
            EndMode3D();
            //rlEnableDepthTest(); //Leave depth test off. If left on it messes with minimap

        EndShaderMode();

        
        if (pendingLevelIndex != -1) {
            DrawText("Loading...", GetScreenWidth()/2 - MeasureText("Loading...", 20)/2,
                     GetScreenHeight()/2, 20, WHITE);
        } else {
            //health mana stam bars UI
            if (controlPlayer) DrawUI();
            
            //draw mini map
            if (isDungeon) miniMap.DrawMiniMap();

            if (debugInfo) { //Press ~ for debug mode. 
                DrawTimer(ElapsedTime);
                DrawText("PRESS TAB FOR FREE CAMERA", GetScreenWidth()/2, 15, 20, WHITE);
                //show FPS over top of lightmap
                DrawText(TextFormat("%d FPS", GetFPS()), 350, 10, 20, WHITE);

            }
            
        } 
    EndDrawing();
}
