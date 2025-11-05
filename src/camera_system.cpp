#include "camera_system.h"
#include "raymath.h"
#include "input.h" 
#include "world.h"
#include "rlgl.h"

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
    return RAD2DEG * atan2f(d.x, d.z); // x vs z (matches your forward build)
}
static inline float PitchFromDir(const Vector3& d) {
    // safer than asinf(d.y) when d isn't perfectly normalized
    return RAD2DEG * atan2f(d.y, sqrtf(d.x*d.x + d.z*d.z));
}

void CameraSystem::SnapAllToPlayer() {
    // 1) position/target from the *player camera*, not player entity
    playerRig.cam.position = playerRig.cam.position; // (no-op, just emphasis)
    Vector3 pos = playerRig.cam.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(playerRig.cam.target, pos));

    // 2) copy camera state
    freeRig.cam.position = pos;
    freeRig.cam.target   = Vector3Add(pos, dir);
    freeRig.fov          = playerRig.fov;

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
    if (mode == CamMode::Player) { playerRig.cam.fovy = playerRig.fov; return playerRig.cam; }
    else                         { freeRig.cam.fovy   = freeRig.fov;   return freeRig.cam;  }
}
const Camera3D& CameraSystem::Active() const {
    return (mode == CamMode::Player) ? playerRig.cam : freeRig.cam;
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

void CameraSystem::Update(float dt) {
    if (mode == CamMode::Player) UpdatePlayerCam(dt);
    else                         UpdateFreeCam(dt);
    ApplyShake(dt);
}



// Exponential smoothing helper
inline float SmoothStepExp(float current, float target, float speed, float dt) {
    float a = 1.0f - expf(-speed * dt);
    return current + (target - current) * a;
}


void CameraSystem::UpdatePlayerCam(float dt) {
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

    playerRig.cam.position = basePos;
    playerRig.cam.target   = Vector3Add(basePos, forward);

    // keep the angles cached so other rigs can copy
    playerRig.yaw   = pv.yawDeg;
    playerRig.pitch = pv.pitchDeg;

    SetFarClip(isDungeon ? 16000.0f : 16000.0f);
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
    float sens = 0.12f;
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
