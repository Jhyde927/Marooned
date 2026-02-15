#pragma once
#include "raylib.h"



struct Particle {
    Vector3 position;
    Vector3 velocity;
    float life;
    float maxLife;
    float gravity;
    Color color;
    float size;
    bool active = false;

    float drag = 0.0f;     // damping coefficient (higher = stops faster)
    float bounce = 0.0f;   // bounciness 0..1

    void Update(float dt);
    void Draw(Camera3D& camera) const;
};
