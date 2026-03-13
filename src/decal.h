#pragma once
#include "raylib.h"
#include <vector>
#include <iostream>
#include "raymath.h"
#include "emitter.h"


// What kind of decal this is
enum class DecalType {
    Smoke,
    Blood,
    Explosion,
    MagicAttack,
    NewBlood,
    Blocked,
    MeleeSwipe,
    ZombieHead,
    ZombieArm,
    ZombieGib,
    Bone,
};

struct Decal {
    Vector3 position;
    DecalType type;
    Texture2D texture;
    int maxFrames = 1;
    float lifetime = 1.0f;
    float frameTime = 0.1f;
    float size = 1.0f;
    float timer = 0.0f;
    int currentFrame = 0;
    bool alive = true;
    Vector3 velocity;
    float drag = 1.0f;
    float gravity = 900.0f;
    bool canBounce;
    bool hasBounced;
    Emitter bloodEmitter;

    Decal(Vector3 pos, DecalType t, Texture2D tex, int frameCount, float life, float frameDuration, float scale = 1.0f)
        : position(pos), type(t), texture(tex), maxFrames(frameCount),
          lifetime(life), frameTime(frameDuration), size(scale)
    {}

    void Update(float deltaTime) {
        timer += deltaTime;

        if (timer >= lifetime) {
            alive = false;
            return;
        }

        if (type == DecalType::MeleeSwipe){
                    // Kinematics
            position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
            // Exponential drag (stable, framerate-independent)
            float damp = expf(-drag * deltaTime);
            velocity = Vector3Scale(velocity, damp);

        }

        if (type == DecalType::ZombieHead || type == DecalType::ZombieArm || type == DecalType::ZombieGib || type == DecalType::Bone) {
            bloodEmitter.UpdateBlood(deltaTime);
            //gravity
            velocity.y -= gravity * deltaTime;
            //move
            position = Vector3Add(position, Vector3Scale(velocity, deltaTime));
            //damp
            float damp = expf(-drag * deltaTime);
            velocity = Vector3Scale(velocity, damp);  

            float floorY = 125.0f;

            if (position.y < floorY) {
                position.y = floorY;

                if (canBounce && !hasBounced) {
                    hasBounced = true;

                    velocity.y *= -0.35f;  // small bounce upward
                    velocity.x *= 0.7f;    // lose some sideways speed
                    velocity.z *= 0.7f;
                }
                else {
                    velocity.y = 0.0f;
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                }
            }


        }




        currentFrame = static_cast<int>(timer / frameTime);
        if (currentFrame >= maxFrames) currentFrame = maxFrames - 1;
    }
    //decals are drawn in transparentDraw after GatherDecals


};