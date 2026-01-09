#include "transparentDraw.h"
#include "raylib.h"
#include "character.h"
#include "decal.h"
#include "dungeonGeneration.h"
#include "raymath.h"
#include "world.h"
#include "resourceManager.h"
#include "vector"
#include "rlgl.h"
#include "algorithm"
#include "spiderEgg.h"

std::vector<BillboardDrawRequest> billboardRequests;

float GetAdjustedBillboardSize(float baseSize, float distance) {
    //billboard scalling is way to extreme because of the size of the world. compensate by enlarging it a tiny bit depending on distance. 
    const float compensationFactor = 0.0001f;

    // Boost size as distance grows
    return baseSize * (1.0f + distance * compensationFactor);
}

void GatherGrapplePoint(Camera& camera) {
    for (GrapplePoint& g : grapplePoints){
        float dist = Vector3Distance(camera.position, g.position);

        billboardRequests.push_back({
            Billboard_FacingCamera, 
            g.position,
            g.tex,
            Rectangle{0, 0, (float)g.tex.width, (float)g.tex.height},
            g.scale,
            WHITE,
            dist,
            1.0f,
            false,
            false,
            false
        });
    }
}

void GatherEnemies(Camera& camera) {
    for (Character* enemy : enemyPtrs) {
        if (enemy->isDead && enemy->deathTimer <= 0.0f) continue;

        float dist = Vector3Distance(camera.position, enemy->position);

        Rectangle sourceRect = {
            (float)(enemy->currentFrame * enemy->frameWidth),
            (float)(enemy->rowIndex     * enemy->frameHeight),
            (float) enemy->frameWidth,
            (float) enemy->frameHeight
        };

        // Decide flipping for strafing, Raptors and pirates. 
        bool flipX = false;
        if (enemy->type == CharacterType::Raptor || enemy->type == CharacterType::Pirate || enemy->type == CharacterType::Pterodactyl || enemy->type == CharacterType::Wizard){
            if (enemy->facingMode == FacingMode::Strafing) {
                flipX = (enemy->strafeSideSign < 0.0f);
            }else{
                flipX = false;
            }

        }

        if (enemy->hitTimer > 0.0f) flipX = false; //dont flipX when taking damage. 

        // offset to prevent z-fighting
        Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.position, enemy->position));
        Vector3 offsetPos = Vector3Add(enemy->position, Vector3Scale(camDir, 10.0f));

        float billboardSize = GetAdjustedBillboardSize(enemy->frameWidth * enemy->scale, dist);

        Color finalTint = WHITE;
        if (enemy->hitTimer > 0.0f) finalTint = {255,50,50,255};
        if (enemy->state == CharacterState::Freeze) finalTint = SKYBLUE;

        billboardRequests.push_back({
            Billboard_FacingCamera,
            offsetPos,
            enemy->texture,
            sourceRect,
            billboardSize,
            finalTint,
            dist,
            0.0f,
            flipX,     // <----- added
            false,
            false
        });
    }
}


void GatherCollectables(Camera& camera, const std::vector<Collectable>& collectables) {
    for (const Collectable& c : collectables) {
        float dist = Vector3Distance(camera.position, c.position);

        billboardRequests.push_back({
            Billboard_FacingCamera, 
            c.position,
            c.icon,
            Rectangle{0, 0, (float)c.icon.width, (float)c.icon.height},
            c.scale,
            WHITE,
            dist,
            0.0f,
            false,
            false,
            false
        });
    }
}

void GatherSpiderEggDrawRequests(Camera& camera)
{
    for (SpiderEgg& egg : eggs) {
        float dist = Vector3Distance(camera.position, egg.position);
        Rectangle src {
            (float)(egg.currentFrame * egg.frameWidth),
            (float)(egg.currentRow   * egg.frameHeight),
            (float)egg.frameWidth,
            (float)egg.frameHeight
        };

        BillboardDrawRequest req {};
        req.type      = BillboardType::Billboard_FacingCamera;
        req.position  = egg.position;
        req.texture   = egg.texture;
        req.sourceRect = src;
        req.size      = egg.frameWidth * egg.scale;
        req.tint      = (egg.hitFlashTimer > 0 ? RED : WHITE);
        req.rotationY = egg.rotationY;
        req.isPortal  = false;
        req.distanceToCamera = dist;

        billboardRequests.push_back(req);
    }
}



void GatherDungeonFires(Camera& camera, float deltaTime) {
    for (size_t i = 0; i < pillars.size(); ++i) {
        PillarInstance& pillar = pillars[i];
        Fire& fire = fires[i];

        // Animate fire
        fire.fireAnimTimer += deltaTime;
        if (fire.fireAnimTimer >= fire.fireFrameDuration) {
            fire.fireAnimTimer -= fire.fireFrameDuration;
            fire.fireFrame = (fire.fireFrame + 1) % 60;
        }

        // Compute source rect
        int frameX = fire.fireFrame % 10;
        int frameY = fire.fireFrame / 10;
        Rectangle sourceRect = {
            (float)(frameX * 64),
            (float)(frameY * 64),
            64.0f,
            64.0f
        };

        // Compute fire position
        Vector3 firePos = pillar.position;
        firePos.y += 130;

        // Add to billboard requests
        float dist = Vector3Distance(camera.position, firePos);
        billboardRequests.push_back({
            Billboard_FacingCamera,
            firePos,
            R.GetTexture("fireSheet"),
            sourceRect,
            100.0f,
            WHITE,
            dist,
            0.0f,
            false,
            false,
            false
        });
    }
}

void GatherWebs(Camera& camera) {
    for (const SpiderWebInstance& web : spiderWebs) {
        if (!isDungeon) return;
        //if (web.destroyed && !web.showBrokeWebTexture) continue;

        Texture2D tex = web.destroyed ? R.GetTexture("brokeWebTexture") : R.GetTexture("spiderWebTexture");

        billboardRequests.push_back({
            Billboard_FixedFlat,
            web.position,
            tex,
            Rectangle{0, 0, (float)tex.width, (float)tex.height},
            400.0f,
            WHITE,
            Vector3Distance(camera.position, web.position),
            web.rotationY,
            false,
            false,
            false
        });
    }
}

void GatherDoors(Camera& camera) {
    for (const Door& door : doors) {
        const float width = door.scale.x;
        const float halfW = width * 0.5f;

        Vector3 up = { 0, 1, 0 };

        // Closed basis
        Vector3 forwardClosed = Vector3RotateByAxisAngle({0, 0, 1}, up, door.rotationY);
        Vector3 rightClosed   = Vector3CrossProduct(up, forwardClosed);

        // center is door.position (bottom middle, like in your original quad code)
        Vector3 center = door.position;

        // bottom-left corner in CLOSED pose: center − right * halfWidth
        Vector3 bottomLeftClosed = Vector3Add(center, Vector3Scale(rightClosed, -halfW));

        billboardRequests.push_back({
            Billboard_Door,
            bottomLeftClosed,   // <-- store the *hinge corner*
            door.doorTexture,
            Rectangle{ 0, 0, (float)door.doorTexture.width, (float)door.doorTexture.height },
            width,              // size: treat this as world width
            door.tint,
            Vector3Distance(camera.position, bottomLeftClosed),
            door.rotationY,     // closed yaw (radians)
            false, //flipx
            door.isPortal,
            door.isOpen
        });
    }
}




void GatherDecals(Camera& camera, const std::vector<Decal>& decals) {
    for (const Decal& decal : decals) {
        if (!decal.alive) continue;

        float dist = Vector3Distance(camera.position, decal.position);

        Rectangle sourceRect;

        if (decal.type == DecalType::Explosion) {
            sourceRect = {
                static_cast<float>(decal.currentFrame * 196),
                0,
                196,
                196
            };
        
        } else if (decal.type == DecalType::MeleeSwipe){
            sourceRect = {
                static_cast<float>(decal.currentFrame * 196),
                0,
                196,
                196
            };           

        }else if (decal.type == DecalType::Blood){
            sourceRect = {
                static_cast<float>(decal.currentFrame * 256),
                0,
                256,
                256
            };    

        }else {
            sourceRect = {
                static_cast<float>(decal.currentFrame * 64),
                0,
                64,
                64
            };
        }

        billboardRequests.push_back({
            Billboard_Decal,
            decal.position,
            decal.texture,
            sourceRect,
            decal.size,
            WHITE,
            dist,
            0.0f,
            false,
            false,
            false
        });
    }
}

void GatherMuzzleFlashes(Camera& camera, const std::vector<MuzzleFlash>& flashes) {
    for (const auto& flash : flashes) {
        float dist = Vector3Distance(camera.position, flash.position);
        
        billboardRequests.push_back({
            Billboard_FacingCamera,
            flash.position,
            flash.texture,
            Rectangle{0, 0, (float)flash.texture.width, (float)flash.texture.height},
            flash.size,
            WHITE,
            dist,
            0.0f,
            false,
            false,
            false
        });
    }
}

void GatherTransparentDrawRequests(Camera& camera, float deltaTime) {
    billboardRequests.clear();

    GatherEnemies(camera);
    GatherDungeonFires(camera, deltaTime);
    GatherWebs(camera);
    GatherDoors(camera);
    GatherDecals(camera, decals);
    GatherMuzzleFlashes(camera, activeMuzzleFlashes);
    GatherCollectables(camera, collectables);
    GatherSpiderEggDrawRequests(camera);
    GatherGrapplePoint(camera);
}

void DrawTransparentDrawRequests(Camera& camera) {
    //sort and draw the drawRequest structs. 
    std::sort(billboardRequests.begin(), billboardRequests.end(),
        [](const BillboardDrawRequest& a, const BillboardDrawRequest& b) {
            return a.distanceToCamera > b.distanceToCamera;
        });

    for (const BillboardDrawRequest& req : billboardRequests) {
        //use alpha cut out shader on everything. treeShader does the fog at a distance thing + alpha cutout
        if (!isDungeon) BeginShaderMode(R.GetShader("treeShader"));
        if (isDungeon) BeginShaderMode(R.GetShader("cutoutShader"));

        Rectangle src = req.sourceRect;
        if (req.flipX) {
            src.width = -src.width; // negative width flips UVs
        }
                
        switch (req.type) {
            case Billboard_FacingCamera: //use draw billboard for both decals, and enemies. 
            case Billboard_Decal:
                DrawBillboardRec(
                    camera,
                    (req.texture),
                    src,
                    req.position,
                    Vector2{req.size, req.size},
                    req.tint
                );
                break;
            case Billboard_FixedFlat: //special case for webs
                DrawFlatWeb(
                    (req.texture),
                    req.position,
                    req.size,
                    req.size,
                    req.rotationY,
                    req.tint
                );
                break;

            case Billboard_Door:
                if (req.isPortal) BeginShaderMode(R.GetShader("portalShader")); 

                float doorWidth  = req.size;    // what we pushed from GatherDoors
                float doorHeight = 365.0f;
                if (req.isOpen){
                    rlDisableDepthMask();  // depth writes OFF, Open doors were occluding enemy billboards, so don't write to depth.
                    DrawFlatDoor( (req.texture),  req.position,  doorWidth,  doorHeight,  req.rotationY,  req.isOpen, req.tint);
                    rlEnableDepthMask();
                }else{
                    rlEnableDepthMask();
                    DrawFlatDoor( (req.texture),  req.position,  doorWidth,  doorHeight,  req.rotationY,  req.isOpen, req.tint);
                }      
                
                break;
        }
        EndShaderMode();
        rlEnableDepthMask();
    }
}
