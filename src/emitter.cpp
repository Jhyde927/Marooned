#include "emitter.h"
#include <iostream>
#include "utilities.h"

Emitter::Emitter()
    : position({0, -9999, 0}) //-9999 off screen
{
    particles.resize(maxParticles);
}

Emitter::Emitter(Vector3 pos) : position(pos) {
    particles.resize(maxParticles);
}

void Emitter::SetPosition(Vector3 newPos) {
    position = newPos;
}

void Emitter::Update(float dt) { 
    
    if (emissionRate > 0.0f) {
        timeSinceLastEmit += dt;

        int toEmit = static_cast<int>(emissionRate * timeSinceLastEmit);
        if (toEmit > 0) {
            EmitParticles(toEmit);
            timeSinceLastEmit = 0.0f;
        }
    }

    for (auto& p : particles) {
        p.Update(dt);
    }
}

void Emitter::UpdateBlood(float dt){
  for (auto& p : particles) {
        p.Update(dt);
    }
}

void Emitter::EmitBurst(Vector3 pos, int count, ParticleType t) {
    if (canBurst) {
        canBurst = false;
        SetPosition(pos);
        SetParticleType(t);
        EmitParticles(count);
        
    }
}

void Emitter::EmitBlood(Vector3 pos, int count, Color color) {
    SetPosition(pos);

    for (int i = 0; i < count; ++i) {
        for (auto& p : particles) {
            if (!p.active) {
                p.active = true;
                p.position = position;

                p.color = color;
                p.gravity = 600.0f;  
                p.velocity = {
                    (float)GetRandomValue(-100, 100),
                    (float)GetRandomValue(50, 150),
                    (float)GetRandomValue(-100, 100)
                };

                p.maxLife = 2.0f;
                p.life = p.maxLife;
                p.size = 5.0f;

                break;
            }
        }
    }
}


void Emitter::EmitParticles(int count) {
    for (int i = 0; i < count; ++i) {
        for (auto& p : particles) {
            if (!p.active) {
                CreateParticle(p); // ← reuse particle

                break;
            }
        }
    }
}

void Emitter::CreateParticle(Particle& p) {
    p.active = true;
    p.position = position;

    switch (particleType) {
        case ParticleType::Sparks:
            p.color = ORANGE;
            p.gravity = 980.0f;
            p.velocity = {
                RandomFloat(-300, 300),
                RandomFloat(300, 1000),
                RandomFloat(-300, 300)
            };
            break;

        case ParticleType::FireTrail:
        {
            p.color = ORANGE;
            p.gravity = 100.0f;

            // Emit particles in a sphere around emitter position, not a point. 
            float radius = 20.0f; 
            Vector3 offset;
            do {
                offset = {
                    RandomFloat(-1.0f, 1.0f),
                    RandomFloat(-1.0f, 1.0f),
                    RandomFloat(-1.0f, 1.0f)
                };
            } while (Vector3Length(offset) > 1.0f); // reject points outside unit sphere
            offset = Vector3Scale(Vector3Normalize(offset), RandomFloat(0.0f, radius));

            p.position = Vector3Add(position, offset);

            p.velocity = {
                RandomFloat(-30, 30),
                RandomFloat(30, 100),
                RandomFloat(-30, 30)
            };
        }
        break;

        case ParticleType::Smoke:
            p.color = DARKGRAY;
            p.gravity = -100.0f;
            p.velocity = {
                RandomFloat(-50, 50),
                RandomFloat(-25, 25),
                RandomFloat(-50, 50)
            };
            break;

        case ParticleType::IceMist:

            p.color = SKYBLUE;
            p.gravity = -50.0f;
            p.velocity = {
                RandomFloat(-40, 40),
                RandomFloat(-10, 20),
                RandomFloat(-40, 40)
            };
            break;

        case ParticleType::IceBlast:
        p.color = SKYBLUE;
        p.gravity = 980.0f; // rising slowly
        p.velocity = {
            RandomFloat(-250, 250),
            RandomFloat(300, 1000),
            RandomFloat(-250, 250)
        };
        p.size = 10.0f;
        break;

        default:
            p.color = GRAY;
            break;
    }

    p.maxLife = 1.5f;
    p.life = p.maxLife;
    p.size = 8.0f;
}





void Emitter::Draw(Camera3D& camera) const{
    for (const auto& p : particles) {
        p.Draw(camera);
        
    }
}
