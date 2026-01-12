#pragma once
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

enum class CamMode { Player, Free, Cinematic, Death};

struct DeathCamState {
    Vector3 startPos;
    Vector3 startTarget;
    Vector3 startForward;

    Vector3 endPos;
    Vector3 endTarget;

    float t = 0.0f;
    float duration = 0.8f; // tweakable

    float endYawDeg = 35.0f;   // head turns right a bit (use -35 for left)
};


struct PlayerView {
    Vector3 position;
    float yawDeg;      // player.rotation.y
    float pitchDeg;    // player.rotation.x
    bool isSwimming;
    bool onBoard;
    Vector3 boatPos;   // only used if onBoard
    float camDipY;

};

struct CameraRig {
    Camera3D cam{};
    float yaw = 0.f;
    float pitch = 0.f;
    float fov = 45.f;
    float nearClip = 60.0f;      // matches your BeginCustom3D
    float farClip = 50000.f;
    Vector3 velocity{0,0,0};    // free-cam movement
    Vector3 smoothedPos = { 0,0,0 };
    bool    hasSmoothedInit = false;
};

struct CinematicDesc {
    Vector3 focus{0,0,0};        // what we look at / orbit around
    float radius = 5000.0f;
    float height = 2000.0f;
    float orbitSpeedDeg = 3.0f;  // deg/sec

    float lookSmooth = 8.0f;
    float posSmooth  = 3.0f;

    // NEW: startup behavior
    bool snapOnStart = true;     // if true, start exactly on orbit pose immediately
    float startAngleDeg = 180.0f; // where on the ring to start (deg). 180 gives you -Z side

    // Optional: later if you want moving focus
    bool useDynamicFocus = false;
    Vector3 dynamicFocus{0,0,0};
};


class CameraSystem {
public:
    static CameraSystem& Get();

    void Init(const Vector3& startPos);
    void SyncFromPlayer(const PlayerView& view);
    void Update(float dt);

    void AttachToPlayer(const Vector3& pos, const Vector3& forward);
    void SetMode(CamMode m);
    void SnapAllToPlayer();

    CamMode GetMode() const;

    void SetFOV(float fov);
    void SetFarClip(float farClip);
    void Shake(float magnitude, float duration);

    // Cinematic control
    void StartCinematic(const CinematicDesc& desc);  // call when entering menu
    void StopCinematic();                             // call when leaving menu
    void SetCinematicFocus(const Vector3& p);         // optional runtime tweak

    void StartDeathCam(float dungeonFloorY, float terrainFloorY);

    Camera3D& Active();
    const Camera3D& Active() const;
    void BeginCustom3D(const Camera3D& cam, float nearClip, float farClip);

    bool IsPlayerMode() const { return GetMode() == CamMode::Player; }
    bool aspectSquare = false;


private:
    CameraSystem() = default;

    PlayerView pv{};
    void UpdatePlayerCam(float dt);
    void UpdateFreeCam(float dt);
    void UpdateCinematicCam(float dt);
    void UpdateDeathCam(float dt);
    void ApplyShake(float dt);

    CamMode mode = CamMode::Player;

    CameraRig playerRig{};
    CameraRig freeRig{};
    CameraRig cinematicRig{};

    // cinematic state
    CinematicDesc cine{};
    bool cineActive = false;
    float cineOrbitAngleDeg = 0.0f;
    float shakeTime = 0.0f;
    float shakeMag = 0.0f;
};
