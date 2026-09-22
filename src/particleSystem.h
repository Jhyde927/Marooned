#pragma once

#include <cstddef>
#include <vector>

#include "raylib.h"
#include "particle.h"
#include "particleTypes.h"

class ParticleSystem
{
public:
    ParticleSystem();
    explicit ParticleSystem(int maxParticleCount);

    void Update(float deltaTime);
    void Draw(Camera3D& camera) const;

    // Immediately creates a group of particles.
    void EmitBurst(
        Vector3 position,
        int count,
        ParticleType type);

    // Creates particles continuously over time.
    //
    // The accumulator belongs to the object producing the trail,
    // such as a bullet.
    void EmitTrail(
        Vector3 position,
        float deltaTime,
        float emissionRate,
        float& emissionAccumulator,
        ParticleType type);

    void EmitTrail(
        Vector3 position,
        float deltaTime,
        float emissionRate,
        float& emissionAccumulator,
        ParticleType type,
        Color color);

    void EmitBlood(
        Vector3 position,
        int count,
        Color color);

    std::size_t GetActiveParticleCount() const;
    int GetMaxParticleCount() const;

private:
    std::vector<Particle> particles;

    int maxParticles = 10000;

    Particle* FindInactiveParticle();

    void EmitParticles(
        Vector3 position,
        int count,
        ParticleType type,
        Color color);

    void CreateParticle(
        Particle& particle,
        Vector3 position,
        ParticleType type,
        Color color);

};