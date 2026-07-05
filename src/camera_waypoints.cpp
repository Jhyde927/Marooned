#include "camera_system.h"
#include "raymath.h"
#include "world.h"
#include "shaderSetup.h"

namespace Cutscenes {

    void StartRiverIntro(){
        CutsceneDesc intro;

        Vector3 playerForward = GetForwardFromYaw(player.startRotationY);

        Vector3 playerViewTarget = Vector3Add(
            player.position,
            Vector3Scale(playerForward, 10000.0f)
        );

        intro.startPos = { -10845.8, 2000.0, 2969.99 };
        intro.endPos = player.position;
        intro.endTarget = playerViewTarget;


        // This is what the camera looks at for most of the cutscene.
        intro.target   = player.position;//{ 0.0f, 200.0f, 0.0f };
        intro.lockTarget = true;

        intro.duration = 25.0f;
        intro.arcHeight = 4000.0f;
        intro.pathType = CutscenePathType::ArcY;
        intro.returnToPlayerOnFinish = true;

        intro.mergeToPlayerViewAtEnd = true;
        intro.mergeStartT = 0.5f;

        CameraSystem::Get().StartCutscene(intro);

    }

    void StartIslandIntro(){
        CutsceneDesc intro;

        Vector3 playerForward = GetForwardFromYaw(player.startRotationY);

        Vector3 playerViewTarget = Vector3Add(
            player.position,
            Vector3Scale(playerForward, 10000.0f)
        );

        intro.startPos = { -10845.8, 2000.0, 2969.99 };
        intro.endPos = player.position;
        intro.endTarget = playerViewTarget;


        // This is what the camera looks at for most of the cutscene.
        intro.target   = { 0.0f, 200.0f, 0.0f };
        intro.lockTarget = true;

        intro.duration = 25.0f;
        intro.arcHeight = 2500.0f;
        intro.pathType = CutscenePathType::ArcY;
        intro.returnToPlayerOnFinish = true;

        intro.mergeToPlayerViewAtEnd = true;
        intro.mergeStartT = 0.7f;

        CameraSystem::Get().StartCutscene(intro);

    }


    void StartIslandWaypointIntro()
    {
        CameraSystem& camSys = CameraSystem::Get();

        Vector3 playerCamPos;
        Vector3 playerCamTarget;
        camSys.GetPlayerCameraPose(playerCamPos, playerCamTarget);

        WaypointCutsceneDesc desc;
        desc.snapOnStart = true;
        desc.returnToPlayerOnFinish = true;
        desc.hideCeiling = false;


        //Work In Progress
        const float camY = 1800.0f;
        //const float lookY = 300.0f;

        //Vector3 startPos    = Vector3{ 5475.0f,   camY, -5665.0f };
        //Vector3 rightIsland = Vector3{-8254.4, camY, -8892.74};
        Vector3 farIsland   = Vector3{-8722.67, camY, 9487.32 };
        //Vector3 leftIsland  = Vector3{ 8281.82, camY, 8645.83};


        //Vector3(-940.195, 653.18, 840.402)
        Vector3 middle = Vector3{ -940.0f, camY, -840.0f};



        Vector3 p0 = farIsland;
        Vector3 p1 = middle;
        //Vector3 p2 = startPos;
        // Vector3 p3 = leftIsland;
        // Vector3 p4 = startPos;

        CameraWaypoint w0;
        w0.position = p0;
        w0.target = Vector3{0, 300, 0};
        w0.durationToNext = 15.0f;
        desc.points.push_back(w0);

        CameraWaypoint w1;
        w1.position = p1;
        w1.target =  Vector3{0, camY, 0};
        w1.durationToNext = 15.0f;
        desc.points.push_back(w1);

        CameraWaypoint w2;
        w2.position = playerCamPos;
        w2.target =  playerCamTarget;
        w2.durationToNext = 0.0f;
        desc.points.push_back(w2);

        // CameraWaypoint w3;
        // w3.position = p3;
        // w3.target =  Vector3{0};
        // w3.durationToNext = 9.0f;
        // desc.points.push_back(w3);

        // Circle back near the start before merging.
        // CameraWaypoint w4;
        // w4.position = p4;
        // w4.target = playerCamTarget;
        // w4.durationToNext = 2.0f;
        // desc.points.push_back(w4);

        // // Final exact player camera merge.
        // CameraWaypoint w5;
        // w5.position = playerCamPos;
        // w5.target = playerCamTarget;
        // w5.durationToNext = 0.0f;
        // desc.points.push_back(w5);

        camSys.StartWaypointCutscene(desc);
    }

    void StartKrakenScene()
    {
        GameSettings::drawMinimap = false;
        CameraSystem& camSys = CameraSystem::Get();

        Vector3 playerCamPos;
        Vector3 playerCamTarget;
        camSys.GetPlayerCameraPose(playerCamPos, playerCamTarget);


        const float camY = 300.0f;     // tweak
        const float lookY = 260.0f;    // tweak

        Vector3 p0 = DungeonTileCenter(27, 29, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p1 = DungeonTileCenter(24,  21, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p2 = DungeonTileCenter(14,  21, dungeonWidth, dungeonHeight, tileSize, camY);

        Vector3 middleDeck = DungeonTileCenter(24, 21, dungeonWidth, dungeonHeight, tileSize, lookY);
        Vector3 krakenPos = gKraken.startPos;//DungeonTileCenter(2, 7, dungeonWidth, dungeonHeight, tileSize, lookY);

        WaypointCutsceneDesc desc;
        desc.snapOnStart = true;
        desc.returnToPlayerOnFinish = true;
        desc.hideCeiling = false;

        CameraWaypoint w0;
        w0.position = p0;
        w0.target = middleDeck;
        w0.durationToNext = 9.0f;
        desc.points.push_back(w0);

        CameraWaypoint w1;
        w1.position = p1;
        w1.target = krakenPos;
        w1.durationToNext = 6.0f;
        desc.points.push_back(w1);

        CameraWaypoint w2;
        w2.position = p2;
        w2.target = krakenPos;
        w2.durationToNext = 3.0f;
        desc.points.push_back(w2);

        //w3

        CameraWaypoint w3;
        w3.position = p1;
        w3.target = krakenPos;
        w3.durationToNext = 3.0f;
        desc.points.push_back(w3);

        CameraWaypoint w4;
        w4.position = playerCamPos;
        w4.target = playerCamTarget;
        w4.durationToNext = 0.0f;
        desc.points.push_back(w4);

        camSys.StartWaypointCutscene(desc);
    }


    void StartSpiderScene(){
        GameSettings::drawMinimap = false;
        CameraSystem& camSys = CameraSystem::Get();
        ShaderSetup::gBloom.letterboxTarget = 0.14f;

        Vector3 giantSpiderPos;
        for (Character* gs : enemyPtrs){
            if (gs->type == CharacterType::GiantSpider){
                giantSpiderPos = gs->position;
                break;
            }
        }

        Vector3 playerCamPos;
        Vector3 playerCamTarget;
        camSys.GetPlayerCameraPose(playerCamPos, playerCamTarget);
        const float camY = 300.0f;  
        //semi circle the spider. 
        Vector3 p0 = DungeonTileCenter(28, 17, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p1 = DungeonTileCenter(27,  23, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p2 = DungeonTileCenter(21,  23, dungeonWidth, dungeonHeight, tileSize, camY);

        Vector3 p3 = DungeonTileCenter(14,  24, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p4 = DungeonTileCenter(15,  29, dungeonWidth, dungeonHeight, tileSize, camY);

        WaypointCutsceneDesc desc;
        desc.snapOnStart = true;
        desc.returnToPlayerOnFinish = true;
        desc.hideCeiling = false;

        CameraWaypoint w0;
        w0.position = p0;
        w0.target = p1;
        w0.durationToNext = 9.0f;
        desc.points.push_back(w0);

        CameraWaypoint w1;
        w1.position = p1;
        w1.target = p2;
        w1.durationToNext = 6.0f;
        desc.points.push_back(w1);

        CameraWaypoint w2;
        w2.position = p2;
        w2.target = giantSpiderPos;
        w2.durationToNext = 6.0f;
        desc.points.push_back(w2);

        CameraWaypoint w3;
        w3.position = p3;
        w3.target = giantSpiderPos;
        w3.durationToNext = 6.0f;
        desc.points.push_back(w3);


        CameraWaypoint w4;
        w4.position = p4;
        w4.target = giantSpiderPos;
        w4.durationToNext = 0.0f;
        desc.points.push_back(w4);

        camSys.StartWaypointCutscene(desc);


    }

    void StartDungeonHallwayIntro()
    {
        GameSettings::drawMinimap = false;
        CameraSystem& camSys = CameraSystem::Get();

        Vector3 playerCamPos;
        Vector3 playerCamTarget;
        camSys.GetPlayerCameraPose(playerCamPos, playerCamTarget);


        const float camY = 300.0f;     // tweak
        const float lookY = 260.0f;    // tweak

        Vector3 p0 = DungeonTileCenter(30, 1, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p1 = DungeonTileCenter(8,  1, dungeonWidth, dungeonHeight, tileSize, camY);
        Vector3 p2 = DungeonTileCenter(2,  7, dungeonWidth, dungeonHeight, tileSize, camY);

        Vector3 lookDownHall = DungeonTileCenter(8, 1, dungeonWidth, dungeonHeight, tileSize, lookY);
        Vector3 lookLeftRoom = DungeonTileCenter(2, 7, dungeonWidth, dungeonHeight, tileSize, lookY);

        WaypointCutsceneDesc desc;
        desc.snapOnStart = true;
        desc.returnToPlayerOnFinish = true;
        desc.hideCeiling = false;

        CameraWaypoint w0;
        w0.position = p0;
        w0.target = lookDownHall;
        w0.durationToNext = 9.0f;
        desc.points.push_back(w0);

        CameraWaypoint w1;
        w1.position = p1;
        w1.target = lookLeftRoom;
        w1.durationToNext = 6.0f;
        desc.points.push_back(w1);

        CameraWaypoint w2;
        w2.position = p2;
        w2.target = playerCamTarget;
        w2.durationToNext = 3.0f;
        desc.points.push_back(w2);

        CameraWaypoint w3;
        w3.position = playerCamPos;
        w3.target = playerCamTarget;
        w3.durationToNext = 0.0f;
        desc.points.push_back(w3);

        camSys.StartWaypointCutscene(desc);
    }

}


static float Smooth01(float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    return t * t * (3.0f - 2.0f * t);
}

void CameraSystem::SwitchToPlayerCamera()
{
    mode = CamMode::Player;

    Vector3 pos;
    Vector3 target;
    GetPlayerCameraPose(pos, target); // should calculate from player pos/yaw/pitch NOW

    playerRig.cam.position = pos;
    playerRig.cam.target = target;

    // Important if UpdatePlayerCam uses smoothed state
    playerRig.smoothedPos = pos;
    // playerCamSmoothedPos = pos;
    // playerCamSmoothedTarget = target;
}

void CameraSystem::StartWaypointCutscene(const WaypointCutsceneDesc& desc)
{
    if (desc.points.size() < 2) {
        return;
    }

    waypointCutscene = desc;
    waypointIndex = 0;
    waypointT = 0.0f;
    waypointActive = true;

    cineKind = CinematicKind::Waypoints;
    cineActive = false;
    cutsceneActive = false;
    cutsceneT = 0.0f;

    mode = CamMode::Cinematic;

    // if (waypointCutscene.hideCeiling) {
    //     drawCeiling = false;
    // }

    cinematicRig.fov = (playerRig.fov > 0.0f) ? playerRig.fov : GameSettings::fovY;
    cinematicRig.cam.fovy = cinematicRig.fov;
    cinematicRig.cam.up = Vector3{0, 1, 0};
    cinematicRig.cam.projection = CAMERA_PERSPECTIVE;

    if (waypointCutscene.snapOnStart) {
        cinematicRig.cam.position = waypointCutscene.points[0].position;
        cinematicRig.cam.target = waypointCutscene.points[0].target;
    }
}

void CameraSystem::UpdateWaypointCutsceneCam(float dt)
{
    if (!waypointActive) {
        return;
    }
    player.canMove = false;

    if (waypointCutscene.points.size() < 2) {
        waypointActive = false;
        SetMode(CamMode::Player);

 
        return;
    }

    int nextIndex = waypointIndex + 1;

    if (nextIndex >= (int)waypointCutscene.points.size()) {
        waypointActive = false;
        drawCeiling = levels[gCurrentLevelIndex].hasCeiling;
        ShaderSetup::gBloom.letterboxTarget = 0.0f;
        GameSettings::drawMinimap = true; //turn minimap back on after waypoint cutscene.
        player.canMove = true;
        if (CurrentLevelIs("Ship")) SpawnManager::cutSceneFinished = true; //kraken cut scene finished. 

        if (waypointCutscene.returnToPlayerOnFinish) {
            //SnapAllToPlayer();
            SwitchToPlayerCamera();
            
            //SetMode(CamMode::Player);
        }

        return;
    }

    const CameraWaypoint& a = waypointCutscene.points[waypointIndex];
    const CameraWaypoint& b = waypointCutscene.points[nextIndex];

    float duration = a.durationToNext;
    if (duration <= 0.0f) {
        duration = 0.001f;
    }

    waypointT += dt / duration;

    float t = waypointT;
    if (t > 1.0f) {
        t = 1.0f;
    }

    float e = Smooth01(t);

    // Vector3 camPos = Vector3Lerp(a.position, b.position, e);

    // Vector3 travelDir = Vector3Subtract(b.position, a.position);
    // if (Vector3LengthSqr(travelDir) > 0.01f) {
    //     travelDir = Vector3Normalize(travelDir);
    // }

    // Vector3 lookTarget = Vector3Add(b.position, Vector3Scale(travelDir, 500.0f));
    // lookTarget.y = b.target.y;

    // cinematicRig.cam.position = camPos;
    // cinematicRig.cam.target = lookTarget;

    cinematicRig.cam.position = Vector3Lerp(a.position, b.position, e);
    cinematicRig.cam.target = Vector3Lerp(a.target, b.target, e);
    cinematicRig.cam.up = Vector3{0, 1, 0};
    cinematicRig.cam.fovy = cinematicRig.fov;
    cinematicRig.cam.projection = CAMERA_PERSPECTIVE;

    if (waypointT >= 1.0f) {
        waypointIndex++;
        waypointT = 0.0f;
    }
}

