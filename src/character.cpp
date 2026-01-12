#include "character.h"
#include "raymath.h"
#include <iostream>
#include "rlgl.h"
#include "world.h"
#include "algorithm"
#include "sound_manager.h"
#include "player.h"
#include "dungeonGeneration.h"
#include "pathfinding.h"
#include "resourceManager.h"
#include "utilities.h"
#include "heightmapPathfinding.h"

//Character raptor(spawnPos, R.GetTexture("raptorTexture"), 200, 200, 1, 0.5f, 0.5f, 0, CharacterType::Raptor);

Character::Character(Vector3 pos, Texture2D& tex, int fw, int fh, int frames, float speed, float scl, int row, CharacterType t)
    : position(pos),
      texture(tex),
      frameWidth(fw),
      frameHeight(fh),
      currentFrame(0),
      maxFrames(frames),
      rowIndex(row),
      animationTimer(0),
      animationSpeed(speed),
      scale(scl),
      type(t) {}



BoundingBox Character::GetBoundingBox() const {
    float halfWidth  = (frameWidth  * scale * 0.4f) / 2.0f; //40 percent of the width centered

    float heightScale = 1.0f;

    // The spider billboard only occupies the bottom half of its frame.
    if (type == CharacterType::Spider) {
        heightScale = 0.5f;
    }

    float halfHeight = (frameHeight * scale * heightScale) / 2.0f;

    return {
        { position.x - halfWidth, position.y - halfHeight, position.z - halfWidth },
        { position.x + halfWidth, position.y + halfHeight, position.z + halfWidth }
    };
}

void Character::playRaptorSounds()
{
    if (raptorSoundCooldown > 0.0f) return;

    // 1.5–3 second cooldown randomization for variety
    raptorSoundCooldown = GetRandomValue(1500, 3000) / 1000.0f;

    int rn = GetRandomValue(1, 3);

    auto& sm = SoundManager::GetInstance();
    switch (rn){
        case 1: sm.PlaySoundAtPosition("dinoTweet", position, player.position, player.rotation.y, 4000); break;
        case 2: sm.PlaySoundAtPosition("dinoTweet2", position, player.position, player.rotation.y, 4000); break;
        case 3: sm.PlaySoundAtPosition("dinoTarget", position, player.position, player.rotation.y, 4000); break;
    }
}

Vector3 Character::GetFeetPos() const {
    return { position.x, position.y - spriteHeight * 0.5f, position.z };
}

void Character::SetFeetPos(const Vector3& feet) {
    position.x = feet.x;
    position.z = feet.z;
    position.y = feet.y + spriteHeight * 0.5f;
}

void Character::ApplyGroundSnap()
{
    // Get current feet pos
    Vector3 feet = GetFeetPos();

    // Sample the terrain height at this XZ
    float groundY = GetHeightAtWorldPosition(feet, heightmap, terrainScale);
    if (isDungeon) groundY = floorHeight;

    feet.y = groundY;
    SetFeetPos(feet);

}

void Character::TakeDamage(int amount) {
    if (isDead) return;
    if (amount <= 0) return;
    
    currentHealth -= amount;

    accumulateDamage += amount;
    if (accumulateDamage >= 200 && type == CharacterType::GiantSpider){
        //run away until you take 200 damage
        if (!spiderAgro){
            spiderAgro = true;
            accumulateDamage = 0.0f;

        }
    }
    if (accumulateDamage >= 400 && type == CharacterType::GiantSpider){
        if (spiderAgro){
            spiderAgro = false;
            accumulateDamage = 0.0f;

        }
    }

    if (currentHealth <= 0) { //die

        hitTimer = 1.0f; //stay red for 1 second on death. 
        currentHealth = 0;
        isDead = true;
        deathTimer = 0.0f;
        if (type == CharacterType::GiantSpider && levelIndex == 8) OpenEventLockedDoors(); //open boss door when spider dies. abstract this 

        if (type == CharacterType::Ghost) SetAnimation(1, 7, 0.2);

        // Toward the *camera/player* in world space
        Vector3 toPlayer = Vector3Normalize(Vector3Subtract(player.position, position));
        Vector3 newPos   = Vector3Add(position, Vector3Scale(toPlayer, 100.0f)); // 100 units in front of the enemy
        if (type == CharacterType::Skeleton || type == CharacterType::Ghost) {
            bloodEmitter.EmitBlood(newPos, 20, WHITE);
        } else {
            bloodEmitter.EmitBlood(newPos, 20, RED);

            Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
            Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, 50.0f));
            offsetPos.y += 10; 
            decals.emplace_back(offsetPos, DecalType::Blood, R.GetTexture("bloodSheet"), 8, 0.7f, 0.07f,128.0f);
        }

        ChangeState(CharacterState::Death);

        
        if (type != CharacterType::Spider)  SoundManager::GetInstance().PlaySoundAtPosition("dinoDeath", position, player.position, 0.0f, 3000);
        if (type == CharacterType::Skeleton) SoundManager::GetInstance().PlaySoundAtPosition("bones", position, player.position, 0.0f, 3000);
        if (type == CharacterType::Spider) SoundManager::GetInstance().PlaySoundAtPosition("spiderDeath", position, player.position, 0.0f, 3000);
     
    } else {
        hitTimer = 0.5f; //tint red
        

        if (canBleed){
            canBleed = false;
            // Toward the *camera/player* in world space
            Vector3 toPlayer = Vector3Normalize(Vector3Subtract(player.position, position));
            Vector3 newPos   = Vector3Add(position, Vector3Scale(toPlayer, 25.0f)); // 25 units in front of the enemy
            if (type == CharacterType::Skeleton || type == CharacterType::Ghost) {
                bloodEmitter.EmitBlood(newPos, 10, WHITE);
            } else {
                bloodEmitter.EmitBlood(newPos, 20, RED);
            }
        }
        //SetAnimation(4, 1, 1.0f); // Use first frame of death anim for 1 second. for all enemies
        
        
        AlertNearbySkeletons(position, 3000.0f);

        if (type == CharacterType::Pirate){
            SoundManager::GetInstance().PlaySoundAtPosition("phit1", position, player.position, 0.0f, 3000);
        }else if (type == CharacterType::Spider){
            SoundManager::GetInstance().PlaySoundAtPosition("spiderDeath", position, player.position, 0.0f, 3000);
        }else if (type == CharacterType::Raptor || type == CharacterType::Skeleton){
            SoundManager::GetInstance().PlaySoundAtPosition("dinoHit", position, player.position, 0.0f, 4000.0f); //raptor and skeletons
        }else if (type == CharacterType::Trex){
            SoundManager::GetInstance().PlaySoundAtPosition(GetRandomValue(0, 1) == 0 ? "TrexHurt2" : "TrexHurt", position, player.position, 0.0f, 3000);
        }else if (type == CharacterType::GiantSpider){
            SoundManager::GetInstance().PlaySoundAtPosition("spiderDeath", position, player.position, 0.0f, 3000);
        }

        //dont stagger if your frozen. 
        if (state != CharacterState::Freeze){
            ChangeState(CharacterState::Stagger); //stagger last
        }

        
    }
}


void Character::eraseCharacters() {
    // Remove dead enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Character& e) {
            return e.isDead && e.deathTimer > 5.0f;
        }),
        enemies.end());

    // Rebuild enemyPtrs
    enemyPtrs.clear();
    for (auto& e : enemies) {
        enemyPtrs.push_back(&e);
    }
}

void Character::UpdateAltitude(float dt, float groundY, float desiredAltitude)
{
    // desiredAltitude is “meters above ground”
    const float MAX_CLIMB_SPEED = 800.0f;   // units/sec
    const float MAX_CLIMB_ACCEL = 2000.0f;  // units/sec^2
    const float MAX_DIVE_SPEED  = 2200.0f;  // going down (much faster)
    const float MAX_DIVE_ACCEL = 6000.0f;

    float targetY = groundY + desiredAltitude;

    float dy = targetY - position.y;

    // Desired vertical velocity (simple P controller, capped)
    // float desiredVY = Clamp(dy * 2.0f, -MAX_CLIMB_SPEED, MAX_CLIMB_SPEED);
    float desiredVY = dy * 2.0f;

    if (desiredVY > 0)
        desiredVY = Clamp(desiredVY, 0.0f, MAX_CLIMB_SPEED);
    else
        desiredVY = Clamp(desiredVY, -MAX_DIVE_SPEED, 0.0f);

    // Accelerate verticalVel toward desiredVY
    //float ay = Clamp(desiredVY - verticalVel, -MAX_CLIMB_ACCEL, MAX_CLIMB_ACCEL);
    float maxAccel = (desiredVY < verticalVel) ? MAX_DIVE_ACCEL : MAX_CLIMB_ACCEL;
    float ay = Clamp(desiredVY - verticalVel, -maxAccel, maxAccel);
    verticalVel += ay * dt;
    position.y  += verticalVel * dt;

    // optional: dead-zone stop
    if (fabsf(dy) < 5.0f && fabsf(verticalVel) < 5.0f)
    {
        position.y = targetY;
        verticalVel = 0.0f;
    }
}




void Character::Update(float deltaTime, Player& player ) {
    if (isLoadingLevel) return;
    bloodEmitter.UpdateBlood(deltaTime);
    
    animationTimer += deltaTime;
    stateTimer += deltaTime;
    raptorSoundCooldown -= deltaTime;
    if (raptorSoundCooldown < 0) raptorSoundCooldown = 0;

    spriteHeight = frameHeight * scale;
    if (!isDungeon && type != CharacterType::Pterodactyl) ApplyGroundSnap();

    float groundY = GetHeightAtWorldPosition(position, heightmap, terrainScale); //get groundY from heightmap
    if (type == CharacterType::Pterodactyl){
        if (GetFeetPos().y < groundY) SetFeetPos(Vector3 {position.x, groundY, position.z}); //prevent dactyls from going underground
    }


    if (isDungeon)
    {
        // 2) If standing over lava, define the lava "sink floor"
        if (overLava || overVoid)
        {
            float lavaSinkDepth = overVoid ? 1200.0f : 150.0f;
            const float lavaFallSpeed = 600.0f;
            const int   lavaDamage    = 500;

            float lavaY = dungeonPlayerHeight - lavaSinkDepth;

            // 3) Move feet down toward lavaY (clamped, no overshoot)
            float feetY = GetFeetPos().y;

            if (feetY > lavaY)
            {
                float step = lavaFallSpeed * deltaTime;
                position.y -= step;

                // recompute after move
                feetY = GetFeetPos().y;

                if (overVoid && feetY <= dungeonPlayerHeight - 200.0f && !lavaDamageApplied){
                    TakeDamage(lavaDamage);
                    lavaDamageApplied = true;
                }

                // 4) Trigger damage once when we cross / reach lavaY
                if (!lavaDamageApplied && feetY <= lavaY)
                {
                    TakeDamage(lavaDamage);
                    lavaDamageApplied = true;
                }
            }

            // While over lava, we don't want normal floor snapping
            //floorY = lavaY;
        }
        else
        {
            // Reset when not over lava so it can happen again later
            lavaDamageApplied = false;
        }
    }

    
    //Run AI state machine depending on characterType
    UpdateAI(deltaTime,player);

    if (type == CharacterType::Bat && deathTimer <= 0.0f){
        // Advance phase
        bobPhase += bobSpeed * deltaTime;

        // Keep it bounded (optional but tidy)
        if (bobPhase > 2.0f * PI)
            bobPhase -= 2.0f * PI;

        // Compute bob offset
        float bobOffset = sinf(bobPhase) * bobAmplitude;

        // Final vertical position
        if (isDead){
            position.y -= 100 * deltaTime;
        }else{
            position.y = groundY + hoverHeight + bobOffset;
        }

        



    }

    // Advance animation frame
    if (animationTimer >= animationSpeed) {
        animationTimer = 0;


        if (state == CharacterState::Death) {
            if (currentFrame < maxFrames - 1) {
                currentFrame++;
            }
            // else do nothing — stay on last frame
        } else {
            currentFrame = (currentFrame + 1) % maxFrames; //loop
        }
    }

    if (hitTimer > 0.0f){
        hitTimer -= deltaTime;
    }else{
        hitTimer = 0.0f;
    }

    previousPosition = position;
    eraseCharacters(); //clean up dead enemies

}


// sprite sheet layout – adjust these to match your art
constexpr int ROW_RUN_FRONT = 1;  // toward camera
constexpr int ROW_RUN_SIDE  = 5;  // side profile
constexpr int ROW_RUN_BACK  = 3;  // away/runaway

void Character::UpdateMovementAnim()
{
    // Call this once per frame after you've updated facingMode

    switch (facingMode)
    {
        case FacingMode::Leaving: {
            // back / runaway row
            if (rowIndex != ROW_RUN_BACK) {
                SetAnimation(ROW_RUN_BACK, 4, 0.25f, true); // 4 frames, tweak speed
            }
        } break;

        case FacingMode::Approaching: {
            // front / approach row
            if (rowIndex != ROW_RUN_FRONT) {
                SetAnimation(ROW_RUN_FRONT, 5, 0.15f, true); // 5 frames, tweak speed
            }
        } break;

        case FacingMode::Strafing: {
            // side profile row
            if (rowIndex != ROW_RUN_SIDE) {
                SetAnimation(ROW_RUN_SIDE, 4, 0.15f, true); 
            }

    
        } break;
    }
}


// Show the character's back when moving away (robust vs player backpedal)
static inline Vector3 XZ(const Vector3& v){ return {v.x, 0.0f, v.z}; }
static inline float   DotXZ(const Vector3& a, const Vector3& b){ return a.x*b.x + a.z*b.z; }
static inline float   LenSqXZ(const Vector3& v){ return v.x*v.x + v.z*v.z; }

void Character::UpdateLeavingFlag(const Vector3& playerPos, const Vector3& playerPrevPos)
{
    // Tunables
    constexpr float MIN_REL_MOVE_EPS_SQ = 4.0f;   // how much relative motion counts as "moving"
    constexpr int   STREAK_TO_APPROACH  = 3;      // frames to confirm approaching
    constexpr int   STREAK_TO_LEAVE     = 30;     // frames to confirm leaving
    constexpr int   STREAK_TO_STRAFE    = 1;      // frames to confirm strafe (tweak)
    constexpr float DIST_EPS            = 1.0f;
    constexpr float NEAR_CONTACT        = 300.0f;

    // Angle thresholds

    constexpr float COS = 0.8f;  //Higher to show side more often
    
    // Current vectors (XZ plane)
    Vector3 r          = XZ( Vector3Subtract(playerPos, this->position) );   // enemy -> player
    Vector3 enemyStep  = XZ( Vector3Subtract(this->position, this->prevPos) );
    Vector3 playerStep = XZ( Vector3Subtract(playerPos,        playerPrevPos) );
    Vector3 relStep    = XZ( Vector3Subtract(enemyStep,        playerStep) );// enemy relative to player

    const float rLenSq    = LenSqXZ(r);
    const float relLenSq  = LenSqXZ(relStep);



    bool decided = false;

    // 0) Near-contact guard: still useful if you want to bias away from "leaving"
    if (rLenSq < NEAR_CONTACT*NEAR_CONTACT) {
        // At very close range, we’ll pick between Approaching / Strafing only
        // based on angle, but *not* Leaving.
        // So don't early-decide here, just remember we're close.
    }

    // 1) Relative-motion test (primary)
    if (relLenSq > MIN_REL_MOVE_EPS_SQ && rLenSq > 1e-6f)
    {
        Vector3 rN    = NormalizeXZ(r);
        Vector3 relN  = NormalizeXZ(relStep);



        float d = DotXZ(rN, relN);   // cos(theta) between enemy->player and relative motion

        FacingMode candidate;

        // classify by angle first
        if (d >  COS) {
            candidate = FacingMode::Approaching;
        } else if (d < -COS) {
            candidate = FacingMode::Leaving;
        } else {
            candidate = FacingMode::Strafing;
        }


            // only if we actually picked Strafing:
        if (candidate == FacingMode::Strafing) {
            // 2D cross product on XZ
            float cross = rN.x * relN.z - rN.z * relN.x;
            strafeSideSign = (cross >= 0.0f) ? 1.0f : -1.0f;
        }

        // Optional bias: if very close, don't ever show "Leaving"
        if (rLenSq < NEAR_CONTACT*NEAR_CONTACT && candidate == FacingMode::Leaving) {
            candidate = FacingMode::Approaching;  // or FacingMode::Strafing, your call
        }

        // Streak logic for smoothing
        switch (candidate)
        {
            case FacingMode::Approaching:
                approachStreak++;
                leaveStreak = 0;
                strafeStreak = 0;
                if (approachStreak >= STREAK_TO_APPROACH) {
                    facingMode = FacingMode::Approaching;
                    decided = true;
                }
                break;

            case FacingMode::Leaving:
                leaveStreak++;
                approachStreak = 0;
                strafeStreak = 0;
                if (leaveStreak >= STREAK_TO_LEAVE) {
                    facingMode = FacingMode::Leaving;
                    decided = true;
                }
                break;

            case FacingMode::Strafing:
                strafeStreak++;
                approachStreak = 0;
                leaveStreak = 0;
                if (strafeStreak >= STREAK_TO_STRAFE) {
                    facingMode = FacingMode::Strafing;
                    decided = true;
                }
                break;
        }
    }

    // 2) Fallback: tiny-motion distance trend (rare)
    if (!decided)
    {
        float curDist = sqrtf(rLenSq);
        if (prevDistToPlayer >= 0.0f)
        {
            float delta = curDist - prevDistToPlayer;
            if (delta > DIST_EPS) {
                // separating -> likely leaving
                leaveStreak++; approachStreak = 0; strafeStreak = 0;
                if (leaveStreak >= STREAK_TO_LEAVE) facingMode = FacingMode::Leaving;
            } else if (delta < -DIST_EPS) {
                // closing -> approaching
                approachStreak++; leaveStreak = 0; strafeStreak = 0;
                if (approachStreak >= STREAK_TO_APPROACH) facingMode = FacingMode::Approaching;
            }
        }
        prevDistToPlayer = curDist;
    }

    // Bookkeeping
    prevPos = this->position;

    // Backwards compat: keep isLeaving in sync for any existing code
    isLeaving = (facingMode == FacingMode::Leaving ||
                facingMode == FacingMode::Strafing);
}


void Character::BuildPathToPlayer()
{
    navPath.clear();
    navPathIndex   = -1;
    navHasPath     = false;

    if (!hasIslandNav) return;

    std::vector<Vector3> path;
    bool ok = HeightmapPathfinding::FindPathBFS(
        gIslandNav,
        position,
        player.position,
        terrainScale.y,
        path
    );


    if (!ok || path.empty())
    {
        return; // fall back to direct Arrive chase
    }

    navPath      = std::move(path);
    navPathIndex = 0;
    navHasPath   = true;
    navRepathTimer = 0.0f;
}


AnimDesc Character::GetAnimFor(CharacterType type, CharacterState state) {
    switch (type) {
        case CharacterType::Trex:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                    return AnimDesc {1, 4, 0.25, true}; //walk

                case CharacterState::RunAway: return {1, 4, 0.25f, true};
                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 3, 1.0f, false};  // 4 * 0.2 = 0.8s
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 4, 0.25f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }
        case CharacterType::Raptor:
            switch (state) {

                case CharacterState::Chase:
                case CharacterState::Reposition:
                case CharacterState::RunAway:
                case CharacterState::Patrol:
                    return AnimDesc{1, 5, 0.15f, true}; // walk

                case CharacterState::Freeze: 
                case CharacterState::Stagger: 
                    return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies

                case CharacterState::Idle: return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 4, 0.2f, false};  
                case CharacterState::Death:  return {4, 5, 0.2f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }


        case CharacterType::Pterodactyl:
            switch (state) {

                case CharacterState::Chase:
                case CharacterState::Reposition:
                case CharacterState::RunAway:
                case CharacterState::Patrol:
                    return AnimDesc{1, 5, 0.25, true}; // walk

                case CharacterState::Freeze: 
                case CharacterState::Stagger: 
                    return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies

                case CharacterState::Idle: return {0, 5, 0.2f, true};
                case CharacterState::Attack: return {2, 5, 0.2f, false};  
                case CharacterState::Death:  return {4, 5, 0.2f, false};
                
                default:                     return {0, 5, 0.2f, true};
            }

        case CharacterType::Skeleton:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 4, 0.2f, false};  // 4 * 0.2 = 0.8s
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 4, 0.2f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }

        case CharacterType::Spider:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 4, 0.2f, false};  // 4 * 0.2 = 0.8s
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 3, 0.5f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }


        case CharacterType::Bat:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 5, 0.2f, true};
                case CharacterState::Attack: return {2, 5, 0.2f, false};  // 4 * 0.2 = 0.8s
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 5, 0.2f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }

        case CharacterType::GiantSpider:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                case CharacterState::RunAway: 
                    return AnimDesc{1, 5, 0.2f, true}; // walk
                
                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 4, 0.2f, false};  // 4 * 0.2 = 0.8s
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 5, 0.25f, false};
                
                default:                     return {0, 1, 1.0f, true};
            }

        case CharacterType::Ghost:
            // fill with whatever you want the ghost to show per state
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                    return AnimDesc{0, 7, 0.12f, true}; // walk
                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 7, 0.2f, true};
                case CharacterState::Attack: return {0, 7, 0.12f, false}; //faster
                case CharacterState::Stagger: return {0, 1, 1.0f, false};
                case CharacterState::Death:  return {1, 7, 0.2, false };
                default:                     return {0, 7, 0.2f, true};
            }

        case CharacterType::Pirate:
          
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Reposition:
                case CharacterState::Patrol:
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Freeze: return     {0, 1, 1.0f, true};
                case CharacterState::Idle:   return     {0, 1, 1.0f, true};
                case CharacterState::Attack: return     {2, 4, 0.2f, false}; // ranged attack = attack
                case CharacterState::MeleeAttack: return{6, 5, 0.12f, false};
                case CharacterState::Stagger: return    {4, 1, 1.0f, false};
                case CharacterState::Death:  return     {4, 3, 0.25f, false };
                default:                     return     {0, 7, 0.2f, true};
            }

        case CharacterType::Wizard:
          
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Reposition:
                case CharacterState::Patrol:
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Freeze: return     {0, 1, 1.0f, true};
                case CharacterState::Idle:   return     {0, 1, 1.0f, true};
                case CharacterState::Attack: return     {2, 4, 0.2f, false}; // ranged attack = attack
                case CharacterState::MeleeAttack: return{6, 5, 0.12f, false};
                case CharacterState::Stagger: return    {4, 1, 1.0f, false};
                case CharacterState::Death:  return     {4, 3, 0.25f, false };
                default:                     return     {0, 7, 0.2f, true};
            }
        
        default:
            return {0, 1, 1.0f, true};
    }
}

// Which states actually use a nav path?
static inline bool StateUsesPath(CharacterState s) {
    switch (s) {
        case CharacterState::Chase:
        case CharacterState::Patrol:
        case CharacterState::Reposition:
            return true;
        default:
            return false;
    }
}

void Character::ChangeState(CharacterState next) {
    if (state == next) return;  // no spam
    state = next;
    stateTimer = 0.0f;

    if (type == CharacterType::Raptor && state == CharacterState::Chase){
        chaseDuration = GetRandomValue(10, 20);
        playRaptorSounds(); //play a random tweet when switching to chase. 
    }

    if (type == CharacterType::Trex && state == CharacterState::Chase){
        chaseDuration = GetRandomValue(10, 20);

    }

    if (state == CharacterState::Attack) attackCooldown = 0.0f;

    const AnimDesc a = GetAnimFor(type, state);
    SetAnimation(a.row, a.frames, a.frameTime, a.loop);
}



void Character::SetAnimation(int row, int frames, float speed, bool loop) {
    rowIndex = row;
    maxFrames = frames;
    animationSpeed = speed;
    currentFrame = 0;
    animationTimer = 0;
    animationLoop = loop;


}
