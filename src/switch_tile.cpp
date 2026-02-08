#include "switch_tile.h"
#include "dungeonGeneration.h"
#include "world.h"
#include "camera_system.h"
#include "sound_manager.h"
#include "pathfinding.h"
#include "dungeonColors.h"

using namespace dungeon;

std::vector<SwitchTile> switches;

static bool HasOrthogonalNeighborWithColor( //look for the fireswitch pixel
    const Color* pixels,
    int w, int h,
    int x, int y,
    Color target)
{
    auto InBounds = [&](int ix, int iy) -> bool {
        return (ix >= 0 && ix < w && iy >= 0 && iy < h);
    };

    // N
    if (InBounds(x, y - 1)) {
        if (EqualsRGB(pixels[(y - 1) * w + x], target))
            return true;
    }
    // E
    if (InBounds(x + 1, y)) {
        if (EqualsRGB(pixels[y * w + (x + 1)], target))
            return true;
    }
    // S
    if (InBounds(x, y + 1)) {
        if (EqualsRGB(pixels[(y + 1) * w + x], target))
            return true;
    }
    // W
    if (InBounds(x - 1, y)) {
        if (EqualsRGB(pixels[y * w + (x - 1)], target))
            return true;
    }

    return false;
}



static int CountOrthogonalNeighborsWithColor(//determine the locktype of the switch. 0 neighbors = gold, 1 = silver, 2 = skeleton 
//3 = event 4 = event. The switch pixel can also have a tomato colored neighbor that means it's a switch activated by fireball. 
    const Color* pixels,
    int w, int h,
    int x, int y,
    Color target)
{
    auto InBounds = [&](int ix, int iy) -> bool {
        return (ix >= 0 && ix < w && iy >= 0 && iy < h);
    };

    int count = 0;

    // N
    if (InBounds(x, y - 1)) {
        Color c = pixels[(y - 1) * w + x];
        if (EqualsRGB(c, target)) count++;
    }
    // E
    if (InBounds(x + 1, y)) {
        Color c = pixels[y * w + (x + 1)];
        if (EqualsRGB(c, target)) count++;
    }
    // S
    if (InBounds(x, y + 1)) {
        Color c = pixels[(y + 1) * w + x];
        if (EqualsRGB(c, target)) count++;
    }
    // W
    if (InBounds(x - 1, y)) {
        Color c = pixels[y * w + (x - 1)];
        if (EqualsRGB(c, target)) count++;
    }

    return count;
}

BoundingBox MakeSwitchBox(const Vector3& pos, SwitchKind kind, float tileSize, float baseY)
{
    //Make differently shaped bounding box depending on SwitchKind. 
    BoundingBox b{};
    switch (kind)
    {
        case SwitchKind::FloorPlate:
        {
            const float half = tileSize * 0.5f; // 100 if tileSize=200
            b.min = { pos.x - half, pos.y,          pos.z - half };
            b.max = { pos.x + half, pos.y + 25.0f,  pos.z + half };
        } break;

        case SwitchKind::InvisibleTrigger:
        {
            const float half = tileSize * 0.5f;
            b.min = { pos.x - half, pos.y,          pos.z - half };
            b.max = { pos.x + half, pos.y + 200.0f, pos.z + half }; // taller volume
        } break;

        case SwitchKind::FireballTarget:
        {
            // “thin slab” target, centered; you can later orient it based on adjacent wall
            const float halfW = tileSize * 0.25f; // 50 if tileSize=200
            const float halfH = tileSize * 0.35f; // 70 if tileSize=200
            const float halfT = 8.0f;             // thickness

            b.min = { pos.x - halfW, pos.y + 40.0f, pos.z - halfT };
            b.max = { pos.x + halfW, pos.y + 40.0f + 2*halfH, pos.z + halfT };
        } break;
    }

    return b;
}

inline bool HasActivator(uint32_t mask, Activator a) { return (mask & (uint32_t)a) != 0; }

bool IsSwitchPressed(const SwitchTile& st, const Player& player, const std::vector<Box>& boxes)
{
    // Player
    if (HasActivator(st.activators, Act_Player)) {
        if (CheckCollisionBoxSphere(st.box, player.position, player.radius)){
            return true;
        }

    }

    // Boxes
    if (HasActivator(st.activators, Act_Box)) {
        for (const Box& b : boxes) {
            if (CheckCollisionBoxes(st.box, b.bounds)){
                return true;
            }

        }
    }
    //fireballs
    if (HasActivator(st.activators, Act_Fireball)) {
        for (Bullet& b : activeBullets) {
            if (CheckCollisionBoxSphere(st.box, b.position, b.radius) && b.type == BulletType::Fireball){
                Camera& camera = CameraSystem::Get().Active();
                b.Explode(camera);
                return true;
            }

        }
    }


    return false;
}

static KeyType KeyTypeFromLockType(LockType lt)
{
    switch (lt)
    {
        case LockType::Gold:     return KeyType::Gold;
        case LockType::Silver:   return KeyType::Silver;
        case LockType::Skeleton: return KeyType::Skeleton;
        case LockType::Event:    return KeyType::Event;
        default:                 return KeyType::None; // or a sentinel
    }
}


void DeactivateSwitch(const SwitchTile& st)
{
    const KeyType key = KeyTypeFromLockType(st.lockType);
    if (key == KeyType::None) return;



    for (int i = 0; i < (int)doors.size(); ++i)
    {
        Door& door = doors[i];
        if (door.requiredKey != key) continue;

        ScheduleDoorAction(i, false, true); // false for close door, true for relock door. 
    }

}



void ActivateSwitch(const SwitchTile& st)
{
    auto UnlockDoors = [&](KeyType key)
    {
        for (int i = 0; i < (int)doors.size(); ++i)
        {
            Door& door = doors[i];
            if (door.requiredKey != key) continue;

            // Instead of immediate:
            // door.isLocked = false; door.isOpen = true; SetTileWalkable(...)

            ScheduleDoorAction(i, true);
        }
    };

    switch (st.lockType)
    {
        case LockType::Gold:     UnlockDoors(KeyType::Gold); break;
        case LockType::Silver:   UnlockDoors(KeyType::Silver); break;
        case LockType::Skeleton: UnlockDoors(KeyType::Skeleton); break;
        case LockType::Event:    UnlockDoors(KeyType::Event); break;
        default: break;
    }
}

static LockType LockTypeFromSwitchNeighborCount(int n)
{
    if (n <= 0) return LockType::Gold;
    if (n == 1) return LockType::Silver;
    if (n == 2) return LockType::Skeleton;
    return LockType::Event; // 3 or 4
}

void GenerateSwitches(float baseY)
{
    switches.clear();

    const Color cSwitch      = ColorOf(Code::Switch);     // anchor pixel
    const Color cSwitchID    = ColorOf(Code::switchID);   // lock markers
    const Color cSwitchFire  = ColorOf(Code::SwitchFire); // kind marker
    const Color cSwitchInvis = ColorOf(Code::SwitchInvis);// kind marker

    for (int y = 0; y < dungeonHeight; y++)
    {
        for (int x = 0; x < dungeonWidth; x++)
        {
            Color current = dungeonPixels[y * dungeonWidth + x];
            if (!EqualsRGB(current, cSwitch)) continue;

            // ---- decode lock routing ----
            const int idCount = CountOrthogonalNeighborsWithColor(
                dungeonPixels, dungeonWidth, dungeonHeight, x, y, cSwitchID
            );

            // ---- decode kind (default FloorPlate) ----
            SwitchKind kind = SwitchKind::FloorPlate;
            if (HasOrthogonalNeighborWithColor(dungeonPixels, dungeonWidth, dungeonHeight, x, y, cSwitchFire))
                kind = SwitchKind::FireballTarget;
            else if (HasOrthogonalNeighborWithColor(dungeonPixels, dungeonWidth, dungeonHeight, x, y, cSwitchInvis))
                kind = SwitchKind::InvisibleTrigger;

            SwitchTile st = {};
            st.position  = GetDungeonWorldPos(x, y, tileSize, baseY);
            st.lockType  = LockTypeFromSwitchNeighborCount(idCount);
            st.kind      = kind;

            // ---- per-kind defaults ----
            switch (st.kind)
            {
                case SwitchKind::FloorPlate:
                    st.mode       = TriggerMode::WhileHeld;
                    st.activators = Act_Player | Act_Box;
                    st.triggered  = false;
                    st.isPressed  = false;
                    break;

                case SwitchKind::InvisibleTrigger:
                    st.mode       = TriggerMode::OnEnter;
                    st.activators = Act_Player;      // match your old invisible switch behavior
                    st.triggered  = false;
                    st.isPressed  = false;
                    break;

                case SwitchKind::FireballTarget:
                    st.mode       = TriggerMode::OnEnter; // almost always what you want
                    st.activators = Act_Fireball;
                    st.triggered  = false;
                    st.isPressed  = false;
                    break;
            }

            // ---- collider shape depends on kind ----
            st.box = MakeSwitchBox(st.position, st.kind, tileSize, baseY);

            switches.push_back(st);
        }
    }
}