// Include ONCE at the END of collisions.cpp, outside any function/namespace.
// Keep its existing game headers; add standard includes here if not already present.
#include <algorithm>
#include <cmath>
#include <vector>
#include "bulletCollisionGeometry.h"
#include "bulletCollisions.h"
#include "bullet.h"
#include "dungeonGeneration.h"
#include "world.h"
#include "pathfinding.h"
#include "spiderEgg.h"
#include "collisions.h"

namespace BulletCollisionDetail
{
using BulletCollisionGeometry::Hit;
using BulletCollisionGeometry::Point;

enum class Kind { Wall, Floor, Ceiling, Boundary, Player, Enemy, Kraken,
                  Egg, Grapple, Pickup, Solid, Web, BarrelProbe };
struct Contact
{
    Kind kind;
    float t;
    Vector3 normal;
    std::size_t index;
};

bool IsMagicBall(const Bullet& b)
{
    return b.type == BulletType::Fireball || b.type == BulletType::Iceball;
}

bool CanCollide(const Bullet& b)
{
    return b.alive && !b.explosionTriggered &&
           !b.stuck && !b.retracting;
}

bool ValidDungeonPoint(Vector3 p)
{
    const int x = GetDungeonImageX(p.x, tileSize, dungeonWidth);
    const int y = GetDungeonImageY(p.z, tileSize, dungeonWidth);
    if (x < 0 || y < 0 || x >= dungeonWidth || y >= dungeonWidth) return false;
    const int i = Idx(x, y);
    return i >= 0 && static_cast<std::size_t>(i) < lavaMask.size() &&
                     static_cast<std::size_t>(i) < voidMask.size();
}

float FloorY(Vector3 p)
{
    if (!isDungeon) return GetHeightAtWorldPosition(p, heightmap, terrainScale);
    const int x = GetDungeonImageX(p.x, tileSize, dungeonWidth);
    const int y = GetDungeonImageY(p.z, tileSize, dungeonWidth);
    const int i = Idx(x, y); // caller validates p before using masks
    if (voidMask[i] == 1) return -1000.0f;
    if (lavaMask[i] == 1) return floorHeight - lavaOffsetY;
    return floorHeight;
}

void AddBox(std::vector<Contact>& hits, Kind kind, std::size_t index,
            const Bullet& b, BoundingBox box)
{
    Hit hit;
    if (BulletCollisionGeometry::SweepBox(b.prevPosition, b.position, box, b.radius, hit))
        hits.push_back({kind, hit.t, hit.normal, index});
}

// Boolean LOS cannot return a face normal. Recover the first obstruction with
// prefix bisection, then probe the X/Z components near that crossing.
// For an exact corner normal, extend the actual DDA to return its crossed face.
void AddDDAWall(std::vector<Contact>& hits, const Bullet& b, float limit)
{
    const Vector3 a = b.prevPosition;
    const Vector3 end = Point(a, b.position, limit);
    if (DDAHasLineOfSightWorld(a, end)) return;
    float lo = 0.0f, hi = limit;
    for (int i = 0; i < 20; ++i)
    {
        const float mid = (lo + hi) * 0.5f;
        if (DDAHasLineOfSightWorld(a, Point(a, b.position, mid))) lo = mid;
        else hi = mid;
    }
    const Vector3 before = Point(a, b.position, lo);
    const Vector3 after = Point(a, b.position, hi);
    const Vector3 delta = Vector3Subtract(b.position, a);
    Vector3 normal = BulletCollisionGeometry::OpposingAxis(delta);
    const bool hitX = !DDAHasLineOfSightWorld(before, {after.x, before.y, before.z});
    const bool hitZ = !DDAHasLineOfSightWorld(before, {before.x, before.y, after.z});
    if (hitX && (!hitZ || std::fabs(delta.x) >= std::fabs(delta.z)))
        normal = {delta.x >= 0 ? -1.0f : 1.0f, 0, 0};
    else if (hitZ)
        normal = {0, 0, delta.z >= 0 ? -1.0f : 1.0f};
    hits.push_back({Kind::Wall, lo, normal, 0}); // last safe point, not inside wall
}

void AddWorld(std::vector<Contact>& hits, const Bullet& b)
{
    const Vector3 a = b.prevPosition, end = b.position;
    float limit = 1.0f;
    if (isDungeon)
    {
        if (!ValidDungeonPoint(a))
        {
            hits.push_back({Kind::Boundary, 0, {0,1,0}, 0});
            return;
        }
        if (!ValidDungeonPoint(end))
        {
            float lo = 0, hi = 1;
            for (int i = 0; i < 20; ++i)
            {
                float mid = (lo + hi) * 0.5f;
                if (ValidDungeonPoint(Point(a, end, mid))) lo = mid;
                else hi = mid;
            }
            limit = lo;
            hits.push_back({Kind::Boundary, limit, {0,1,0}, 0});
        }
        AddDDAWall(hits, b, limit);
        const float ceiling = ceilingHeight - b.radius;
        if (drawCeiling && end.y >= ceiling)
        {
            float t = a.y >= ceiling ? 0.0f : (ceiling - a.y) / (end.y - a.y);
            if (t <= limit) hits.push_back({Kind::Ceiling, t, {0,-1,0}, 0});
        }
    }

    // Sample the height field along the segment, then refine the first bracket.
    // Dungeon samples also catch crossing into a raised floor from lava/void.
    const float length = Vector3Distance(a, end) * limit;
    const float stepSize = isDungeon ? std::max(1.0f, tileSize * 0.125f) : 12.5f;
    const int steps = std::min(4096, std::max(1, static_cast<int>(std::ceil(length / stepSize))));
    auto below = [&](float t) {
        const Vector3 p = Point(a, end, t);
        return p.y <= FloorY(p) + b.radius;
    };
    float last = 0;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = limit * static_cast<float>(i) / steps;
        if (!below(t)) { last = t; continue; }
        float lo = last, hi = t;
        for (int j = 0; j < 16; ++j)
        {
            const float mid = (lo + hi) * 0.5f;
            if (below(mid)) hi = mid; else lo = mid;
        }
        Vector3 normal = {0,1,0};
        if (isDungeon && hi > 0 && FloorY(Point(a,end,hi)) > FloorY(Point(a,end,lo)) + 0.1f)
        {
            const Vector3 d = Vector3Subtract(end,a);
            normal = BulletCollisionGeometry::OpposingAxis({d.x,0,d.z});
        }
        if (!isDungeon)
        {
            const Vector3 p = Point(a, end, hi);
            const float e = 2.0f;
            normal = Vector3Normalize(Vector3{
                (FloorY({p.x-e,p.y,p.z}) - FloorY({p.x+e,p.y,p.z})) / (2*e),
                1,
                (FloorY({p.x,p.y,p.z-e}) - FloorY({p.x,p.y,p.z+e})) / (2*e)});
        }
        hits.push_back({Kind::Floor, hi, normal, 0});
        break;
    }
}

void Gather(std::vector<Contact>& hits, const Bullet& b)
{
    AddWorld(hits, b); // solid world wins equal-distance ties
    if (!b.pendingExplosion)
    {
        // Player shots can still hurt the player through explosion splash, but
        // cannot immediately collide with their shooter on leaving the muzzle.
        if (b.enemy || b.launcher)
            AddBox(hits, Kind::Player, 0, b, player.GetBoundingBox());
        for (std::size_t i = 0; i < enemyPtrs.size(); ++i)
        {
            Character* e = enemyPtrs[i];
            if (!e || e->isDead || e == b.owner) continue;
            if (IsMagicBall(b) && e->type == CharacterType::Wizard) continue;
            if (b.type == BulletType::Bolt &&
                std::find(b.piercedEnemyIds.begin(), b.piercedEnemyIds.end(), e->id) != b.piercedEnemyIds.end()) continue;
            AddBox(hits, Kind::Enemy, i, b, e->GetBoundingBox());
        }
        if (b.type == BulletType::CannonBall) AddBox(hits, Kind::Kraken, 0, b, gKraken.hitBox);
        for (std::size_t i = 0; i < eggs.size(); ++i)
            if (eggs[i].state != SpiderEggState::Destroyed) AddBox(hits, Kind::Egg, i, b, eggs[i].collider);
        if (b.type == BulletType::Harpoon)
        {
            if (b.age > 0.1f)
                for (std::size_t i = 0; i < grapplePoints.size(); ++i)
                    if (grapplePoints[i].enabled) AddBox(hits, Kind::Grapple, i, b, grapplePoints[i].box);
            for (std::size_t i = 0; i < collectables.size(); ++i)
                if (collectables[i].lastHarpoonBulletId != b.id) AddBox(hits, Kind::Pickup, i, b, collectables[i].hitBox);
        }
    }
    // These are separate objects unless your DDA explicitly includes them.
    for (const Door& d : doors)
    {
        if (!d.isOpen) AddBox(hits, Kind::Solid, 0, b, d.collider);
        if (d.isOpen)
            for (const BoundingBox& side : d.sideColliders) AddBox(hits, Kind::Solid, 0, b, side);
    }
    for (const Box& box : boxes) AddBox(hits, Kind::Solid, 0, b, box.bounds);
    for (const PillarInstance& p : pillars) AddBox(hits, Kind::Solid, 0, b, p.bounds);
    if (!b.pendingExplosion)
    {
        for (std::size_t i = 0; i < spiderWebs.size(); ++i)
            if (!spiderWebs[i].destroyed) AddBox(hits, Kind::Web, i, b, spiderWebs[i].bounds);

        // The supplied barrel helper is destructive and exposes no bounds/query.
        // Probe in travel order, interleaved with real contacts, never beyond a wall.
        // Replace this adapter with swept barrel boxes when their structure is available.
        const float step = std::max(2.0f, b.radius * 0.5f);
        const int count = std::min(4096, std::max(1, static_cast<int>(std::ceil(Vector3Distance(b.prevPosition,b.position)/step))));
        for (int i = 0; i <= count; ++i)
            hits.push_back({Kind::BarrelProbe, static_cast<float>(i)/count, {0,1,0}, 0});
    }
    std::stable_sort(hits.begin(), hits.end(), [](const Contact& a, const Contact& c) { return a.t < c.t; });
}

void ImpactSparks(Bullet& b, Vector3 normal)
{
    b.light.posWhenDetached = Vector3Add(
        b.position,
        Vector3Scale(normal, 5.0f)
    );
    b.light.detached = true;
    b.light.age = 0.0f;
}

// void ImpactSparks(Bullet& b, Vector3 normal)
// {
//     const float speed = Vector3Length(b.velocity);
//     const float strength = Clamp((speed - 850.0f) / 850.0f, 0, 1);
//     if (strength > 0)
//         particleSystem.EmitBurst(b.position, std::max(1, static_cast<int>(100 * strength)), ParticleType::Sparks);
//     b.light.posWhenDetached = Vector3Add(b.position, Vector3Scale(normal, 5.0f));
//     b.light.detached = true;
//     b.light.age = 0;
// }

void SolidImpact(Bullet& b, Camera& camera, Vector3 normal, bool floor = false)
{
    if (IsMagicBall(b) || b.type == BulletType::CannonBall) { b.Explode(camera); return; }
    if (b.type == BulletType::Harpoon || b.type == BulletType::Bolt) { b.kill(camera); return; }
    if (floor)
    {
        bool lava = false;
        if (isDungeon && ValidDungeonPoint(b.position))
        {
            const int x = GetDungeonImageX(b.position.x, tileSize, dungeonWidth);
            const int y = GetDungeonImageY(b.position.z, tileSize, dungeonWidth);
            lava = lavaMask[Idx(x,y)] == 1;
        }
        const bool water = !isDungeon && FloorY(b.position) <= 60.0f;
        if (lava || water)
        {
            BulletParticleBounce(b, lava ? ORANGE : SKYBLUE);
            b.kill(camera);
            return;
        }
    }
    ImpactSparks(b, normal); // capture incoming speed, before damping
    const bool bounced = TryBulletRicochet(b, normal, 0.6f, 500.0f, floor ? 0.7f : 0.999f);
    if (!bounced) b.kill(camera);
    // A surviving ricochet is still a visible, collidable projectile.
    else { b.exploded = false; b.alive = true; }
}

// true = stop handling this bullet this frame; false = penetrate/ignore.
bool Resolve(Bullet& b, Camera& camera, const Contact& hit)
{
    const float qDamage = player.quadDamage ? 4.0f : 1.0f;
    switch (hit.kind)
    {
    case Kind::Boundary:
        b.kill(camera);
        return true;
    case Kind::Player:
        if (b.type == BulletType::Fireball) b.Explode(camera);
        else if (b.type == BulletType::Iceball)
        {
            if (player.state != PlayerState::Frozen && player.canFreeze)
            {
                player.state = PlayerState::Frozen;
                player.canFreeze = false;
                player.freezeTimer = 1.5f;
            }
            b.Explode(camera); // consume even if player cannot be frozen again
        }
        else if (b.type == BulletType::CannonBall) { player.TakeDamage(25); b.Explode(camera); }
        else if (b.type == BulletType::Default) { b.BulletHole(camera); player.TakeDamage(25); }
        else { player.TakeDamage(25); b.kill(camera); }
        return true;
    case Kind::Enemy:
    {
        Character* e = enemyPtrs[hit.index];
        if (!e || e->isDead) return false;
        switch (b.type)
        {
        case BulletType::Default:
            if (b.enemy) { e->TakeDamage(150); b.kill(camera); }
            else if (b.hermit) { e->TakeDamage(10); b.kill(camera); }
            else
            {
                const int extra = e->state == CharacterState::Harpooned ? 20 : 0;
                e->TakeDamage(static_cast<int>(b.ComputeDamage()) + extra);
                ImpactSparks(b, hit.normal);
                if (!TryBulletRicochet(b, hit.normal, 0.6f, 500, 0.99f)) b.kill(camera);
                else b.exploded = false;
            }
            return true;
        case BulletType::Bolt:
            b.piercedEnemyIds.push_back(e->id);
            e->lastBulletIDHit = b.id;
            e->TakeDamage(75 * qDamage);
            return false;
        case BulletType::Harpoon:
            b.stuck = true;
            b.stuckEnemyId = e->id;
            b.stuckOffset = Vector3Subtract(b.position, e->position);
            b.velocity = {0,0,0};
            b.lifeTime = 3.0f;
            e->lastBulletIDHit = b.id;
            e->TakeDamage(75 * qDamage);
            if (!e->isDead && e->type != CharacterType::Trex)
            {
                e->harpoonTarget = player.position;
                e->ChangeState(CharacterState::Harpooned);
                SoundManager::GetInstance().Play("ratchet");
            }
            return true;
        case BulletType::Fireball:
            e->TakeDamage(25 * qDamage);
            b.pendingExplosion = true;
            b.explosionTimer = 0.04f;
            return true;
        case BulletType::Iceball:
            e->ChangeState(CharacterState::Freeze);
            b.pendingExplosion = true;
            b.explosionTimer = 0.04f;
            return true;
        case BulletType::CannonBall:
            e->TakeDamage(100);
            b.Explode(camera);
            return true;
        }
        return true;
    }
    case Kind::Kraken:
        gKraken.TakeDamage(500.0f);
        b.Explode(camera);
        return true;
    case Kind::Egg:
        if (eggs[hit.index].state == SpiderEggState::Destroyed) return false;
        DamageSpiderEgg(eggs[hit.index], (IsMagicBall(b) ? 100.0f : 25.0f) * qDamage, player.position);
        SolidImpact(b, camera, hit.normal);
        return true;
    case Kind::Grapple:
    {
        const GrapplePoint& gp = grapplePoints[hit.index];
        b.stuck = true;
        b.stuckToGrapple = true;
        b.stuckWorldPos = gp.position;
        b.position = gp.position;
        b.velocity = {0,0,0};
        b.lifeTime = 3.0f;
        if (player.state != PlayerState::Grappling)
        {
            player.state = PlayerState::Grappling;
            player.grappleTarget = gp.position;
            player.grappleSpeed = 3500;
            player.grappleStopDist = 70;
            player.grappleBulletId = b.id;
            player.harpoonLifeTimer = 3;
            SoundManager::GetInstance().Play("ratchet");
        }
        return true;
    }
    case Kind::Pickup:
    {
        Collectable& c = collectables[hit.index];
        c.isHarpooned = true;
        c.lastHarpoonBulletId = b.id;
        b.stuck = false;
        b.stuckEnemyId = -1;
        b.velocity = {0,0,0};
        b.retracting = true;
        b.retractTip = b.position;
        b.lifeTime = 9999;
        SoundManager::GetInstance().Play("ratchet");
        return true;
    }
    case Kind::Web:
        if (spiderWebs[hit.index].destroyed) return false;
        spiderWebs[hit.index].destroyed = true;
        SolidImpact(b, camera, hit.normal);
        return true;
    case Kind::BarrelProbe:
        if (!DestroyBarrelAtCollision(b.position, b.radius)) return false;
        if (b.type == BulletType::Bolt) return false;
        if (IsMagicBall(b) || b.type == BulletType::CannonBall) b.Explode(camera);
        else b.kill(camera); // no barrel bounds: absorb rather than invent a normal
        return true;
    case Kind::Floor:
        SolidImpact(b, camera, hit.normal, true);
        return true;
    case Kind::Wall:
    case Kind::Ceiling:
    case Kind::Solid:
        SolidImpact(b, camera, hit.normal);
        return true;
    }
    return true;
}
}

void HandleBulletCollisions(Camera& camera)
{
    using namespace BulletCollisionDetail;
    // Explosions must queue shrapnel; appending to this vector here is unsafe.

    std::vector<Contact> hits;
    for (Bullet& b : activeBullets)
    {
        if (!CanCollide(b)) continue;
        const Vector3 next = b.position;
        hits.clear();
        Gather(hits, b);
        bool stopped = false;
        for (const Contact& hit : hits)
        {
            b.position = Point(b.prevPosition, next, hit.t);
            if (Resolve(b, camera, hit)) { stopped = true; break; }
        }
        if (!stopped) b.position = next;
        if (b.alive && !b.explosionTriggered && b.pendingExplosion && b.explosionTimer <= 0)
            b.Explode(camera);
        // Discard untravelled movement after a bounce/attachment/impact.
        b.prevPosition = b.position;
    }
    // Safe here: no references/iterators into activeBullets remain in use.
    FlushPendingBulletSpawns();
}
