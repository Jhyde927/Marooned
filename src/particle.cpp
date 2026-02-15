#include "particle.h"
#include "raymath.h"
#include <iostream>
#include <algorithm>


void Particle::Update(float dt) {
    if (!active) return;

    // apply gravity 
    // so subtract gravity * dt from upward velocity
    velocity.y -= gravity * dt;

    // apply viscous damping (drag) — slows all components
    // clamp drag so it doesn't explode for large dt
    float damp = Clamp(drag, 0.0f, 20.0f);
    velocity.x -= velocity.x * damp * dt;
    velocity.y -= velocity.y * damp * dt;
    velocity.z -= velocity.z * damp * dt;

    // integrate
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    position.z += velocity.z * dt;

    // life countdown
    life -= dt;
    float lifeT = (maxLife > 0.0f) ? (life / maxLife) : 0.0f;

    // fade alpha and shrink by life fraction
    unsigned char a = (unsigned char)(Clamp(lifeT, 0.0f, 1.0f) * 255.0f);
    color.a = a;
    // shrink slightly over lifetime (tweak multiplier to taste)
    size *= 0.98f + 0.02f * lifeT; // shrinks more as life decreases

    // Ground collision: simple plane at y = groundY.
    // NOTE: set groundY to match your level geometry. For now we assume groundY = 0.
    const float groundY = 0.0f;

    if (position.y <= groundY) {
        // clamp on ground
        position.y = groundY;

        // small threshold velocity to consider "stuck"
        const float minBounceVel = 80.0f; // tweak: velocity in world units/s

        if (fabsf(velocity.y) > minBounceVel) {
            // bounce (lose energy on bounce)
            velocity.y = -velocity.y * bounce;

            // damp horizontal motion on bounce too (blood splashes out a bit then slows)
            velocity.x *= 0.35f;
            velocity.z *= 0.35f;
        } else {
            // "stick" — blood drop hits ground and nearly stops
            velocity = {0.0f, 0.0f, 0.0f};
            // optionally make it linger a bit then die faster
            //life = min(life, 0.25f + 0.25f * lifeT); // shorten remaining life so it disappears
            // optionally reduce size for pooling look:
            size *= 0.6f;
        }
    }

    // deactivate once life is over or too small
    if (life <= 0.0f || size <= 0.01f) {
        active = false;
    }
}

// void Particle::Update(float dt) {
//     if (!active) return;

//     velocity.y -= gravity * dt;
//     position = Vector3Add(position, Vector3Scale(velocity, dt));
//     life -= dt;

//     float t = life / maxLife;
//     color.a = (unsigned char)(255 * t); //fadeout alpha over time. 

//     if (life <= 0.0f) active = false;
// }

void Particle::Draw(Camera3D& camera) const {
    if (!active) return;

    Vector2 screenPos = GetWorldToScreen(position, camera);
    //DrawSphere(position, size, color);
    DrawCube(position, size, size, size, color); //cubes look better i guess. 
}
