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

    void Update(float dt);
    void Draw(Camera3D& camera) const;
};
