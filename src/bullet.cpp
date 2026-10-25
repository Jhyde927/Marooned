#include "bullet.h"
#include "raymath.h"
#include <raylib.h>
#include <cmath>
#include <algorithm>
#include "world.h"
#include "resourceManager.h"
#include "decal.h"
#include "sound_manager.h"
#include "utilities.h"
#include "dungeonGeneration.h"
#include "pathfinding.h"
#include "lighting.h"
#include "collisions.h"
#include "rlgl.h"
#include <cstdint>
#include <utility>
#include "particleSystem.h"

static unsigned int gBulletCounter = 1; // 0 is the existing "no bullet hit" sentinel
static std::vector<Bullet> pendingBulletSpawns;

void FlushPendingBulletSpawns()
{
    for (Bullet& b : pendingBulletSpawns) activeBullets.emplace_back(std::move(b));
    pendingBulletSpawns.clear();
}

void ClearPendingBulletSpawns()
{
    pendingBulletSpawns.clear();
}

Bullet::Bullet(Vector3 startPos, Vector3 vel, float lifetime, bool en, BulletType t, float r, bool launch)
    : position(startPos),
      velocity(vel),
      lifeTime(lifetime),
      enemy(en),
      type(t),
      radius(r),
      launcher(launch),
      alive(true),
      age(0.0f)
{
    prevPosition = startPos;
    initialSpeed = std::max(1.0f, Vector3Length(vel));
}


static inline unsigned char LerpByte(unsigned char a, unsigned char b, float t) {
    return (unsigned char)(a + (b - a) * t);
}


Color BulletHeatHSV(float t)
{
    t = Clamp(t, 0.0f, 1.0f);

    float h = 0.0f;
    float s = 0.0f;
    float vRaw = 1.0f;   // "HDR-ish" value, can go > 1.0

    if (isDungeon)
    {
        // SUPER EXAGGERATED DUNGEON HEAT:
        // - Phase 1: crazy bright, almost white-hot
        // - Phase 2: bright orange/red
        // - Phase 3: dim, cooling ember

        if (t < 0.33f) {
            float k = t / 0.33f;
            h    = 40.0f;                      // warm yellowish white
            s    = 0.1f + k * 0.6f;            // 0.1 → 0.7
            vRaw = 2.5f - k * 0.3f;            // 2.5 → 2.2 (stays VERY bright)
        }
        else if (t < 0.66f) {
            float k = (t - 0.33f) / 0.33f;
            h    = Lerp(40.0f, 10.0f, k);      // yellow → red/orange
            s    = 0.9f + k * 0.2f;            // 0.9 → 1.1 (over-sat in mid)
            vRaw = Lerp(2.2f, 1.2f, k);        // 2.2 → 1.2 (still bright)
        }
        else {
            float k = (t - 0.66f) / 0.34f;
            h    = Lerp(10.0f, 0.0f, k);       // red → gray
            s    = Lerp(0.7f, 0.0f, k);        // lose saturation
            vRaw = Lerp(1.0f, 0.3f, k);        // 1.0 → 0.3 (dim coal)
        }
    }
    else
    {
        // Overworld: metal fragments cooling
        h    = 0.0f;                           // gray
        s    = 0.0f;
        vRaw = Lerp(10.0f, 1.0f, t);            // bright → dark gray
    }

    // Compress HDR-ish vRaw back to 0–1 for ColorFromHSV
    const float MAX_V_RAW = 2.5f;             // matches our hottest value
    float v = vRaw / MAX_V_RAW;

    s = Clamp(s, 0.0f, 1.0f);
    v = Clamp(v, 0.0f, 1.0f);

    return ColorFromHSV(h, s, v);
}




void Bullet::UpdateMagicBall(Camera& camera, float deltaTime)
{
    if (!alive || explosionTriggered) return;
    gravity = launcher ? 0.0f : 980.0f;
    // Preserve the short "travel inside the enemy" delay, without resetting it
    // on repeated collisions or travelling farther than the time remaining.
    const float moveTime = pendingExplosion ? std::min(deltaTime, explosionTimer) : deltaTime;
    velocity.y -= gravity * moveTime;
    position = Vector3Add(position, Vector3Scale(velocity, moveTime));
    spinAngle = std::fmod(spinAngle + 90.0f * deltaTime, 360.0f);
    if (pendingExplosion)
    {
        explosionTimer -= deltaTime;
        // Defer the actual explosion until HandleBulletCollisions has checked
        // this final movement segment against solid world geometry.
    }
    (void)camera;
}


void Bullet::Update(Camera& camera, float deltaTime, ParticleSystem& particles)
{
    prevPosition = position; // includes magic balls; initialized at spawn too
    if (!alive)
    {
        timeSinceImpact += deltaTime;
        return;
    }
    age += deltaTime; // exactly once for every type

    if (explosionTriggered)
    {
        timeSinceExploded += deltaTime;
        if (timeSinceExploded >= 2.0f) alive = false;
        return; // explosion glow stays at its impact position
    }

    lifeTime -= deltaTime;
    const bool magic = type == BulletType::Fireball || type == BulletType::Iceball;
    if (type == BulletType::Harpoon)
    {
        if (retracting)
        {
            const Vector3 anchor = GetHarpoonAnchor();
            const Vector3 delta = Vector3Subtract(anchor, retractTip);
            const float distance = Vector3Length(delta);
            if (distance <= 10.0f) { kill(camera); return; }
            const float step = std::min(retractSpeed * deltaTime, distance);
            retractTip = Vector3Add(retractTip, Vector3Scale(delta, step / distance));
            position = retractTip;
            return;
        }
        if (stuck)
        {
            if (lifeTime <= 0) { kill(camera); return; }
            if (stuckToGrapple) position = stuckWorldPos;
            else
            {
                Character* e = FindEnemyById(stuckEnemyId);
                if (!e || e->isDead) { kill(camera); return; }
                position = Vector3Add(e->position, stuckOffset);
            }
            return;
        }
        if (lifeTime <= 0)
        {
            retracting = true;
            retractTip = position;
            velocity = {0,0,0};
            return;
        }
    }
    else if (lifeTime <= 0)
    {
        // Expiration is not an impact. Do not create sparks/explosions here.
        kill(camera);
        return;
    }

    if (magic)
    {
        if (type == BulletType::Fireball)
        {
            particles.EmitTrail(position, deltaTime, 100.0f, smokeEmissionAccumulator, ParticleType::Smoke);
            particles.EmitTrail(position, deltaTime, 100.0f, fireEmissionAccumulator, ParticleType::FireTrail);
        }
        else particles.EmitTrail(position, deltaTime, 100.0f, smokeEmissionAccumulator, ParticleType::IceMist);
        UpdateMagicBall(camera, deltaTime);
        return;
    }

    if (type == BulletType::Bolt || type == BulletType::Harpoon)
    {
        velocity.y -= gravity * deltaTime;
        const Color trail = {255,255,255,1};
        particles.EmitTrail(position, deltaTime, 20.0f, smokeEmissionAccumulator, ParticleType::BoltTrail, trail);
    }
    else if (type == BulletType::CannonBall)
    {
        velocity.y -= gravity * deltaTime;
        particles.EmitTrail(position, deltaTime, 50.0f, smokeEmissionAccumulator, ParticleType::Smoke);
    }
    else if (!enemy)
    {
        velocity.y -= gravity * deltaTime;
        particles.EmitTrail(position, deltaTime, 3.0f, smokeEmissionAccumulator, ParticleType::Smoke);
    }
    position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
    // No low-speed kill here: it previously killed a successful bounce on the
    // following update. ComputeDamage already gives slow default bullets 0 damage.
}

inline void BeginBulletTransform(const Vector3& pos, float age)
{
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);

    // Make rotation speed scale with bullet age
    float rot = age * 720.0f;  // 720° per second looks chaotic and metallic
    rlRotatef(rot, 0.4f, 1.0f, 0.8f);  // rotate around an arbitrary, diagonal axis

}

inline void EndBulletTransform()
{
    rlPopMatrix();
}


Vector3 GetHarpoonAnchor()
{
    return crossbow.muzzlePos;
}


void DrawBolt(const Bullet& b)
{
    if (b.type != BulletType::Bolt) return;

    // quaternion → axis/angle
    Quaternion q = b.rotation;
    float angle = 2.0f * acosf(q.w);
    float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - q.w * q.w));

    Vector3 axis = (sinTheta < 0.001f)
        ? Vector3{ 0, 1, 0 }
        : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    float angleDeg = angle * RAD2DEG;

    // Shaft: bright white
    DrawModelEx(
        R.GetModel("squareBolt"),
        b.position,
        axis,
        angleDeg,
        { 1.0f, 1.0f, 1.0f },
        LIGHTGRAY
    );


}

void DrawSpiralRope(Vector3 anchor, Vector3 tip, float timeSec)
{
    Vector3 delta = Vector3Subtract(tip, anchor);
    float len = Vector3Length(delta);
    if (len < 5.0f) return;

    Vector3 dir = Vector3Scale(delta, 1.0f / len);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(dir, (fabsf(dir.y) < 0.99f) ? Vector3{0,1,0} : Vector3{1,0,0}));
    Vector3 up    = Vector3Normalize(Vector3CrossProduct(right, dir));

    const int segments = 18;
    const float radius = 0.5f;

    float amp = Clamp(len * 0.006f, 1.5f, 6.0f);

    // how many turns along the rope (slow spiral = < 1.5 turns)
    float turns = 2.0f;

    Vector3 prev = anchor;

    for (int i = 1; i <= segments; i++)
    {
        float t = (float)i / (float)segments;
        Vector3 p = Vector3Add(anchor, Vector3Scale(delta, t));

        float angle = (t * 6.28318f * turns) + timeSec * 0.8f; // rotate slowly over time
        float cs = cosf(angle), sn = sinf(angle);

        // rotating offset around the rope axis
        Vector3 offset = Vector3Add(Vector3Scale(right, cs * amp),
                                    Vector3Scale(up,    sn * amp));
        p = Vector3Add(p, offset);

        Color ropeColor = {150,75,30,255};

        DrawCylinderEx(prev, p, radius, radius, 6, ropeColor);
        prev = p;
    }
}


void DrawHarpoon(const Bullet& b, const Camera& camera)
{
    (void)camera;
    if (b.type != BulletType::Harpoon) return;

    // quaternion → axis/angle (same as DrawBolt)
    Quaternion q = b.rotation;
    float angle = 2.0f * acosf(q.w);
    float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - q.w * q.w));

    Vector3 axis = (sinTheta < 0.001f)
        ? Vector3{ 0, 1, 0 }
        : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    float angleDeg = angle * RAD2DEG;

    // Draw the bolt model (same model)
    DrawModelEx(
        R.GetModel("squareBolt"),
        b.position,
        axis,
        angleDeg,
        { 1.0f, 1.0f, 1.0f },
        LIGHTGRAY
    );

    // Rope
    Vector3 anchor = GetHarpoonAnchor();

    Vector3 tip = b.position;


    if (b.stuck)
    {
        Character* e = FindEnemyById(b.stuckEnemyId); // lookup in enemyPtrs 
        if (e)
            tip = Vector3Add(e->position, b.stuckOffset);
    }

    if (b.retracting) tip = b.retractTip;



    // Avoid weirdness if tip is super close
    float d = Vector3Distance(anchor, tip);
    if (d > 10.0f)
    {
        DrawSpiralRope(anchor, tip, ElapsedTime);
    }
}


void Bullet::Draw(Camera& camera) const {
    // fireEmitter.Draw(camera); //explosion particles
    // sparkEmitter.Draw(camera); //firetrail


    if (exploded) return;

    if (type == BulletType::Fireball){
        Vector3 scale = wizard ? Vector3 {10.0, 10.0, 10.0} : Vector3 {20.0, 20.0, 20.0};
            //dont draw the ball or firetrail if it's exploded. 
        DrawModelEx(R.GetModel("fireballModel"), position, { 0, 1, 0 }, spinAngle, scale, WHITE);
            
        
        
    }else if (type == BulletType::Iceball){
        
        DrawModelEx(R.GetModel("iceballModel"), position, { 0, 1, 0 }, spinAngle, { 20.0f, 20.0f, 20.0f }, WHITE);
            
    }else if (type == BulletType::Bolt){

        DrawBolt(*this);

    }else if (type == BulletType::Harpoon){
        DrawHarpoon(*this, camera);

    }else if (type == BulletType::CannonBall){
        DrawModelEx(R.GetModel("cannonBall"), position, { 0, 1, 0 }, spinAngle, Vector3 {25.0f, 25.0f, 25.0f}, DARKGRAY);


    } else{ //regular bullets
        float t = Clamp(age / 1.5, 0.0f, 1.0f);
        
        Color heat = BulletHeatHSV(t);//BulletHeatColor(t);//shrapnel cools off over time in dungeons, outside bullets are white

        // Pop cube for very early frame
        if (age < 0.5f) {
            BeginBulletTransform(position, age); //rotate the cubes, arbitary rotation
            DrawCube({0,0,0}, 3.5f, 3.5f, 3.5f, heat);
            EndBulletTransform();
        }

        // Main bullet cube
        BeginBulletTransform(position, age);
        DrawCube({0,0,0}, size, size, size, heat);
        EndBulletTransform();
    }
}

bool Bullet::IsDone()
{
    // Keep dead projectiles briefly for any light fade/cleanup that uses them.
    if (!alive) return timeSinceImpact >= std::max(0.25f, light.lifeTime);
    return false;
}

bool Bullet::IsExpired() const {
    return lifeTime <= 0.0f;
}


bool Bullet::IsAlive() const {
    return alive;
}

bool Bullet::IsEnemy() const {
    return enemy;
}

bool Bullet::isFireball() const {
    return fireball;
}

bool Bullet::isExploded() const {
    return exploded;
}

Vector3 Bullet::GetPosition() const {
    return position;
}

void Bullet::Erase(){
    alive = false;
}

void Bullet::kill(Camera& camera){
    (void)camera;
    alive = false;
    exploded = true;
    pendingExplosion = false;
    velocity = {0,0,0};
    
}

void Bullet::BulletHole(Camera& camera, bool enemy){
    //spawn a bullet hole decal before dying. Push backward for enemies and forward for player. 

    float forward = enemy ? -200 : 200;
    float size = enemy ? 25 : 50;

    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, camera.position));
    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, forward));

    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("bulletHoleSheet"), 5, 1.0f, 0.2f, size);
    exploded = true;
    alive = false; //kill the bullet

}

float Bullet::ComputeDamage()
{
    float speed = Vector3Length(velocity);
    float qdamage = 1.0f;

    if (player.quadDamage){
        qdamage = 4.0f;
    }

    // No damage if speed is below a threshold (tune this)
    if (speed < 800.0f) {
        return 0.0f;
    }

    // Protect against divide-by-zero or uninitialized shots
    float denom = (initialSpeed > 1.0f) ? initialSpeed : 1.0f;

    float vFactor = speed / denom;

    // clamp to [0,1]
    vFactor = Clamp(vFactor, 0.0f, 1.0f);
    

    return baseDamage * vFactor * qdamage;
}


void Bullet::Explode(Camera& camera) {
    if (!alive || explosionTriggered) return;
    if (type != BulletType::Fireball && type != BulletType::Iceball &&
        type != BulletType::CannonBall) return;

    if (!explosionTriggered){
        explosionTriggered = true;
        exploded = true;
        pendingExplosion = false;
        explosionTimer = 0;
        timeSinceExploded = 0;
        velocity = {0,0,0};
        light.posWhenDetached = position;
        light.detached = true;
        light.age = 0;

        SoundManager::GetInstance().PlaySoundAtPosition("explosion", position, player.position, player.rotation.y, 3000.0f);
        
        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, camera.position));
        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, 100.0f));

        if (type == BulletType::Fireball || type == BulletType::CannonBall){
            decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("explosionSheet"), 13, 1.0f, 0.1f, 500.0f);
            particleSystem.EmitBurst(position, 200, ParticleType::Sparks);

            ExplodeShrapnelSphere(position, 10, 1500.0f, 1.0f, false);
        }else if (type == BulletType::Iceball){
            particleSystem.EmitBurst(position, 200, ParticleType::IceBlast);

        }

        float minDamage = 10.0f;
        float maxDamage = 200.0f;
        float explosionRadius = 400.0f;
        float r2 = explosionRadius * explosionRadius;

        if (type == BulletType::Fireball)
        {
            // Enemy splash
            for (Character* enemy : enemyPtrs)
            {
                if (!enemy || enemy->isDead) continue;
                Vector3 d = Vector3Subtract(enemy->position, position);
                float dist2 = Vector3DotProduct(d, d);

                if (dist2 < r2)
                {
                    if (HasWorldLineOfSight(position, enemy->position, 0.01f))
                    {
                        float dist = sqrtf(dist2); // only now pay for sqrt
                        float t = dist / explosionRadius; // 0..1
                        float dmg = Lerp(maxDamage, minDamage, t);

                        if (enemy->type != CharacterType::Wizard)
                            enemy->TakeDamage(dmg);

 
                    }
                }
            }

            // Player splash
            float pDamage = 50.0f;

            Vector3 dp = Vector3Subtract(player.position, position);
            float pdist2 = Vector3DotProduct(dp, dp);

            if (pdist2 < r2)
            {
                float pdist = sqrtf(pdist2);
                float t = pdist / explosionRadius;
                float dmg = Lerp(pDamage, minDamage, t);
                player.TakeDamage(dmg);
            }
        

        }else if (type == BulletType::Iceball){
            for (Character* enemy : enemyPtrs){
                if (!enemy || enemy->isDead) continue;
                float dist = Vector3Distance(position, enemy->position);
                if (dist < explosionRadius && !enemy->isDead) {
                    float dmg =  Lerp(maxDamage, minDamage, dist / explosionRadius);
                    enemy->ChangeState(CharacterState::Freeze);
                    enemy->TakeDamage(dmg);
                    //enemy->currentHealth -= dmg; //dont call take damage, it triggers stagger which over rides freeze. 
                    //can cause invincible skeletons if they die to freeze, we check health again on freeze state for skeles not pirates.
                    

                }
            }

            //damage player if too close, 
            float pDamage = 50.0f;
            float pdist = Vector3Distance(player.position, position);
            if (pdist < explosionRadius){
                float dmg =  Lerp(pDamage, minDamage, pdist / explosionRadius); //damage fall off with distance.
                player.TakeDamage(dmg);//upto 50 percent health may be a lot for direct hit. 
            }

        }

    }

}

// Random unit vector on a sphere
static inline Vector3 RandomUnitVectorSphere()
{
    // z in [-1,1], theta in [0,2pi)
    float z     = RandomFloat(-1.0f, 1.0f);
    float theta = RandomFloat(0.0f, 2.0f * PI);

    float r = sqrtf(fmaxf(0.0f, 1.0f - z*z));

    return {
        r * cosf(theta),
        z,
        r * sinf(theta)
    };
}

// Fireball shrapnel burst: bullets in ALL directions
void ExplodeShrapnelSphere(Vector3 origin, int pelletCount,float speed,float lifetime,bool enemy)
{
    // Raise explosion slightly above ground
    origin.y += 50.0f;  
    for (int i = 0; i < pelletCount; ++i)
    {
        Vector3 dir = RandomUnitVectorSphere();
        Vector3 velocity = Vector3Scale(dir, speed);
 
        Bullet b = { origin, velocity, lifetime, enemy, BulletType::Default };

        b.initialSpeed = Vector3Length(b.velocity);
        if (b.initialSpeed < 1.0f) b.initialSpeed = 1.0f;
        b.size = 10.0f;
        b.id = gBulletCounter++;
        b.light.active = true; //glowing shrapnel
        pendingBulletSpawns.emplace_back(b);
    }
}

void FireCrossbow(Vector3 origin, Vector3 forward, float speed, float lifetime, bool enemy) {
    Vector3 vel = Vector3Scale(forward, speed);
    Bullet bolt = {origin, vel, lifetime, enemy, BulletType::Bolt};
    bolt.rotation =  QuaternionFromVector3ToVector3({0,0,1}, forward);
    bolt.id = gBulletCounter++;
    activeBullets.emplace_back(bolt);
}

void FireCrossbowHarpoon(Vector3 origin, Vector3 forward, float speed, float lifetime, bool enemy) {
    Vector3 vel = Vector3Scale(forward, speed);
    Bullet harpoon = {origin, vel, lifetime, enemy, BulletType::Harpoon};
    harpoon.rotation =  QuaternionFromVector3ToVector3({0,0,1}, forward);
    harpoon.id = gBulletCounter++;
    harpoon.lifeTime = 2.5f;
    activeBullets.emplace_back(harpoon);
}

void FireBlunderbuss(Vector3 origin, Vector3 forward, float spreadDegrees, int pelletCount, float speed, float lifetime, bool enemy) {
    if (weapon.doubleLoadScheduled) return;
    for (int i = 0; i < pelletCount; ++i) {
        
        // Convert spread from degrees to radians
        float spreadRad = spreadDegrees * DEG2RAD;

        // Random horizontal and vertical spread
        float yaw   = RandomFloat(-spreadRad, spreadRad);
        float pitch = RandomFloat(-spreadRad, spreadRad);


        // Create rotation matrix from yaw and pitch
        Matrix rotYaw = MatrixRotateY(yaw);
        Matrix rotPitch = MatrixRotate(Vector3CrossProduct(forward, { 0, 1, 0 }), pitch);
        Matrix spreadMatrix = MatrixMultiply(rotYaw, rotPitch);

        Vector3 dir = Vector3Transform(forward, spreadMatrix);
        dir = Vector3Normalize(dir);

        Vector3 velocity = Vector3Scale(dir, speed);

        Bullet b = {origin, velocity, lifetime, enemy, BulletType::Default};

        b.initialSpeed = Vector3Length(b.velocity); //set initial speed.
        

        b.light.range      = lightConfig.dynamicRange;
        b.light.intensity  = 0.0f;
        b.light.color = lightConfig.dynamicFireColor;
        b.light.active = true;
        b.light.detachOnDeath = false;
    
        if (b.initialSpeed < 1.0f) b.initialSpeed = 1.0f;
        b.id = gBulletCounter++;
        activeBullets.emplace_back(b);

    }
}

void FireCannon(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy){
    Vector3 direction = Vector3Subtract(target, origin);
    direction = Vector3Normalize(direction);
    Vector3 velocity = Vector3Scale(direction, speed);
    Bullet b = {origin, velocity, lifetime, enemy};
    b.type = BulletType::CannonBall;
    b.alive = true;
    b.id = gBulletCounter++;
    activeBullets.push_back(b);

    SoundManager::GetInstance().PlaySoundAtPosition("CannonShot", b.position, player.position, 0.0f, 10000.0f);
}

void FireBullet(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool hermit, Character* owner) {
    Vector3 direction = Vector3Subtract(target, origin);
    direction = Vector3Normalize(direction);
    Vector3 velocity = Vector3Scale(direction, speed);
    Bullet b = {origin, velocity, lifetime, enemy};
    b.id = gBulletCounter++;
    b.hermit = hermit;
    b.light.active = true; //glowing enemy bullets. 
    b.owner = owner;

    activeBullets.push_back(b);
}

void FireFireball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool launcher, bool wizard) {
    Vector3 direction = Vector3Normalize(Vector3Subtract(target, origin));
    Vector3 velocity = Vector3Scale(direction, speed);


    Bullet& b = activeBullets.emplace_back(origin, velocity, lifetime, enemy, BulletType::Fireball, 20.0f, launcher);

    b.light.active     = true;
    b.light.color      = (b.type==BulletType::Fireball)? lightConfig.dynamicFireColor : lightConfig.dynamicIceColor;
    b.light.range      = lightConfig.dynamicRange;
    b.light.intensity  = lightConfig.dynamicIntensity;
    b.light.detachOnDeath = true;
    b.light.lifeTime   = 0.15f; // short glow after death
    b.id = gBulletCounter++;
    b.wizard = wizard;
    

    

    SoundManager::GetInstance().PlaySoundAtPosition((rand() % 2 == 0 ? "flame1" : "flame2"), origin, player.position, 0.0f, 3000.0f);
}

void FireIceball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool launcher) {
    Vector3 direction = Vector3Normalize(Vector3Subtract(target, origin));
    Vector3 velocity = Vector3Scale(direction, speed);

    Bullet& b = activeBullets.emplace_back(origin, velocity, lifetime, enemy, BulletType::Iceball, 20.0f, launcher); //larger radius 75, so you can't avoid in hallways

    b.light.active     = true;
    b.light.color      = lightConfig.dynamicIceColor;
    b.light.range      = lightConfig.dynamicRange;
    b.light.intensity  = lightConfig.dynamicIntensity;
    b.light.detachOnDeath = true;
    b.light.lifeTime   = 0.15f; // short glow after death
    b.id = gBulletCounter++;

    SoundManager::GetInstance().PlaySoundAtPosition("iceMagic", origin, player.position, 0.0f, 3000.0f);
}

