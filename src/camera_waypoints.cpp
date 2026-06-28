#include "camera_system.h"
#include "raymath.h"
#include "world.h"
#include "shaderSetup.h"



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
        
        if (CurrentLevelIs("Ship")) SpawnManager::cutSceneFinished = true; //kraken cut scene finished. 

        if (waypointCutscene.returnToPlayerOnFinish) {
            SnapAllToPlayer();
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