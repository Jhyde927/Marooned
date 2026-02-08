# Dungeon PNG Color Legend

Each tile in the dungeon PNG encodes gameplay data via RGB color.
Transparent pixels are treated as void.

---

## Geometry & Navigation

- **Void**  
  Transparent  
  Meaning: Empty / non-existent tile

- **Wall**  
  RGB: (0, 0, 0) — Black  
  Meaning: Solid wall

- **Floor**  
  RGB: (255, 255, 255) — White  
  Meaning: Floor tile (informational; usually implicit)

---

## Player & Entities

- **Player Start**  
  RGB: (0, 255, 0) — Green  
  Meaning: Player spawn position

- **Skeleton**  
  RGB: (255, 0, 0) — Red  
  Meaning: Skeleton enemy spawn

- **Pirate**  
  RGB: (255, 0, 255) — Magenta  
  Meaning: Pirate enemy spawn

- **Wizard**  
  RGB: (148, 0, 211) — Dark Violet  
  Meaning: Wizard enemy spawn

- **Bat**  
  RGB: (75, 0, 130) — Deep Purple  
  Meaning: Bat enemy spawn

- **Bloat Bat**  
  RGB: (110, 0, 110) — Bruised Purple  
  Meaning: Exploding bat enemy

- **Ghost**  
  RGB: (200, 200, 200) — Very Light Gray  
  Meaning: Ghost enemy spawn

- **Hermit**  
  RGB: (110, 74, 45) — Weathered Oak  
  Meaning: Friendly NPC

---

## Props & Items

- **Barrel**  
  RGB: (0, 0, 255) — Blue  
  Meaning: Breakable barrel

- **Box**  
  RGB: (139, 69, 19) — Saddle Brown  
  Meaning: Pushable / destructible box

- **Chest**  
  RGB: (0, 128, 255) — Sky Blue  
  Meaning: Treasure chest

- **Health Potion**  
  RGB: (255, 105, 180) — Pink  
  Meaning: Health pickup

- **Mana Potion**  
  RGB: (0, 0, 128) — Dark Blue  
  Meaning: Mana pickup

- **Key (Gold)**  
  RGB: (255, 200, 0) — Gold  
  Meaning: Generic key

- **Silver Key**  
  RGB: (180, 190, 205) — Cool Silver  
  Meaning: Opens silver doors

- **Skeleton Key**  
  RGB: (192, 192, 192) — Steel  
  Meaning: Opens skeleton doors

- **Blunderbuss**  
  RGB: (204, 204, 255) — Periwinkle  
  Meaning: Weapon pickup

- **Harpoon**  
  RGB: (96, 128, 160) — Gunmetal Blue  
  Meaning: Weapon pickup

- **Magic Staff**  
  RGB: (128, 0, 0) — Dark Red  
  Meaning: Weapon pickup

---

## Doors & Portals

- **Doorway**  
  RGB: (128, 0, 128) — Purple  
  Meaning: Standard doorway

- **Door Portal**  
  RGB: (200, 0, 200) — Hot Purple  
  Meaning: Portal-style door

- **Exit (Previous Level)**  
  RGB: (0, 128, 128) — Teal  
  Meaning: Exit to previous level

- **Next Level Door**  
  RGB: (255, 128, 0) — Orange  
  Meaning: Leads to next level

- **Locked Door (Aqua)**  
  RGB: (0, 255, 255) — Aqua  
  Meaning: Requires key

- **Silver Door**  
  RGB: (0, 64, 64) — Dark Cyan  
  Meaning: Requires silver key

- **Skeleton Door**  
  RGB: (230, 225, 200) — Aged Ivory  
  Meaning: Requires skeleton key

- **Monster Door**  
  RGB: (96, 0, 0) — Oxblood  
  Meaning: Enemy-triggered door

- **Secret Door**  
  RGB: (64, 0, 64) — Very Dark Purple  
  Meaning: Hidden door

---

## Switches & Events

- **Switch (Visible)**  
  RGB: (178, 190, 181) — Ash Gray  
  Meaning: Pressure plate / switch

- **Invisible Switch**  
  RGB: (159, 226, 191) — Seafoam Green  
  Meaning: Hidden trigger

- **Fire Switch**  
  RGB: (255, 99, 71) — Tomato  
  Meaning: Fire-linked trigger

- **Switch ID Marker**  
  RGB: (0, 102, 85) — Payne’s Green  
  Meaning: Groups switches & doors

- **Event Locked Door**  
  RGB: (0, 255, 128) — Spring Green  
  Meaning: Opens via event

---

## Traps & Hazards

- **Lava Tile**  
  RGB: (200, 0, 0) — Dark Red  
  Meaning: Damaging lava floor

- **Lava Glow**  
  RGB: (178, 34, 34) — Fire Brick  
  Meaning: Lava lighting marker

- **Launcher Trap (Fire)**  
  RGB: (255, 66, 52) — Vermillion  
  Meaning: Fireball launcher

- **Ice Launcher**  
  RGB: (173, 216, 230) — Ice Blue  
  Meaning: Ice projectile launcher

---

## Trap Encoding Helpers

- **Direction Marker**  
  RGB: (200, 200, 0) — Yellowish  
  Meaning: Trap firing direction

- **Timing Marker (1s)**  
  RGB: (200, 50, 0)

- **Timing Marker (2s)**  
  RGB: (200, 100, 0)

- **Timing Marker (3s)**  
  RGB: (200, 150, 0)

- **Timing Marker (Alt)**  
  RGB: (200, 175, 0)

---

## Lighting

- **Light Source**  
  RGB: (255, 255, 0) — Yellow

- **Invisible Light**  
  RGB: (255, 255, 100) — Pale Yellow

- **Blue Light**  
  RGB: (100, 149, 237) — Cornflower Blue

- **Yellow Light**  
  RGB: (255, 216, 0) — Safety Yellow

- **Green Light**  
  RGB: (80, 200, 120) — Emerald Green

---

## Special Walls

- **Invisible Wall**  
  RGB: (255, 203, 164) — Peach  
  Meaning: Collision without visuals

- **Windowed Wall**  
  RGB: (83, 104, 120) — Payne’s Gray  
  Meaning: Wall with visibility
