#include "bullet.h"
#include "raymath.h"
#include <raylib.h>
#include <cmath>
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

static unsigned int gBulletCounter = 0;

Bullet::Bullet(Vector3 startPos, Vector3 vel, float lifetime, bool en, BulletType t, float r, bool launch)
    : position(startPos),
      velocity(vel),
      alive(true),
      age(0.0f),
      maxLifetime(lifetime),
      enemy(en),
      type(t),
      fireEmitter(startPos),
      sparkEmitter(startPos),
      radius(r),
      launcher(launch)
{}


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




void Bullet::UpdateMagicBall(Camera& camera, float deltaTime) {
    //if (!alive) return;

    // Gravity-based arc
    
    gravity = launcher ? 0 : 980.0f; //fireballs fired from traps have no gravity
    fireEmitter.SetPosition(position);
    sparkEmitter.SetPosition(position);
    
    velocity.y -= gravity * deltaTime;

    // Move
    position = Vector3Add(position, Vector3Scale(velocity, deltaTime));

    spinAngle += 90.0f * deltaTime; // 90 degrees per second
    if (spinAngle >= 360.0f) spinAngle -= 360.0f;

    // Collision with floor
    if (isDungeon) {
        if (position.y <= floorHeight) {
            //Explode(camera);
            Explode(camera);

            return;
        }
        if (position.y >= ceilingHeight) {
            Explode(camera);
            
            return;
        }
    } else {
        float terrainHeight = GetHeightAtWorldPosition(position, heightmap, terrainScale);
        if (position.y <= terrainHeight) {
            Explode(camera);
            return;
        }
    }

    if (pendingExplosion) {
        explosionTimer -= deltaTime;
        if (explosionTimer <= 0.0f) {
            Explode(camera); // now do the actual explosion logic
            pendingExplosion = false;
        }
    }

    // Lifetime kill
    age += deltaTime;
    if (age >= maxLifetime && type == BulletType::Default) {
        kill(camera);
    }



}

void Bullet::HandleBulletWorldCollision(Camera& camera){
    // Handle floor/ceiling/terrain collision
    if (isDungeon) {
        if (drawCeiling && position.y >= ceilingHeight){
            Vector3 n = {0, 1, 0};
            TryBulletRicochet(*this, n, 0.6f, 500, 0.99);

        }


        // Recompute only if we changed tiles
        int tx = GetDungeonImageX(position.x, tileSize, dungeonWidth);
        int ty = GetDungeonImageY(position.z, tileSize, dungeonWidth);
        if (tx != curTileX || ty != curTileY) {
            curTileX = tx; curTileY = ty;
            tileIsLava = (lavaMask[Idx(tx, ty)] == 1);

            // If lava: let bullets sink *below* the normal floor before killing.
            // Otherwise: normal floor kill right at floorHeight.
            killFloorY = tileIsLava ? (floorHeight - 150)    // 150
                                    : (floorHeight+20);
        }

        // Continuous check to avoid tunneling
        if (prevPosition.y > killFloorY && position.y <= killFloorY) {
            Vector3 n = {0, 1, 0};
            TryBulletRicochet(*this, n, 0.6f, 500, 0.99);

            return;
        }
    } else {
        // Overworld: terrain height varies; sampling each frame is fine.
        float h = GetHeightAtWorldPosition(position, heightmap, terrainScale);
        if (prevPosition.y > h && position.y <= h) {
            Vector3 n = {0, 1, 0};
            if (position.y <= 60){
                BulletParticleBounce(*this, SKYBLUE);
            }else{
                
                if (TryBulletRicochet(*this, n, 0.6, 500, 0.7)){
                    //do nothing
                }else{
                    BulletParticleBounce(*this, BROWN);//kick up dirt and die. 
                }
            }

            return;
        }
    }

}



void Bullet::Update(Camera& camera, float deltaTime) {
    fireEmitter.Update(deltaTime);
    sparkEmitter.Update(deltaTime); 
    lifeTime -= deltaTime;
    if (!alive){
        timeSinceImpact += deltaTime;
    } 

    if (type == BulletType::Harpoon && retracting)
    {
        Vector3 anchor = GetHarpoonAnchor(camera); // or pass in anchor from player update

        Vector3 toA = Vector3Subtract(anchor, retractTip);
        float d = Vector3Length(toA);

        if (d < 10.0f)
        {
            lifeTime = 0.0f; // now actually kill it
            exploded = true;
            alive = false;
        }
        else
        {
            Vector3 dir = Vector3Scale(toA, 1.0f / d);
            float step = retractSpeed * deltaTime;
            float move = fminf(step, d);
            retractTip = Vector3Add(retractTip, Vector3Scale(dir, move));
        }

        // Optional: keep the bolt model riding the retract tip
        //position = retractTip;
    }




    // Fireball logic
    if (type == BulletType::Fireball) {
        
        fireEmitter.SetParticleType(ParticleType::Smoke);

        sparkEmitter.SetParticleType(ParticleType::FireTrail);
        sparkEmitter.UpdateTrail(deltaTime);
        fireEmitter.UpdateTrail(deltaTime);
        UpdateMagicBall(camera, deltaTime);

        if (!exploded && explosionTriggered) {
            exploded = true;
     
        }
        
        if (exploded){
            timeSinceExploded += deltaTime;

            if (timeSinceExploded >= 2.0f) { //wait for particles to act. 
                alive = false;
                return;

            }

        }
        return; //skip normal bullet logic
    }
    else if (type == BulletType::Iceball){
        sparkEmitter.SetParticleType(ParticleType::IceMist);
        fireEmitter.SetParticleType(ParticleType::IceMist);
        fireEmitter.UpdateTrail(deltaTime);
        sparkEmitter.UpdateTrail(deltaTime);

        UpdateMagicBall(camera, deltaTime);

        if (!exploded && explosionTriggered) {
            exploded = true;
     
        }
        
        if (exploded){
            timeSinceExploded += deltaTime;

            if (timeSinceExploded >= 2.0f) { //wait for particles to act. 
                alive = false;
                return;

            }

        }
        return; //skip normal bullet logic

    }else if (type == BulletType::Bolt || type == BulletType::Harpoon){
        velocity.y -= gravity * deltaTime;
        fireEmitter.SetParticleType(ParticleType::BoltTrail);
        fireEmitter.SetPosition(position);
        fireEmitter.SetParticleSize(1.0f);
        fireEmitter.SetEmissionRate(50.0f);
        fireEmitter.SetColor(LIGHTGRAY);
        fireEmitter.UpdateTrail(deltaTime); //smoke trail update
    }


    // Standard bullet movement (non-fireball)
    if (!IsEnemy() && type == BulletType::Default){
        fireEmitter.UpdateTrail(deltaTime); //smoke trail update
        velocity.y -= gravity * deltaTime;
        fireEmitter.SetParticleType(ParticleType::Smoke);
        fireEmitter.SetPosition(position);
        fireEmitter.SetParticleSize(2.0f);
        fireEmitter.SetEmissionRate(10.0f);
        fireEmitter.SetColor(DARKGRAY);

    }

    prevPosition = position;
    position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
    age += deltaTime;

    if (age >= maxLifetime && !exploded) alive = false;
    if (type == BulletType::Harpoon && stuck && lifeTime <= 0.0f){
        alive = false;
        exploded = true;
    }
    if (type == BulletType::Harpoon && !stuck && lifeTime <= 0.0f)
    {
       retracting = true;
       retractTip = position;
       velocity = {0,0,0};
       lifeTime = 9999.0f; // keep it alive while retracting
       if (Vector3Distance(position, player.position) < 100 ){
            alive = false;
            exploded = true;
       } 
    }

    HandleBulletWorldCollision(camera);

    float speed = Vector3Length(velocity);
    if (speed < 800.0f && type != BulletType::Harpoon) {   //  ~ one bounce theoretically
        alive = false;
        exploded = true;
        
    }
}

inline void BeginBulletTransform(const Vector3& pos, float age, float size)
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


Vector3 GetHarpoonAnchor(const Camera& cam)
{
    Vector3 f = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 r = Vector3Normalize(Vector3CrossProduct(f, cam.up));
    Vector3 u = Vector3Normalize(cam.up);

    Vector3 p = cam.position;
    p = Vector3Add(p, Vector3Scale(r,  25.0f));
    p = Vector3Add(p, Vector3Scale(u, -20.0f));
    p = Vector3Add(p, Vector3Scale(f,  60.0f));
    return p;
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

void DrawHarpoon(const Bullet& b, const Camera& camera)
{
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
    Vector3 anchor = GetHarpoonAnchor(camera);

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
        float ropeRadius = 0.5f;   // tweak: 1.0 - 3.0
        int ropeSides = 6;         // cheap but round enough

        DrawCylinderEx(anchor, tip, ropeRadius, ropeRadius, ropeSides,
                       (Color){150, 75, 30, 255});
    }
}








void Bullet::Draw(Camera& camera) const {
    fireEmitter.Draw(camera); //explosion particles
    sparkEmitter.Draw(camera); //firetrail

    if (exploded) return;

    if (type == BulletType::Fireball){
         
            //dont draw the ball or firetrail if it's exploded. 
        DrawModelEx(R.GetModel("fireballModel"), position, { 0, 1, 0 }, spinAngle, { 20.0f, 20.0f, 20.0f }, WHITE);
            
        
        
    }else if (type == BulletType::Iceball){
        
        DrawModelEx(R.GetModel("iceballModel"), position, { 0, 1, 0 }, spinAngle, { 25.0f, 25.0f, 25.0f }, WHITE);
            
    }else if (type == BulletType::Bolt){

        DrawBolt(*this);

    }else if (type == BulletType::Harpoon){
        DrawHarpoon(*this, camera);

    } else{ //regular bullets
        float t = Clamp(age / 1.5, 0.0f, 1.0f);
        
        Color heat = BulletHeatHSV(t);//BulletHeatColor(t);//shrapnel cools off over time in dungeons, outside bullets are white

        // Pop cube for very early frame
        if (age < 0.5f) {
            BeginBulletTransform(position, age, 3.5f); //rotate the cubes, arbitary rotation
            DrawCube({0,0,0}, 3.5f, 3.5f, 3.5f, heat);
            EndBulletTransform();
        }

        // Main bullet cube
        BeginBulletTransform(position, age, size);
        DrawCube({0,0,0}, size, size, size, heat);
        EndBulletTransform();
    }
}

bool Bullet::IsDone() const
{
    if (alive) return false; //keep updating and drawing particles

    if (type == BulletType::Fireball || type == BulletType::Iceball)
        return age >= 2.0f;

    // regular bullets: keep a short impact window
    return timeSinceImpact >= 0.25f;
}

bool Bullet::IsExpired() const {
    return alive;
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
    //smoke decals and bullet death
    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, camera.position));
    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

    //decals.emplace_back(offsetPos, DecalType::Smoke, R.GetTexture("smokeSheet"), 7, 0.8f, 0.1f, 25.0f);

    alive = false;
    exploded = true;
    
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

    // No damage if speed is below a threshold (tune this)
    if (speed < 800.0f) {
        return 0.0f;
    }

    // Protect against divide-by-zero or uninitialized shots
    float denom = (initialSpeed > 1.0f) ? initialSpeed : 1.0f;

    float vFactor = speed / denom;

    // clamp to [0,1]
    vFactor = Clamp(vFactor, 0.0f, 1.0f);

    return baseDamage * vFactor;
}


void Bullet::Explode(Camera& camera) {
    if (!alive) return; 
    if (type == BulletType::Default) return; //we were calling explode on default bullets some how. likely recycled bullets didn't get reset to defaults

    if (!explosionTriggered){
        explosionTriggered = true;
        velocity.x = 0; //stop bullets velocity when exploding but keep gravity. 
        velocity.z = 0;
        SoundManager::GetInstance().PlaySoundAtPosition("explosion", position, player.position, player.rotation.y, 3000.0f);
        
        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, camera.position));
        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, 100.0f));
        if (type == BulletType::Fireball){
            decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("explosionSheet"), 13, 1.0f, 0.1f, 500.0f);
            fireEmitter.EmitBurst(position, 200, ParticleType::Sparks);
            Vector3 forward = Vector3Negate(Vector3Normalize(velocity));
            ExplodeShrapnelSphere(position, 10, 1500.0f, 1.0f, false);
        }else if (type == BulletType::Iceball){
            
            fireEmitter.EmitBurst(position, 200, ParticleType::IceBlast);
        }

        float minDamage = 10;
        float maxDamage = 200;
        float explosionRadius = 500;
        //area damage
        if (type == BulletType::Fireball){
            for (Character* enemy : enemyPtrs){
                float dist = Vector3Distance(position, enemy->position);
                if (dist < explosionRadius) {
                    float dmg =  Lerp(maxDamage, minDamage, dist / explosionRadius);
                    enemy->TakeDamage(dmg);
                    
                }
            }
            //damage player if too close, 
            float pDamage = 50.0f;
            float pdist = Vector3Distance(player.position, position);
            if (pdist < explosionRadius){
                float dmg =  Lerp(pDamage, minDamage, pdist / explosionRadius); //damage fall off with distance.
                player.TakeDamage(dmg);//upto 50 percent health may be a lot for direct hit. 
            }

        }else if (type == BulletType::Iceball){
            for (Character* enemy : enemyPtrs){
                float dist = Vector3Distance(position, enemy->position);
                if (dist < explosionRadius) {
                    float dmg =  Lerp(maxDamage, minDamage, dist / explosionRadius);
                    enemy->ChangeState(CharacterState::Freeze);
                    enemy->currentHealth -= dmg; //dont call take damage, it triggers stagger which over rides freeze. 
                    //can cause invincible skeletons if they die to freeze, we check health again on freeze state for skeles not pirates.
                    

                }
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
        activeBullets.emplace_back(b);
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
    harpoon.lifeTime = 1.5f;
    activeBullets.emplace_back(harpoon);
}

void FireBlunderbuss(Vector3 origin, Vector3 forward, float spreadDegrees, int pelletCount, float speed, float lifetime, bool enemy) {
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
        if (b.initialSpeed < 1.0f) b.initialSpeed = 1.0f;
        b.id = gBulletCounter++;
        activeBullets.emplace_back(b);

    }
}

void FireBullet(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy) {
    Vector3 direction = Vector3Subtract(target, origin);
    direction = Vector3Normalize(direction);
    Vector3 velocity = Vector3Scale(direction, speed);
    Bullet b = {origin, velocity, lifetime, enemy};
    b.id = gBulletCounter++;
    activeBullets.emplace_back(b);
}

void FireFireball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy, bool launcher) {
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

    SoundManager::GetInstance().PlaySoundAtPosition((rand() % 2 == 0 ? "flame1" : "flame2"), origin, player.position, 0.0f, 3000.0f);

    
}

void FireIceball(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy) {
    Vector3 direction = Vector3Normalize(Vector3Subtract(target, origin));
    Vector3 velocity = Vector3Scale(direction, speed);

    Bullet& b = activeBullets.emplace_back(origin, velocity, lifetime, enemy, BulletType::Iceball, 20.0f);

    b.light.active     = true;
    b.light.color      = lightConfig.dynamicIceColor;
    b.light.range      = lightConfig.dynamicRange;
    b.light.intensity  = lightConfig.dynamicIntensity;
    b.light.detachOnDeath = true;
    b.light.lifeTime   = 0.15f; // short glow after death
    b.id = gBulletCounter++;
    SoundManager::GetInstance().Play("iceMagic");
}

