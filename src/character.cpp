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
    float halfWidth = (frameWidth * scale * 0.4f) / 2.0f;  // Only 40 percent the width of the frame
    float halfHeight = (frameHeight * scale) / 2.0f;

    return {
        { position.x - halfWidth, position.y - halfHeight, position.z - halfWidth },
        { position.x + halfWidth, position.y + halfHeight, position.z + halfWidth }
    };
}

void Character::playRaptorSounds(){
    int rn = GetRandomValue(1, 3);
    switch (rn){
    case 1:SoundManager::GetInstance().PlaySoundAtPosition("dinoTweet", position, player.position, player.rotation.y, 4000);break;
    case 2:SoundManager::GetInstance().PlaySoundAtPosition("dinoTweet2", position, player.position, player.rotation.y, 4000);break;
    case 3:SoundManager::GetInstance().PlaySoundAtPosition("dinoTarget", position, player.position, player.rotation.y, 4000);break;
    } 

}



void Character::TakeDamage(int amount) {
    if (isDead) return;

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

        hitTimer = 5.0f; //stay red for five seconds testing //0.5f
        currentHealth = 0;
        isDead = true;
        deathTimer = 0.0f;
        //state = CharacterState::Death;
        if (type == CharacterType::Ghost) SetAnimation(1, 7, 0.2);

        // Toward the *camera/player* in world space
        Vector3 toPlayer = Vector3Normalize(Vector3Subtract(player.position, position));
        Vector3 newPos   = Vector3Add(position, Vector3Scale(toPlayer, 100.0f)); // 100 units in front of the enemy
        if (type == CharacterType::Skeleton || type == CharacterType::Ghost) {
            bloodEmitter.EmitBlood(newPos, 20, WHITE);
        } else {
            bloodEmitter.EmitBlood(newPos, 20, RED);
        }

        ChangeState(CharacterState::Death);

        
        if (type != CharacterType::Spider)  SoundManager::GetInstance().Play("dinoDeath");
        if (type == CharacterType::Skeleton) SoundManager::GetInstance().Play("bones");
        if (type == CharacterType::Spider) SoundManager::GetInstance().Play("spiderDeath");
     
    } else {
        hitTimer = 0.5f; //tint red
        ChangeState(CharacterState::Stagger);
    
        if (canBleed){
            canBleed = false;
            // Toward the *camera/player* in world space
            Vector3 toPlayer = Vector3Normalize(Vector3Subtract(player.position, position));
            Vector3 newPos   = Vector3Add(position, Vector3Scale(toPlayer, 100.0f)); // 100 units in front of the enemy
            if (type == CharacterType::Skeleton || type == CharacterType::Ghost) {
                bloodEmitter.EmitBlood(newPos, 10, WHITE);
            } else {
                bloodEmitter.EmitBlood(newPos, 20, RED);
            }
        }
        //SetAnimation(4, 1, 1.0f); // Use first frame of death anim for 1 second. for all enemies
        

        AlertNearbySkeletons(position, 3000.0f);

        if (type == CharacterType::Pirate){
            SoundManager::GetInstance().Play("phit1");
        }else if (type == CharacterType::Spider){
            SoundManager::GetInstance().Play("spiderDeath");
        }else if (type == CharacterType::Raptor || type == CharacterType::Skeleton){
            SoundManager::GetInstance().PlaySoundAtPosition("dinoHit", position, player.position, 0.0f, 4000.0f); //raptor and skeletons
        }else if (type == CharacterType::Trex){
            SoundManager::GetInstance().Play(GetRandomValue(0, 1) == 0 ? "TrexHurt2" : "TrexHurt");
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



void Character::Update(float deltaTime, Player& player ) {
    if (isLoadingLevel) return;
    bloodEmitter.UpdateBlood(deltaTime);
 
    animationTimer += deltaTime;
    stateTimer += deltaTime;
    previousPosition = position;
    float groundY = GetHeightAtWorldPosition(position, heightmap, terrainScale); //get groundY from heightmap
    if (isDungeon) groundY = dungeonPlayerHeight;

    // Gravity
    float gravity = 980.0f; //we need gravity for outside maps so characters stick to heightmap.
    if (isDungeon) gravity = 0.0f; //no gravity in dungeons. floor is fixed height. 
    static float verticalVelocity = 0.0f;

    float spriteHeight = frameHeight * scale;

    if (position.y > groundY + spriteHeight / 2.0f) {
        verticalVelocity -= gravity * deltaTime;
        position.y += verticalVelocity * deltaTime;
    } else {
        verticalVelocity = 0.0f;
        position.y = groundY + spriteHeight / 2.0f;
    }
    
    //Run AI state machine depending on characterType
    UpdateAI(deltaTime,player);

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


    eraseCharacters(); //clean up dead enemies

}

// Show the character's back when moving away
static inline Vector3 XZ(const Vector3& v){ return {v.x, 0.0f, v.z}; }
static inline float   DotXZ(const Vector3& a, const Vector3& b){ return a.x*b.x + a.z*b.z; }
static inline float   LenSqXZ(const Vector3& v){ return v.x*v.x + v.z*v.z; }

void Character::UpdateLeavingFlag(const Vector3& playerPos)
{
    // Tunables
    constexpr float MIN_MOVE_EPS_SQ   = 100.0f; // how much squared motion counts as "moving" (units^2 per tick)
    constexpr int   STREAK_TO_FLIP    = 3;    // require N consistent frames to flip
    constexpr float DIST_EPS          = 1.0f; // tiny epsilon to ignore micro distance jitter

    // Current vectors (XZ plane)
    Vector3 toPlayer   = XZ( Vector3Subtract(playerPos, this->position) );
    Vector3 deltaMove  = XZ( Vector3Subtract(this->position, this->prevPos) );

    bool decisionMade = false;

    // 1) Prefer velocity-based decision if we actually moved
    if (LenSqXZ(deltaMove) > MIN_MOVE_EPS_SQ && LenSqXZ(toPlayer) > 0.0001f)
    {
        // If movement has a positive dot toward the player → approaching; negative → leaving
        const float d = DotXZ(deltaMove, toPlayer);
        if (d > 0.0f) {        // moving toward player
            approachStreak++;  leaveStreak = 0;
            if (approachStreak >= STREAK_TO_FLIP) { isLeaving = false; decisionMade = true; }
        } else if (d < 0.0f) { // moving away from player
            leaveStreak++;     approachStreak = 0;
            if (leaveStreak >= STREAK_TO_FLIP+10)  { isLeaving = true;  decisionMade = true; } //slightly more ticks to flip to isLeaving
        }
    }

    // 2) Fallback: if barely moving, use distance trend (prevents flicker at path corners)
    if (!decisionMade)
    {
        float curDist = sqrtf(LenSqXZ(toPlayer));
        if (prevDistToPlayer >= 0.0f)
        {
            float delta = curDist - prevDistToPlayer;
            if (delta > DIST_EPS) { // getting farther
                leaveStreak++;     approachStreak = 0;
                if (leaveStreak >= STREAK_TO_FLIP)  isLeaving = true;
            } else if (delta < -DIST_EPS) { // getting closer
                approachStreak++;  leaveStreak = 0;
                if (approachStreak >= STREAK_TO_FLIP) isLeaving = false;
            }
        }
        prevDistToPlayer = curDist;
    }

    // Bookkeeping
    prevPos = this->position;
}

AnimDesc Character::GetAnimFor(CharacterType type, CharacterState state) {
    switch (type) {
        case CharacterType::Trex:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                case CharacterState::Orbit:
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
                case CharacterState::Orbit: 
                    return AnimDesc{1, 5, 0.12f, true}; // walk

                
                case CharacterState::RunAway:
                case CharacterState::Patrol:
                    if (isLeaving){
                        return AnimDesc {3, 4, 0.12f, true};
                    }else{
                        return AnimDesc{1, 5, 0.12f, true}; // walk
                    }
                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return {0, 1, 1.0f, true};
                case CharacterState::Attack: return {2, 5, 0.2f, false};  
                case CharacterState::Stagger: return {4, 1, 1.0f, false}; // Use first frame of death anim for 1 second. for all enemies
                case CharacterState::Death:  return {4, 5, 0.2f, false};
                
                default:                     return {0, 1, 1.0f, true};
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

        case CharacterType::GiantSpider:
            switch (state) {
                case CharacterState::Chase:
                case CharacterState::Patrol:
                case CharacterState::Reposition:
                case CharacterState::RunAway: 
                    if (isLeaving){
                        return AnimDesc {3, 4, 0.25f, true}; //run away
                    }else{
                        return AnimDesc{1, 5, 0.2f, true}; // walk
                    }

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
                    return AnimDesc{1, 4, 0.2f, true}; // walk

                case CharacterState::Patrol:
                    // Choose row depending on whether the pirate is walking toward or away
                    return isLeaving
                        ? AnimDesc{5, 4, 0.25f, true}   // walking away (show back)
                        : AnimDesc{1, 4, 0.25f, true};  // walking toward (normal walk)

                case CharacterState::Freeze: return {0, 1, 1.0f, true};
                case CharacterState::Idle:   return     {0, 1, 1.0f, true};
                case CharacterState::Attack: return     {2, 4, 0.2f, false}; // ranged attack = attack
                case CharacterState::MeleeAttack: return{3, 5, 0.12f, false};
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
        case CharacterState::Orbit:
            return true;
        default:
            return false;
    }
}

void Character::ChangeState(CharacterState next) {
    if (state == next) return;  // no spam

    // Auto-flush path when transitioning from a path-using state to a non-path state. 
    //bool clearPath = StateUsesPath(state) && !StateUsesPath(next);
    state = next;
    stateTimer = 0.0f;

    //if (clearPath) currentWorldPath.clear(); testing with this off to see if it fixes a bug where skeletons stop chasing and just stand there. 
    //is does fix the bug. clearing the path is causing enemies to stop and remain idle sometimes instead of chasing. 
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
