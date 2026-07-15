#pragma once
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>

extern std::vector<BoundingBox> cameraCollisionBoxes;

namespace Cutscenes 
{
    void StartDungeonHallwayIntro();
    void StartIslandIntro();
    void StartRiverIntro();
    void StartIslandWaypointIntro();
    void StartKrakenScene();
    void StartSpiderScene();
}

enum class CamMode { 
    Player, 
    Free,
    ThirdPerson, 
    Cinematic, 
    Death};

enum class CinematicKind {
    Orbit,
    Cutscene,
    Waypoints
};

enum class CutscenePathType {
    Line,
    ArcY,
    ArcSide     // sideways relative to start -> end direction
};

struct CutsceneDesc {
    Vector3 startPos{0,0,0};
    Vector3 endPos{0,0,0};

    // Usually dungeon entrance, island center, boss, ship, etc.
    Vector3 target{0,0,0};

    float duration = 4.0f;

    //30,1 -> 8,1 -> 2,7

    // If Arc, this is how high the camera rises in the middle.
    float arcHeight = 1500.0f;

    CutscenePathType pathType = CutscenePathType::ArcSide;

    // If true, camera.target stays locked to target.
    // Later this could become startTarget/endTarget if you want target movement.
    bool lockTarget = true;

    bool snapOnStart = true;

    // Auto return to player camera when finished.
    bool returnToPlayerOnFinish = true;

    // New: near the end, blend camera.target to match player camera.
    bool mergeToPlayerViewAtEnd = false;

    // Last chunk of the cutscene used for the merge.
    // Example: 0.25 means final 25 percent of the cutscene.
    float mergeStartT = 0.75f;

    Vector3 endTarget{0,0,0};
};

struct CameraWaypoint {
    Vector3 position{0,0,0};
    Vector3 target{0,0,0};

    // Time to move from this waypoint to the next waypoint.
    // The last waypoint's duration is ignored.
    float durationToNext = 1.0f;
};

struct WaypointCutsceneDesc {
    std::vector<CameraWaypoint> points;

    bool snapOnStart = true;
    bool returnToPlayerOnFinish = true;

    // Optional: draw ceiling off during fly-throughs, same as your current cutscene.
    bool hideCeiling = false;
};

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
    float yaw = 0.0f;
    float pitch = 0.0f;
    float fov = 45.0f;
    float nearClip = 60.0f;      // matches your BeginCustom3D
    float farClip = 50000.f;
    Vector3 velocity{0,0,0};    // free-cam movement
    Vector3 smoothedPos = { 0,0,0 };
    bool    hasSmoothedInit = false;
};

struct CinematicDesc {
    Vector3 focus{0,0,0};        // what we look at / orbit around

    float radius = 10000.0f;
    float height = 4000.0f;

    float orbitSpeedDeg = 3.0f;  // deg/sec

    float lookSmooth = 8.0f;
    float posSmooth  = 3.0f;

    // NEW: startup behavior
    bool snapOnStart = true;     // if true, start exactly on orbit pose immediately
    float startAngleDeg = 180.0f; // where on the ring to start (deg). 180 gives you -Z side

    bool useDynamicFocus = false;
    Vector3 dynamicFocus{0,0,0};

    //up and down movements for orbital cam. 
    bool bobHeight = false;        
    float bobAmount = 2000.0f;     // how far up/down
    float bobSpeed = 10.0f;       // cycles-ish per second feel
    float bobPhase = 0.0f;        // optional offset
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
    void InitPlayerView();

    CamMode GetMode() const;

    CameraRig GetPlayerRig() const;

    Vector3 GetCutscenePathPosition(float t) const;
    Vector3 GetOrbitCinematicPosition(float angleDeg) const;
    void GetPlayerCameraPose(Vector3& outPos, Vector3& outTarget) const;

    void SetFOV(float fov);
    void SetFarClip(float farClip);
    void Shake(float magnitude, float duration);

    void StartCutscene(const CutsceneDesc& desc);

    // Cinematic control
    void StartCinematic(const CinematicDesc& desc);  // call when entering menu
    void StopCinematic();                             // call when leaving menu
    void SetCinematicFocus(const Vector3& p);         // optional runtime tweak

    void StartDeathCam(float dungeonFloorY, float terrainFloorY);

    CinematicDesc MakeStartupMenuCinematic();
    CinematicDesc MakeEnterMenuCinematic(bool isDungeon, bool isShip, int dungeonWidth);

    Camera3D& Active();
    const Camera3D& Active() const;
    void BeginCustom3D(const Camera3D& cam, float nearClip, float farClip);

    bool IsPlayerMode() const { return GetMode() == CamMode::Player; }

    bool IsThirdPersonMode() const { return GetMode() == CamMode::ThirdPerson; }

    void StartWaypointCutscene(const WaypointCutsceneDesc& desc);

    bool IsCutsceneActive() const { return cutsceneActive; }
    bool aspectSquare = false;





private:
    CameraSystem() = default;

    PlayerView pv{};
    void UpdateCutsceneCam(float dt);
    void UpdatePlayerCam(float dt);
    void UpdateThirdPersonCam(const PlayerView& view);
    void UpdateFreeCam(float dt);
    void UpdateCinematicCam(float dt);
    void UpdateOrbitCinematicCam(float dt);
    void UpdateDeathCam(float dt);
    void ApplyShake(float dt);
    void UpdateWaypointCutsceneCam(float dt);
    void SwitchToPlayerCamera();

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

    float cineTime = 0.0f;

    CinematicKind cineKind = CinematicKind::Orbit;

    CutsceneDesc cutscene{};
    float cutsceneT = 0.0f;
    bool cutsceneActive = false;

    WaypointCutsceneDesc waypointCutscene{};
    int waypointIndex = 0;
    float waypointT = 0.0f;
    bool waypointActive = false;

    //third person
    float thirdPersonDistance = 300.0f;
    float thirdPersonHeight   = 110.0f;//180
    float thirdPersonLookY    = 80.0f;
    

};
