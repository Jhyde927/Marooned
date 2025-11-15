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


void GatherEnemies(Camera& camera) {
    //gather functions replace character.draw()
    //Everything 2d is saved to a vector of struct drawRequests. We save all the info needed to draw the billboard or quad to the drawRequest struct.
    //and push to the vector. Then sort the drawRequests based on distance and draw in that order. This prevents billboards occluding each other. 
    //This has become redundant. writing to depth sorts them alpha cutoff shader prevents transparent parts occluding other billboards 
    for (Character* enemy : enemyPtrs) {
        if (enemy->isDead && enemy->deathTimer <= 0.0f) continue;

        float dist = Vector3Distance(camera.position, enemy->position);

        // Frame source rectangle
        Rectangle sourceRect = {
            (float)(enemy->currentFrame * enemy->frameWidth),
            (float)(enemy->rowIndex * enemy->frameHeight),
            (float)enemy->frameWidth,
            (float)enemy->frameHeight
        };

        // Slight camera-facing offset to avoid z-fighting
        Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.position, enemy->position));
        Vector3 offsetPos = Vector3Add(enemy->position, Vector3Scale(camDir, 10.0f));

        //compensate for extreme billboard scaling, things shrink at a distance but not by much. 
        float billboardSize = GetAdjustedBillboardSize(enemy->frameWidth * enemy->scale, dist);
        // Dynamic tint for damage
        Color finalTint = (enemy->hitTimer > 0.0f) ? (Color){255, 50, 50, 255} : WHITE;
        if (enemy->state == CharacterState::Freeze){
            finalTint = SKYBLUE;
        }

        billboardRequests.push_back({
            Billboard_FacingCamera,//BillboardType
            offsetPos, //billboard position + z fighting offset
            enemy->texture,
            sourceRect,
            billboardSize,
            finalTint,
            dist,
            0.0f
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
            0.0f
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
            0.0f
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
            web.rotationY
        });
    }
}

void GatherDoors(Camera& camera) {
    for (const Door& door : doors) {
        if (door.isOpen) continue;

        billboardRequests.push_back({
            Billboard_Door,
            door.position,
            door.doorTexture,
            Rectangle{ 0, 0, (float)door.doorTexture.width, (float)door.doorTexture.height },
            door.scale.x, // width, used in size
            door.tint,
            Vector3Distance(camera.position, door.position),
            door.rotationY,
            door.isPortal
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
            0.0f
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
            0.0f
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
        switch (req.type) {
            case Billboard_FacingCamera: //use draw billboard for both decals, and enemies. 
            case Billboard_Decal:
                DrawBillboardRec(
                    camera,
                    (req.texture),
                    req.sourceRect,
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
                //we added another field to drawRequest just for portal doors. We could mark other things as portal an apply the same wacky color shader to them. 
                //maybe we could protal shader ghost. 
                DrawFlatDoor(
                    (req.texture), 
                    req.position, 
                    req.size, 
                    req.size * 1.225f, 
                    req.rotationY, 
                    req.tint);
                break;
        }
        EndShaderMode();
        rlEnableDepthMask();
    }
}
