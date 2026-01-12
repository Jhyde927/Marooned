#pragma once

#include <climits>
#include "raylib.h"
#include <vector>
#include "emitter.h"
#include "raymath.h"
#include <cstdint>

enum class BulletType {
    Default,
    Fireball,
    Iceball,
    Bolt,
    Harpoon,
   
};

struct BulletLight {
    bool   active = false;        // has light?
    Vector3 color = {1,1,1};      // 0..1
    float  range = 400.0f;           // > 0
    float  intensity = 0.0f;       // >= 0

    // Optional post-explosion glow
    bool   detachOnDeath = true;
    bool   detached = false;      // became a glow after bullet died
    float  age = 0.0f;
    float  lifeTime = 0.0f;        // e.g. 0.25s
    Vector3 posWhenDetached{};    // freeze final position
};


class Bullet {
public:
    Bullet(Vector3 position, Vector3 velocity, float lifetime, bool enemy,  BulletType t = BulletType::Default, float radius = 25.0f, bool launcher = false);
    Emitter fireEmitter;
    Emitter sparkEmitter;
    Vector3 position;
    Vector3 velocity;   // replaces direction and speed
    bool alive;
    bool enemy;
    bool fireball = false;
    float age;
    float maxLifetime;
    float lifeTime = 1.5f;
    float timeSinceImpact = 0.0f;
    float timer;
    float gravity = 300.0f;
    float radius = 25.0f; //draw radius or collider radius? 
    float spinAngle = 0.0f;
    float size = 3.0f;
    
    //Grapple
    bool stuckToGrapple = false;
    Vector3 stuckWorldPos = {0,0,0};   // where it sticks if not attached to enemy
    bool wizard = false; 
    bool exploded = false;
    float timeSinceExploded = 0.0f;
    bool explosionTriggered = false;
    unsigned int id;            // <-- unique per bullet
    bool   stuck = false;
    int    stuckEnemyId = -1;
    Vector3 stuckOffset = {0};   // world-space offset from enemy.position
    bool retracting = false;
    float retractSpeed = 4000.0f; // units/sec tweak

    Vector3 retractTip = {0};     // where the rope tip currently is during retract

    Quaternion rotation;   // NEW
    BulletLight light;
    void Update(Camera& camera, float deltaTime);
    void UpdateMagicBall(Camera& camera, float deltaTime);
    void Erase();
    void Draw(Camera& camera) const;
   
    void kill(Camera& camera);

    //These just return the public members now. Clean this up eventually
    bool IsExpired() const;
    bool IsAlive() const;
    bool IsEnemy() const;
    bool isFireball() const;
    bool isExploded() const;
    bool IsDone();
    void Explode(Camera& camera);
    void BulletHole(Camera& camera, bool enemy = false);
    void HandleBulletWorldCollision(Camera& camera);

    float ComputeDamage();
    float GetRadius() const { return radius; }
    bool pendingExplosion = false;
    float explosionTimer = 0.0f;
    bool launcher = false;
    BulletType type = BulletType::Default;
    Vector3 GetPosition() const;
    Vector3 prevPosition;
    int curTileX = INT_MIN, curTileY = INT_MIN;
    float killFloorY = 0;          // Y at which to kill this bullet in the current tile
    bool tileIsLava = false;              // cached flag for current tile

    float   baseDamage = 10.0f;
    float   initialSpeed = 1.0f;  // set when fired


};
void ExplodeShrapnelSphere(Vector3 origin, int pelletCount =10,float speed = 2000.0f,float lifetime = 0.6f,bool enemy = false);
void FireBlunderbuss(Vector3 origin, Vector3 forward, float spreadDegrees, int pelletCount, float speed, float lifetime, bool enemy);
void FireBullet(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy);
void FireFireball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool launcher, bool wizard);
void FireIceball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool launcher);
void FireCrossbow(Vector3 origin, Vector3 forward, float speed, float lifetime, bool enemy);
void FireCrossbowHarpoon(Vector3 origin, Vector3 forward, float speed, float lifetime, bool enemy);
Vector3 GetHarpoonAnchor(const Camera& cam);