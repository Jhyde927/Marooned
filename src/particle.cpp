#include "particle.h"
#include "raymath.h"
#include <iostream>

void Particle::Update(float dt) {
    if (!active) return;

    velocity.y -= gravity * dt;
    position = Vector3Add(position, Vector3Scale(velocity, dt));
    life -= dt;

    float t = life / maxLife;
    color.a = (unsigned char)(255 * t); //fadeout alpha over time. 

    if (life <= 0.0f) active = false;
}

void Particle::Draw(Camera3D& camera) const {
    if (!active) return;

    Vector2 screenPos = GetWorldToScreen(position, camera);
    //DrawSphere(position, 5.0f, color);
    DrawCube(position, size, size, size, color); //cubes look better i guess. 
}
