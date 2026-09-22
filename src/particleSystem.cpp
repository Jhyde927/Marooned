#include "particleSystem.h"

#include <algorithm>

#include "raymath.h"
#include "utilities.h"

ParticleSystem::ParticleSystem()
    : ParticleSystem(10000)
{
}

ParticleSystem::ParticleSystem(int maxParticleCount)
    : maxParticles(std::max(1, maxParticleCount))
{
    particles.resize(maxParticles);
}

void ParticleSystem::Update(float deltaTime)
{
    for (Particle& particle : particles)
    {
        if (particle.active)
        {
            particle.Update(deltaTime);
        }
    }
}

void ParticleSystem::Draw(Camera3D& camera) const
{
    for (const Particle& particle : particles)
    {
        if (particle.active)
        {
            particle.Draw(camera);
        }
    }
}

void ParticleSystem::EmitBurst(
    Vector3 position,
    int count,
    ParticleType type)
{
    if (count <= 0)
    {
        return;
    }

    EmitParticles(position, count, type, RED);
}

void ParticleSystem::EmitTrail(
    Vector3 position,
    float deltaTime,
    float emissionRate,
    float& emissionAccumulator,
    ParticleType type)
{
    Color defaultColor = WHITE;

    if (type == ParticleType::Smoke)
    {
        defaultColor = {
            static_cast<unsigned char>(0.2f * 255.0f),
            static_cast<unsigned char>(0.2f * 255.0f),
            static_cast<unsigned char>(0.2f * 255.0f),
            1
        };
    }

    EmitTrail(
        position,
        deltaTime,
        emissionRate,
        emissionAccumulator,
        type,
        defaultColor);
}

void ParticleSystem::EmitTrail(
    Vector3 position,
    float deltaTime,
    float emissionRate,
    float& emissionAccumulator,
    ParticleType type,
    Color color)
{
    if (deltaTime <= 0.0f || emissionRate <= 0.0f)
    {
        return;
    }

    emissionAccumulator += deltaTime * emissionRate;

    const int count = static_cast<int>(emissionAccumulator);

    if (count <= 0)
    {
        return;
    }

    emissionAccumulator -= static_cast<float>(count);

    EmitParticles(
        position,
        count,
        type,
        color);
}
// void ParticleSystem::EmitTrail(
//     Vector3 position,
//     float deltaTime,
//     float emissionRate,
//     float& emissionAccumulator,
//     ParticleType type)
// {
//     if (deltaTime <= 0.0f || emissionRate <= 0.0f)
//     {
//         return;
//     }

//     emissionAccumulator += deltaTime * emissionRate;

//     const int count = static_cast<int>(emissionAccumulator);

//     if (count <= 0)
//     {
//         return;
//     }

//     emissionAccumulator -= static_cast<float>(count);

//     EmitParticles(position, count, type, WHITE);
// }

void ParticleSystem::EmitBlood(
    Vector3 position,
    int count,
    Color color)
{
    if (count <= 0)
    {
        return;
    }

    EmitParticles(
        position,
        count,
        ParticleType::Blood,
        color);
}

std::size_t ParticleSystem::GetActiveParticleCount() const
{
    std::size_t count = 0;

    for (const Particle& particle : particles)
    {
        if (particle.active)
        {
            ++count;
        }
    }

    return count;
}

int ParticleSystem::GetMaxParticleCount() const
{
    return static_cast<int>(particles.size());
}

Particle* ParticleSystem::FindInactiveParticle()
{
    for (Particle& particle : particles)
    {
        if (!particle.active)
        {
            return &particle;
        }
    }

    return nullptr;
}

void ParticleSystem::EmitParticles(
    Vector3 position,
    int count,
    ParticleType type,
    Color color)
{
    for (int i = 0; i < count; ++i)
    {
        Particle* particle = FindInactiveParticle();

        if (particle == nullptr)
        {
            return;
        }

        CreateParticle(
            *particle,
            position,
            type,
            color);
    }
}

void ParticleSystem::CreateParticle(
    Particle& particle,
    Vector3 position,
    ParticleType type,
    Color color)
{
    // Reset every property because this Particle may have been used by
    // a different effect previously.
    particle.active = true;
    particle.position = position;
    particle.velocity = { 0.0f, 0.0f, 0.0f };
    particle.life = 1.5f;
    particle.maxLife = 1.5f;
    particle.gravity = 0.0f;
    particle.color = WHITE;
    particle.size = 8.0f;
    particle.drag = 0.0f;
    particle.bounce = 0.0f;

    switch (type)
    {
        case ParticleType::Smoke:
        {
            particle.color = color;

            particle.gravity = -100.0f;

            particle.velocity = {
                RandomFloat(-50.0f, 50.0f),
                RandomFloat(-25.0f, 25.0f),
                RandomFloat(-50.0f, 50.0f)
            };

            break;
        }

        case ParticleType::Sparks:
        {
            particle.color = ORANGE;
            particle.gravity = 980.0f;

            particle.velocity = {
                RandomFloat(-300.0f, 300.0f),
                RandomFloat(300.0f, 1000.0f),
                RandomFloat(-300.0f, 300.0f)
            };

            break;
        }

        case ParticleType::Blood:
        {
            particle.color = color;
            particle.gravity = 1800.0f + RandomFloat(-200.0f, 200.0f);

            particle.velocity = {
                RandomFloat(-120.0f, 120.0f),
                RandomFloat(80.0f, 500.0f),
                RandomFloat(-120.0f, 120.0f)
            };

            particle.maxLife = RandomFloat(0.4f, 1.2f);
            particle.life = particle.maxLife;
            particle.size = RandomFloat(3.0f, 5.5f);

            break;
        }

        case ParticleType::IceMist:
        {
            particle.color = SKYBLUE;
            particle.gravity = -50.0f;

            particle.velocity = {
                RandomFloat(-40.0f, 40.0f),
                RandomFloat(-10.0f, 20.0f),
                RandomFloat(-40.0f, 40.0f)
            };

            break;
        }

        case ParticleType::IceBlast:
        {
            particle.color = SKYBLUE;
            particle.gravity = 980.0f;

            particle.velocity = {
                RandomFloat(-250.0f, 250.0f),
                RandomFloat(300.0f, 1000.0f),
                RandomFloat(-250.0f, 250.0f)
            };

            break;
        }

        case ParticleType::FireTrail:
        {
            particle.color = ORANGE;
            particle.gravity = 100.0f;

            const float radius = 20.0f;

            Vector3 offset = {
                RandomFloat(-1.0f, 1.0f),
                RandomFloat(-1.0f, 1.0f),
                RandomFloat(-1.0f, 1.0f)
            };

            const float offsetLength = Vector3Length(offset);

            if (offsetLength > 0.0001f)
            {
                offset = Vector3Scale(
                    offset,
                    RandomFloat(0.0f, radius) / offsetLength);
            }
            else
            {
                offset = { 0.0f, 0.0f, 0.0f };
            }

            particle.position = Vector3Add(position, offset);

            particle.velocity = {
                RandomFloat(-30.0f, 30.0f),
                RandomFloat(30.0f, 100.0f),
                RandomFloat(-30.0f, 30.0f)
            };

            break;
        }

        case ParticleType::Impact:
        {
            // This will become configurable when we migrate impact effects.
            particle.color = WHITE;
            particle.gravity = 500.0f;

            particle.velocity = {
                RandomFloat(-200.0f, 200.0f),
                RandomFloat(-200.0f, 200.0f),
                RandomFloat(-200.0f, 200.0f)
            };

            break;
        }

        case ParticleType::BoltTrail:
        {
            particle.color = GRAY;
            particle.gravity = -100.0f;

            particle.velocity = {
                RandomFloat(-50.0f, 50.0f),
                RandomFloat(-25.0f, 25.0f),
                RandomFloat(-50.0f, 50.0f)
            };

            break;
        }

        case ParticleType::Squid:
        {
            particle.color = DARKPURPLE;
            particle.gravity = 980.0f;

            particle.velocity = {
                RandomFloat(-1000.0f, 1000.0f),
                RandomFloat(0.0f, 3000.0f),
                RandomFloat(-1000.0f, 1000.0f)
            };

            break;
        }
    }
}