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


Color BulletHeatColor(float t)
{
    // key colors
    Color hot   = {255, 255, 255, 255}; // bright yellowish white
    Color orange= {255, 160,  40, 200};
    Color yellow   = {255,  255,  0, 255};
    Color gray =  {200, 200, 200, 100};
    Color gone  = {220,  200,  200,   0}; // same hue, alpha 0

    if (t < 0.33f) {
        return LerpColor(hot, yellow, t / 0.33f);
    } else if (t < 0.66f) {
        return LerpColor(yellow, gray, (t - 0.33f) / 0.33f);
    } else {
        return LerpColor(gray, gone, (t - 0.66f) / 0.34f);
    }
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
            TryBulletRicochet(*this, n);

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
            TryBulletRicochet(*this, n);

            return;
        }
    } else {
        // Overworld: terrain height varies; sampling each frame is fine.
        float h = GetHeightAtWorldPosition(position, heightmap, terrainScale);
        if (prevPosition.y > h && position.y <= h) {
            Vector3 n = {0, 1, 0};
            TryBulletRicochet(*this, n);

            return;
        }
    }

}



void Bullet::Update(Camera& camera, float deltaTime) {
    fireEmitter.Update(deltaTime);
    sparkEmitter.Update(deltaTime); 

    if (!alive){
        timeSinceImpact += deltaTime;
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
    }


    // Standard bullet movement (non-fireball)
    if (!IsEnemy() && type == BulletType::Default){
        fireEmitter.UpdateTrail(deltaTime); //smoke trail update
        velocity.y -= gravity * deltaTime;
        fireEmitter.SetParticleType(ParticleType::Smoke);
        fireEmitter.SetPosition(position);
        fireEmitter.SetParticleSize(1.0f);
        fireEmitter.SetEmissionRate(4.0f);

    }

    prevPosition = position;
    position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
    age += deltaTime;

    if (age >= maxLifetime && !exploded) alive = false;


    HandleBulletWorldCollision(camera);

    float speed = Vector3Length(velocity);
    if (speed < 800.0f) {   //  ~ one bounce theoretically
        alive = false;
        exploded = true;
        
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
            
            
   
    } else{ //regular bullets
        float t = Clamp(age / 1.0, 0.0f, 1.0f);
        Color heat = BulletHeatColor(t);//shrapnel cools off over time

        if (age < 0.25) DrawSphere(position, 3.5f, heat); //make the bullets pop more, with a sphere to simulate glowing flack. 
        DrawCube(position, 3, 3, 3, heat); 
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


void Bullet::Blood(Camera& camera){
    //spawn blood decals at bullet position, if it's not a skeleton.
    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, camera.position));
    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, 100.0f));

    decals.emplace_back(offsetPos, DecalType::Blood, R.GetTexture("bloodSheet"), 7, 1.0f, 0.1f, 60.0f);

    alive = false;

}

float Bullet::ComputeDamage()
{
    float speed = Vector3Length(velocity);

    // No damage if speed is below a threshold (tune this)
    if (speed < 1000.0f) {
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
        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));
        if (type == BulletType::Fireball){
            decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("explosionSheet"), 13, 1.0f, 0.1f, 500.0f);
            fireEmitter.EmitBurst(position, 200, ParticleType::Sparks);

        }else if (type == BulletType::Iceball){
            
            fireEmitter.EmitBurst(position, 200, ParticleType::IceBlast);
        }

        float minDamage = 10;
        float maxDamage = 200;
        float explosionRadius = 200;
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

        b.initialSpeed = Vector3Length(b.velocity); //set initial speed. Why cant we just use max speed again? 
        if (b.initialSpeed < 1.0f) b.initialSpeed = 1.0f;

        activeBullets.emplace_back(b);

    }
}

void FireBullet(Vector3 origin, Vector3 target, float speed, float lifetime, bool enemy) {
    Vector3 direction = Vector3Subtract(target, origin);
    direction = Vector3Normalize(direction);
    Vector3 velocity = Vector3Scale(direction, speed);

    activeBullets.emplace_back(origin, velocity, lifetime, enemy);
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

    SoundManager::GetInstance().Play("iceMagic");
}

