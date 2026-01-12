#include "camera_system.h"
#include "raymath.h"
#include "input.h" 
#include "world.h"
#include "rlgl.h"
#include "utilities.h"

DeathCamState deathCam;

CameraSystem& CameraSystem::Get() {
    static CameraSystem instance;
    return instance;
}

void CameraSystem::SyncFromPlayer(const PlayerView& view) {
    pv = view;
}

void CameraSystem::Init(const Vector3& startPos) {
    playerRig.cam.position = startPos;
    playerRig.cam.target   = Vector3Add(startPos, {0,0,1});
    playerRig.cam.up       = {0,1,0};
    playerRig.cam.fovy     = playerRig.fov;

    playerRig.cam.projection = CAMERA_PERSPECTIVE;

    freeRig = playerRig; // start free cam matching player
}

static inline float YawFromDir(const Vector3& d) {
    return RAD2DEG * atan2f(d.x, d.z); // x vs z 
}
static inline float PitchFromDir(const Vector3& d) {
    // safer than asinf(d.y) when d isn't perfectly normalized
    return RAD2DEG * atan2f(d.y, sqrtf(d.x*d.x + d.z*d.z));
}

void CameraSystem::SnapAllToPlayer() {
    // 1) position/target from the *player camera*, not player entity
    playerRig.cam.position = playerRig.cam.position; 
    Vector3 pos = playerRig.cam.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(playerRig.cam.target, pos));

    // 2) copy camera state
    freeRig.cam.position = pos;
    freeRig.cam.target   = Vector3Add(pos, dir);
    freeRig.fov          = playerRig.fov;

    cinematicRig.cam.position = pos;
    

    // 3) copy angles — either from cached playerRig...
    freeRig.yaw   = playerRig.yaw;
    freeRig.pitch = playerRig.pitch;

}

void CameraSystem::AttachToPlayer(const Vector3& pos, const Vector3& forward) {
    // Simple follow; you can add head-bob/weapon sway offsets here
    playerRig.cam.position = pos;
    playerRig.cam.target   = Vector3Add(pos, forward);
}

void CameraSystem::SetMode(CamMode m) { mode = m; }
CamMode CameraSystem::GetMode() const { return mode; }

void CameraSystem::SetFOV(float fov) {
    playerRig.fov = fov; 
    freeRig.fov   = fov;
}

void CameraSystem::SetFarClip(float farClip) {
    playerRig.farClip = farClip;
    freeRig.farClip   = farClip;
}

Camera3D& CameraSystem::Active() {
    if (mode == CamMode::Player)      { playerRig.cam.fovy = playerRig.fov; return playerRig.cam; }
    else if (mode == CamMode::Free)   { freeRig.cam.fovy   = freeRig.fov;   return freeRig.cam;  }
    else                              { cinematicRig.cam.fovy = cinematicRig.fov; return cinematicRig.cam; }
}

const Camera3D& CameraSystem::Active() const {
    if (mode == CamMode::Player) return playerRig.cam;
    if (mode == CamMode::Free)   return freeRig.cam;
    return cinematicRig.cam;
}

static Vector3 RotateY(Vector3 v, float rad)
{
    float s = sinf(rad), c = cosf(rad);
    return { v.x*c + v.z*s, v.y, -v.x*s + v.z*c };
}



void CameraSystem::Shake(float mag, float dur) { shakeMag = mag; shakeTime = dur; }

void CameraSystem::ApplyShake(float dt) {
    if (shakeTime <= 0.f) return;
    shakeTime -= dt;
    float t = shakeTime;
    Vector3 jitter = { (GetRandomValue(-100,100)/100.f)*shakeMag,
                       (GetRandomValue(-100,100)/100.f)*shakeMag,
                       (GetRandomValue(-100,100)/100.f)*shakeMag };
    if (mode == CamMode::Player) playerRig.cam.position = Vector3Add(playerRig.cam.position, jitter);
    else                         freeRig.cam.position   = Vector3Add(freeRig.cam.position, jitter);
}

// Exponential smoothing helper
inline float SmoothStepExp(float current, float target, float speed, float dt) {
    float a = 1.0f - expf(-speed * dt);
    return current + (target - current) * a;
}



void CameraSystem::UpdatePlayerCam(float dt)
{
    Vector3 basePos = pv.onBoard
        ? Vector3Add(pv.boatPos, Vector3{0, 200.0f, 0})
        : pv.position;

    if (player.isSwimming && !pv.onBoard) basePos.y -= 40.0f;

    float yawRad   = DEG2RAD * pv.yawDeg;
    float pitchRad = DEG2RAD * pv.pitchDeg;

    Vector3 forward = {
        cosf(pitchRad) * sinf(yawRad),
        sinf(pitchRad),
        cosf(pitchRad) * cosf(yawRad)
    };

    // --- Smooth camera position (NOT forward/target direction) ---
    if (!playerRig.hasSmoothedInit)
    {
        playerRig.smoothedPos = basePos;      // snap on first frame / after teleport
        playerRig.hasSmoothedInit = true;
    }

    // Tune: higher = tighter camera, lower = smoother float
    const float followSpeedXZ = 22.0f;
    const float followSpeedY  = 10.0f;

    float aXZ = 1.0f - expf(-followSpeedXZ * dt);
    float aY  = 1.0f - expf(-followSpeedY  * dt);

    playerRig.smoothedPos.x = Lerp(playerRig.smoothedPos.x, basePos.x, aXZ);
    playerRig.smoothedPos.z = Lerp(playerRig.smoothedPos.z, basePos.z, aXZ);
    playerRig.smoothedPos.y = Lerp(playerRig.smoothedPos.y, basePos.y, aY);

    // Use smoothed position, but immediate forward direction
    playerRig.cam.position = playerRig.smoothedPos;
    playerRig.cam.target   = Vector3Add(playerRig.smoothedPos, forward);

    // keep the angles cached so other rigs can copy
    playerRig.yaw   = pv.yawDeg;
    playerRig.pitch = pv.pitchDeg;

    if (debugInfo){
        SetFarClip(isDungeon ? 100000.0f : 100000.0f);
    }else{
        SetFarClip(isDungeon ? 50000.0f : 100000.0f);
    }
}

void CameraSystem::UpdateFreeCam(float dt) {
    const float speed = (IsKeyDown(KEY_LEFT_SHIFT) ? 1500.f : 900.f);
    Vector3 f = Vector3Normalize(Vector3Subtract(freeRig.cam.target, freeRig.cam.position));
    Vector3 r = Vector3Normalize(Vector3CrossProduct(f, {0,1,0}));

    Vector3 move{0,0,0};
    if (IsKeyDown(KEY_W)) move = Vector3Add(move, f);
    if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, f);
    if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, r);
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, r);

    // vertical controls
    if (IsKeyDown(KEY_SPACE)) move = Vector3Add(move, {0, 1, 0});
    if (IsKeyDown(KEY_LEFT_CONTROL)) move = Vector3Add(move, {0, -1, 0}); 

    if (Vector3Length(move) > 0) {
        move = Vector3Scale(Vector3Normalize(move), speed * dt);
        freeRig.cam.position = Vector3Add(freeRig.cam.position, move);
        freeRig.cam.target   = Vector3Add(freeRig.cam.target, move);
    }

    // Mouse look
    Vector2 delta = GetMouseDelta();
    float sens = 0.05f; //matches player's 
    freeRig.yaw   -= delta.x * sens;
    freeRig.pitch += -delta.y * sens;
    freeRig.pitch = Clamp(freeRig.pitch, -89.f, 89.f);

    Vector3 dir{
        cosf(DEG2RAD*freeRig.pitch) * sinf(DEG2RAD*freeRig.yaw),
        sinf(DEG2RAD*freeRig.pitch),
        cosf(DEG2RAD*freeRig.pitch) * cosf(DEG2RAD*freeRig.yaw)
    };
    freeRig.cam.target = Vector3Add(freeRig.cam.position, dir);
    freeRig.cam.fovy   = freeRig.fov;

    if (debugInfo){
        SetFarClip(isDungeon ? 100000.0f : 100000.0f);
    }else{
        SetFarClip(isDungeon ? 50000.0f : 100000.0f);
    }
}


void CameraSystem::BeginCustom3D(const Camera3D& cam, float nearClip, float farClip) {
    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    Matrix proj = MatrixPerspective(DEG2RAD * cam.fovy,
                                (float)GetScreenWidth()/(float)GetScreenHeight(),
                                nearClip, farClip);
    rlMultMatrixf(MatrixToFloat(proj));

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    Matrix view = MatrixLookAt(cam.position, cam.target, cam.up);
    rlMultMatrixf(MatrixToFloat(view));
}

static inline float SmoothExp(float current, float target, float speed, float dt) {
    float a = 1.0f - expf(-speed * dt);
    return current + (target - current) * a;
}

static inline Vector3 SmoothExpV3(Vector3 current, Vector3 target, float speed, float dt) {
    return {
        SmoothExp(current.x, target.x, speed, dt),
        SmoothExp(current.y, target.y, speed, dt),
        SmoothExp(current.z, target.z, speed, dt),
    };
}

void CameraSystem::StartDeathCam(float dungeonFloorY, float terrainFloorY)
{
    Camera& cam = playerRig.cam;
    
    deathCam.startPos    = cam.position;
    deathCam.startTarget = cam.target;

    Vector3 forward = Vector3Subtract(cam.target, cam.position);
    float len2 = forward.x*forward.x + forward.y*forward.y + forward.z*forward.z;

    if (len2 < 0.000001f)
        forward = { 0, 0, 1 };
    else
        forward = Vector3Scale(forward, 1.0f / sqrtf(len2));

    deathCam.startForward = forward;

    float floorY = isDungeon ? dungeonFloorY : terrainFloorY;
    const float eyeOffset = 50.0f;

    deathCam.endPos = {
        cam.position.x,
        floorY + eyeOffset,
        cam.position.z
    };

    deathCam.endPos = Vector3Add(
        deathCam.endPos,
        Vector3Scale(forward, 10.0f)
    );

    // Reset timer
    deathCam.t = 0.0f;
    deathCam.duration = 2.0f;

    mode = CamMode::Death;
}


// void CameraSystem::StartDeathCam(float dungeonFloorY, float terrainFloorY)
// {

//     Camera& cam = playerRig.cam;
    
//     deathCam.startPos    = cam.position;
//     deathCam.startTarget = cam.target;

//     Vector3 forward = Vector3Subtract(cam.target, cam.position);
//     float len2 = forward.x*forward.x + forward.y*forward.y + forward.z*forward.z;

//     if (len2 < 0.000001f) {
//         // Fallback forward if camera target/pos are degenerate
//         forward = { 0, 0, 1 };
//     } else {
//         forward = Vector3Scale(forward, 1.0f / sqrtf(len2));
//     }

//     Vector3 forwardTurned = RotateY(forward, DEG2RAD * deathCam.endYawDeg);


//     // --- decide floor Y ---
//     float floorY = isDungeon ? dungeonFloorY : terrainFloorY;

//     // Small offset so camera doesn't clip into floor
//     const float eyeOffset = 75.0f;

//     // --- end position ---
//     deathCam.endPos = {
//         cam.position.x,
//         floorY + eyeOffset,
//         cam.position.z
//     };

//     // Small forward slide feels more natural than straight drop
//     deathCam.endPos = Vector3Add(
//         deathCam.endPos,
//         Vector3Scale(forward, 10.0f)
//     );

//     // --- end target (look slightly down) ---
//     deathCam.endTarget = Vector3Add(
//         deathCam.endPos,
//         Vector3Scale(forwardTurned, 100.0f)
//     );
//     deathCam.endTarget.y -= 20.0f;

//     // Reset timer
//     deathCam.t = 0.0f;
//     deathCam.duration = 2.0f;

//     mode = CamMode::Death;
// }

void CameraSystem::UpdateDeathCam(float dt)
{
    deathCam.t += dt;
    float t = deathCam.t / deathCam.duration;
    if (t > 1.0f) t = 1.0f;

    float e = 1.0f - powf(1.0f - t, 3.0f);

    Camera& cam = playerRig.cam;

    // Position lerp (unchanged)
    cam.position = Vector3Lerp(
        deathCam.startPos,
        deathCam.endPos,
        e
    );

    // --- yaw turn over time ---
    float yawDeg = Lerp(0.0f, deathCam.endYawDeg, e);
    Vector3 forwardTurned = RotateY(
        deathCam.startForward,
        DEG2RAD * yawDeg
    );

    // Rebuild target from current position
    cam.target = Vector3Add(
        cam.position,
        Vector3Scale(forwardTurned, 100.0f)
    );

    // Slight downward look
    cam.target.y -= 20.0f;
}


// void CameraSystem::UpdateDeathCam(float dt)
// {

//     deathCam.t += dt;
//     float t = deathCam.t / deathCam.duration;
//     if (t > 1.0f) t = 1.0f;

//     // Ease-out cubic (feels like collapse)
//     float e = 1.0f - powf(1.0f - t, 3.0f);

//     Camera& cam = playerRig.cam;
    
//     cam.position = Vector3Lerp(
//         deathCam.startPos,
//         deathCam.endPos,
//         e
//     );

//     cam.target = Vector3Lerp(
//         deathCam.startTarget,
//         deathCam.endTarget,
//         e
//     );
// }


void CameraSystem::StartCinematic(const CinematicDesc& desc) {
    cine = desc;
    cineActive = true;

    // Enter cinematic mode first so Active() doesn't matter anymore.
    mode = CamMode::Cinematic;

    // Pick starting orbit angle deterministically
    cineOrbitAngleDeg = cine.startAngleDeg;

    Vector3 focus = cine.useDynamicFocus ? cine.dynamicFocus : cine.focus;
    float ang = DEG2RAD * cineOrbitAngleDeg;

    Vector3 startPos = {
        focus.x + sinf(ang) * cine.radius,
        focus.y + cine.height,
        focus.z + cosf(ang) * cine.radius
    };

    if (cine.snapOnStart) {
        cinematicRig.cam.position = startPos;
        cinematicRig.cam.target   = focus;
    } else {
        // If you want “ease-in” from whatever the cinematic cam currently was:
        // leave position/target as-is and smoothing will pull it toward the orbit path.
        // (But default snapOnStart=true is best for menus.)
    }

    // FOV: use current active fov if you want, or keep a fixed menu fov
    // Here: copy a reasonable value from rigs (or just set cinematicRig.fov = playerRig.fov;)
    cinematicRig.fov = (playerRig.fov > 0.f) ? playerRig.fov : 45.f;
    cinematicRig.cam.fovy = cinematicRig.fov;
    cinematicRig.cam.up   = {0,1,0};
    cinematicRig.cam.projection = CAMERA_PERSPECTIVE;
}


void CameraSystem::StopCinematic() {
    cineActive = false;
    // You choose where to return. Usually menu -> Player.
    mode = CamMode::Player;
}

void CameraSystem::SetCinematicFocus(const Vector3& p) {
    cine.focus = p;
}

void CameraSystem::Update(float dt) {

    switch (mode)
    {
        case CamMode::Player:
            if (!player.dying) UpdatePlayerCam(dt);
            break;

        case CamMode::Free:
            UpdateFreeCam(dt);
            break;

        case CamMode::Death:
            UpdateDeathCam(dt);
            break;

        case CamMode::Cinematic:
            UpdateCinematicCam(dt);
            break;
    }
        
    

    // if (mode == CamMode::Player)      UpdatePlayerCam(dt);
    // else if (mode == CamMode::Free)   UpdateFreeCam(dt);
    // else                              UpdateCinematicCam(dt);


    ApplyShake(dt);

}

void CameraSystem::UpdateCinematicCam(float dt) {
    if (!cineActive) return;

    cineOrbitAngleDeg += cine.orbitSpeedDeg * dt;
    if (cineOrbitAngleDeg >= 360.0f) cineOrbitAngleDeg -= 360.0f;
    if (cineOrbitAngleDeg < 0.0f)    cineOrbitAngleDeg += 360.0f;

    float ang = DEG2RAD * cineOrbitAngleDeg;

    Vector3 focus = cine.useDynamicFocus ? cine.dynamicFocus : cine.focus;

    Vector3 desiredPos = {
        focus.x + sinf(ang) * cine.radius,
        focus.y + cine.height,
        focus.z + cosf(ang) * cine.radius
    };

    if (cine.posSmooth <= 0.0f) cinematicRig.cam.position = desiredPos;
    else cinematicRig.cam.position = SmoothExpV3(cinematicRig.cam.position, desiredPos, cine.posSmooth, dt);

    if (cine.lookSmooth <= 0.0f) cinematicRig.cam.target = focus;
    else cinematicRig.cam.target = SmoothExpV3(cinematicRig.cam.target, focus, cine.lookSmooth, dt);

    cinematicRig.cam.up   = {0,1,0};
    cinematicRig.cam.fovy = cinematicRig.fov;

    if (debugInfo){
        SetFarClip(isDungeon ? 50000.0f : 50000.0f);
    }else{
        SetFarClip(isDungeon ? 16000.0f : 50000.0f);
    }
    
}

