# ASCII Universe

ASCII Universe is a **C++20 colony and world simulation game** inspired by **Dwarf Fortress**, **Goblin Camp**, **KeeperRL**, and classic roguelikes.

The long-term goal is a persistent simulated world where autonomous inhabitants mine, gather resources, haul items, construct settlements, operate workshops, satisfy needs, form relationships, create civilizations, and eventually participate in procedurally generated history.

The project is being built from the ground up with a strong separation between:

```text
Player Intent
     ↓
Designations
     ↓
Jobs
     ↓
Autonomous Workers
     ↓
Simulation Systems
     ↓
Persistent World State
     ↓
SDL ASCII Frontend
```

The player is not intended to directly control individual goblins. Instead, the player designates work and manages the settlement while qualified goblins autonomously claim and perform jobs.

---

## Current Status

ASCII Universe currently has a working simulation foundation with:

- A 3D Dwarf Fortress-style world made of 2D Z-level slices
- Procedural terrain and geology
- Trees and surface resources
- Underground mineral layers and ore veins
- Water with deterministic flow simulation
- Autonomous mining
- Digging upward and downward
- Tree felling
- Item spawning
- Cross-Z-level hauling
- Stockpiles
- A* pathfinding across stairs, ramps, and multiple Z-levels
- Save/load support
- Deterministic world seeds
- An SDL3 ASCII frontend
- Runtime Z-level switching

The current development build automatically creates initial jobs so the simulation systems can be tested before the full player designation UI is implemented.

---

# Screens and World Model

ASCII Universe uses a **3D tile volume**:

```text
                 Z 11
        ┌──────────────────┐
        │ Surface / Air    │
        └──────────────────┘

                 Z 10
        ┌──────────────────┐
        │ Grass / Trees    │
        └──────────────────┘

                  Z 9
        ┌──────────────────┐
        │ Soil / Clay      │
        └──────────────────┘

                  Z 8
        ┌──────────────────┐
        │ Soil / Stone     │
        └──────────────────┘

                  Z 7
        ┌──────────────────┐
        │ Limestone / Ore  │
        └──────────────────┘

                  ...

                  Z 0
        ┌──────────────────┐
        │ Deep Rock        │
        └──────────────────┘
```

The simulation exists in full 3D, but the player views **one horizontal Z-level at a time**, similar to Dwarf Fortress.

A goblin can be working underground while another goblin works on the surface.

Changing the viewed Z-level does not pause or alter simulation on the other levels.

---

# SDL ASCII Frontend

The project now uses **SDL3** rather than rendering directly into the terminal.

The SDL frontend provides:

- A resizable window
- Colored ASCII glyphs
- Independent simulation and rendering
- Z-level switching
- Keyboard input
- A simulation HUD
- Display of terrain beneath open-air tiles
- No dependence on terminal dimensions, ANSI cursor positioning, or terminal state

The current frontend uses SDL's built-in fixed-width ASCII rendering while the interface is still under active development.

A future version can replace the glyph renderer with SDL_ttf without changing the simulation.

---

# Current Controls

```text
[ / , / PageDown     View lower Z-level
] / . / PageUp       View higher Z-level

Space                Pause / resume simulation

S                    Save world

Q / Escape           Save and quit
```

Additional camera movement, mouse selection, designation painting, and simulation speed controls are planned.

---

# Tile System

Tiles are no longer simply `Floor` or `Wall`.

Each tile stores several independent properties:

```text
Tile
│
├── Shape
│   ├── Open
│   ├── Floor
│   ├── Wall
│   ├── Ramp
│   ├── Up Stair
│   ├── Down Stair
│   └── Up/Down Stair
│
├── Material
│
├── Feature
│   └── Tree
│
└── Liquid
    ├── Type
    └── Depth
```

Separating tile geometry from material allows the simulation to represent things such as:

```text
Granite Wall
Iron Ore Wall
Soil Wall
Limestone Floor
Granite Stair
Grass Floor
```

without creating a different tile enum for every possible combination.

---

# Geology and Materials

The procedural world generator creates material-aware geology.

Current material categories include:

### Surface and Soil

```text
Grass
Soil
Clay
Sand
```

### Sedimentary Stone

```text
Limestone
Sandstone
```

### Igneous / Metamorphic Stone

```text
Granite
Basalt
Marble
Obsidian
```

### Minerals and Ores

```text
Coal
Iron Ore
Copper Ore
Tin Ore
Silver Ore
Gold Ore
Quartz
```

### Wood

```text
Oak
Pine
```

World depth influences material generation.

For example:

```text
Surface
   ↓
Grass

Upper underground
   ↓
Soil
Clay
Sand

Intermediate layers
   ↓
Limestone
Sandstone

Deeper layers
   ↓
Granite
Basalt
Marble
Obsidian

Mineral veins
   ↓
Iron
Copper
Tin
Silver
Gold
Coal
Quartz
```

The geology system is intended to eventually support biome-specific stone, gem deposits, aquifers, magma, soil fertility, and much larger geological formations.

---

# Procedural World Generation

World generation is deterministic from a seed.

Example:

```bash
./build/ascii_universe --seed 123456
```

The same seed produces the same generated starting world.

A different seed:

```bash
./build/ascii_universe --seed 999999
```

produces another world.

The current generator creates:

- A variable-height surface
- Grass layers
- Soil, clay, and sand
- Sedimentary layers
- Deep stone layers
- Mineral veins
- Trees
- Water
- Ramps
- A test underground fortress area
- Vertical access between surface and underground layers

World generation uses a separate deterministic random stream from runtime simulation randomness.

---

# Water Simulation

Water is represented directly on world tiles.

Liquid depth currently ranges from:

```text
0 = dry
1 = shallow
...
7 = full
```

The current system is a deterministic cellular fluid simulation.

Water attempts to move in this order:

```text
Gravity
   ↓
Lower Z-level
   ↓
Ramps / vertical connections
   ↓
Horizontal equalization
```

Deep water can prevent normal goblin movement.

This is intentionally a gameplay-oriented fluid simulation rather than a full physical fluid solver.

Future work may include:

- Pressure
- Flow strength
- Aquifers
- Pumps
- Drainage
- Evaporation
- Freezing
- Magma
- Temperature interaction
- Swimming and drowning

---

# Entity Component System

ASCII Universe uses **EnTT** for its ECS architecture.

Entities provide identity while components store data and capabilities.

Example:

```text
Uru
│
├── Goblin
├── Name
├── Miner
├── Position
├── Glyph
├── AssignedJob
└── MovementPath
```

Another goblin might contain:

```text
Brakka
│
├── Goblin
├── Name
├── Woodcutter
├── Position
└── Glyph
```

Current professions include:

```text
Miner
Hauler
Woodcutter
```

Profession components determine which jobs a goblin may claim.

Eventually these marker components may evolve into a richer labor and skill system.

---

# Job System

The simulation owns a central `JobBoard`.

Current job types are:

```text
Mine
Dig Down
Dig Up
Fell Tree
Haul
```

Jobs move through the following states:

```text
Available
Assigned
Complete
Cancelled
```

The basic worker pipeline is:

```text
Designation
     ↓
Job Creation
     ↓
Available Job
     ↓
Find Qualified Idle Goblin
     ↓
Check Reachability
     ↓
A* Pathfinding
     ↓
Assign Worker
     ↓
Movement
     ↓
Perform Work
     ↓
Complete Job
```

Workers therefore operate autonomously rather than receiving direct movement commands from the player.

---

# Mining

Mining removes a solid wall from the world.

Example:

```text
Before:

#######
#..g###
#...X##
#######

After:

#######
#...g##
#....##
#######
```

The mined material determines which item is produced.

Examples:

```text
Granite Wall
    ↓
Granite Stone

Iron Ore Wall
    ↓
Iron Ore

Gold Ore Wall
    ↓
Gold Ore
```

The resulting item is a real ECS entity and can later be hauled to a stockpile.

---

# Vertical Excavation

Miners can also create vertical access.

## Dig Down

A miner works from the current tile and creates a connection to the level below.

Conceptually:

```text
Before

Z 7       .
Z 6       #

After

Z 7       >
           │
Z 6       <
```

## Dig Up

The inverse process allows underground goblins to excavate toward the level above.

Multiple connected levels can form:

```text
Z 10      >
           │
Z  9      X
           │
Z  8      X
           │
Z  7      <
```

The pathfinder recognizes these vertical connections.

---

# Tree Felling

Trees are tile features rather than terrain walls.

A woodcutter can receive a `FellTree` job:

```text
Tree
  ↓
Woodcutter walks adjacent
  ↓
Tree removed
  ↓
Log item created
  ↓
Haul job generated
  ↓
Log delivered to stockpile
```

Current tree materials include:

```text
Oak
Pine
```

---

# Items

Items are ECS entities.

Current broad item categories are:

```text
Stone
Ore
Soil
Log
```

Each item also retains its underlying material.

For example:

```text
Item
├── Type: Ore
└── Material: IronOre
```

or:

```text
Item
├── Type: Log
└── Material: OakWood
```

This allows future systems to distinguish between generic item purpose and exact material.

---

# Stockpiles

Stockpiles occupy regions of the 3D world.

A stockpile stores:

```text
Bounds
Accepted Item Types
Current Items
Reserved Cells
Optional Capacity
```

Destination cells are reserved when haul jobs are generated so multiple haulers do not attempt to place items in the same location.

Stockpiles can currently accept categories such as:

```text
Stone
Ore
Soil
Logs
```

Future stockpile controls will allow the player to create, resize, delete, and configure stockpiles from the SDL interface.

---

# Hauling

Hauling is fully autonomous.

The pipeline is:

```text
Loose Item
    ↓
Find Compatible Stockpile
    ↓
Reserve Destination Cell
    ↓
Create Haul Job
    ↓
Hauler Claims Job
    ↓
Pathfind to Item
    ↓
Pick Up Item
    ↓
Pathfind to Stockpile
    ↓
Drop Item
    ↓
Complete Job
```

Because pathfinding operates across Z-levels, a surface hauler can retrieve ore from underground through stairs and return it to a surface stockpile.

---

# 3D A* Pathfinding

Pathfinding operates in the full `(x, y, z)` world.

Normal movement remains four-directional on a level:

```text
    ↑

←       →

    ↓
```

Vertical movement is possible through:

```text
Up Stairs
Down Stairs
Up/Down Stairs
Ramps
```

The heuristic includes all three dimensions:

```text
|goal.x - current.x|
+
|goal.y - current.y|
+
|goal.z - current.z|
```

Pathfinding is deterministic and uses stable tie-breaking.

---

# Fixed Timestep Simulation

The simulation runs independently from the SDL renderer.

Current simulation frequency:

```text
10 ticks / second
```

The runtime loop is:

```text
Real Time
    ↓
Accumulator
    ↓
Fixed Simulation Tick
    ↓
Systems
    ↓
World State
    ↓
SDL Renderer
```

This architecture allows rendering speed and simulation speed to remain independent.

It also provides a foundation for:

```text
Pause
Fast-forward
Single stepping
Replays
Headless simulation
Automated testing
Long-term historical simulation
```

---

# Current System Order

A simulation step currently follows roughly:

```text
Designation Deduplication
          ↓
Designation → Jobs
          ↓
Haul Job Generation
          ↓
Excavation Job Assignment
          ↓
Tree-Felling Assignment
          ↓
Hauling Assignment
          ↓
Movement
          ↓
Excavation
          ↓
Tree Felling
          ↓
Hauling
          ↓
Item Spawn Events
          ↓
Item Pickup Events
          ↓
Item Drop Events
          ↓
Additional Haul Generation
          ↓
Water Simulation
          ↓
Tick++
```

Explicit system ordering is important because later systems frequently depend on changes produced earlier in the same simulation tick.

---

# Save / Load

ASCII Universe has persistent JSON saves.

The current save format stores:

- World seed
- World dimensions
- Z-level depth
- Tile shape
- Tile material
- Tile features
- Water type and depth
- Goblins
- Positions
- Professions
- Items and materials
- Item state
- Stockpiles
- Reserved stockpile cells
- Designations
- Jobs
- Job assignments
- Movement paths
- Carried items
- Simulation tick
- Runtime RNG state

The current 3D save format is **version 2**.

Older development saves from the previous 2D format are not compatible.

Default save:

```text
saves/autosave.json
```

---

# Command-Line Options

Create a random world:

```bash
./build/ascii_universe
```

Create a deterministic world:

```bash
./build/ascii_universe --seed 123456
```

Choose a save file:

```bash
./build/ascii_universe \
    --seed 123456 \
    --save saves/world.json
```

Load an existing world:

```bash
./build/ascii_universe \
    --load saves/world.json
```

Run a fixed number of additional simulation ticks:

```bash
./build/ascii_universe \
    --seed 123456 \
    --save saves/test.json \
    --stop-after 100
```

This is useful for deterministic simulation testing.

---

# Architecture

The current architecture is approximately:

```text
ASCII Universe
│
├── SDL Frontend
│   ├── Window
│   ├── ASCII rendering
│   ├── Colors
│   ├── HUD
│   ├── Keyboard events
│   └── Z-level view
│
├── Simulation
│   ├── Fixed timestep
│   ├── System ordering
│   ├── Simulation clock
│   └── Deterministic RNG
│
├── World
│   ├── 3D GameMap
│   ├── Tile geometry
│   ├── Materials
│   ├── Features
│   ├── Liquids
│   └── Procedural generation
│
├── ECS
│   ├── Goblins
│   ├── Items
│   ├── Professions
│   ├── Positions
│   ├── Movement paths
│   └── Designations
│
├── Jobs
│   ├── JobBoard
│   ├── Excavation
│   ├── Tree felling
│   └── Hauling
│
├── AI
│   └── 3D A*
│
├── Environment
│   └── Water
│
└── Persistence
    └── Versioned JSON saves
```

The SDL frontend does not contain simulation rules.

Likewise, simulation systems do not depend on SDL.

That separation is intentional.

---

# Project Structure

```text
ascii-universe/
│
├── CMakeLists.txt
├── README.md
│
├── include/
│   └── ascii/
│       ├── Color.hpp
│       ├── Components.hpp
│       ├── Designations.hpp
│       ├── Events.hpp
│       ├── GameMap.hpp
│       ├── Items.hpp
│       ├── Jobs.hpp
│       ├── Material.hpp
│       ├── Pathfinder.hpp
│       ├── Position.hpp
│       ├── Random.hpp
│       ├── SaveManager.hpp
│       ├── SDLFrontend.hpp
│       ├── Simulation.hpp
│       ├── Stockpiles.hpp
│       ├── Tile.hpp
│       ├── WorldGenerator.hpp
│       │
│       └── systems/
│           ├── DesignationSystems.hpp
│           ├── ExcavationSystems.hpp
│           ├── FellingSystems.hpp
│           ├── HaulingSystems.hpp
│           ├── ItemSystems.hpp
│           ├── MovementSystem.hpp
│           ├── StockpileSystems.hpp
│           ├── SystemUtils.hpp
│           └── WaterSystem.hpp
│
└── src/
    ├── main.cpp
    ├── SaveManager.cpp
    ├── SDLFrontend.cpp
    ├── Simulation.cpp
    ├── WorldGenerator.cpp
    │
    └── systems/
        ├── DesignationSystems.cpp
        ├── ExcavationSystems.cpp
        ├── FellingSystems.cpp
        ├── HaulingSystems.cpp
        ├── ItemSystems.cpp
        ├── MovementSystem.cpp
        ├── StockpileSystems.cpp
        └── WaterSystem.cpp
```

---

# Dependencies

ASCII Universe currently uses:

```text
C++20
CMake
Ninja
EnTT
nlohmann/json
SDL3
```

EnTT and nlohmann/json are fetched through CMake.

SDL3 is currently expected to be installed on the system.

---

# Building

## macOS

Install Apple's development tools if needed:

```bash
xcode-select --install
```

Install dependencies:

```bash
brew install cmake ninja sdl3
```

Clone:

```bash
git clone https://github.com/JarronAnt/ascii-universe.git
cd ascii-universe
```

Configure:

```bash
cmake -S . -B build -G Ninja
```

Build:

```bash
cmake --build build
```

Run:

```bash
./build/ascii_universe
```

---

## Arch Linux / CachyOS

Install dependencies:

```bash
sudo pacman -S --needed \
    base-devel \
    cmake \
    ninja \
    git \
    sdl3
```

Clone and build:

```bash
git clone https://github.com/JarronAnt/ascii-universe.git
cd ascii-universe

cmake -S . -B build -G Ninja
cmake --build build
```

Run:

```bash
./build/ascii_universe
```

---

# Development Workflow

Normal rebuild:

```bash
cmake --build build
```

Run:

```bash
./build/ascii_universe --seed 123456
```

Clean rebuild:

```bash
rm -rf build

cmake -S . -B build -G Ninja

cmake --build build
```

---

# Roadmap

## Phase 1 — Core Simulation

- [x] C++20
- [x] CMake / Ninja
- [x] EnTT ECS
- [x] Fixed timestep
- [x] Deterministic RNG
- [x] Autonomous movement
- [x] A* pathfinding

## Phase 2 — Autonomous Work

- [x] Designations
- [x] Job board
- [x] Worker assignment
- [x] Mining
- [x] Hauling
- [x] Stockpiles
- [x] Item events
- [x] Tree felling
- [x] Dig up/down

## Phase 3 — 3D World

- [x] Dwarf Fortress-style Z-levels
- [x] 3D positions
- [x] Vertical pathfinding
- [x] Stairs
- [x] Ramps
- [x] Material-aware geology
- [x] Mineral veins
- [x] Surface trees
- [x] Water simulation

## Phase 4 — Persistence and Frontend

- [x] Versioned saves
- [x] Save/load world state
- [x] RNG state persistence
- [x] SDL3 frontend
- [x] Colored ASCII
- [x] Z-level viewing
- [x] Pause
- [x] Runtime save controls

## Phase 5 — Playable Fortress Controls

Next major milestone:

- [ ] Camera movement
- [ ] Tile cursor
- [ ] Mouse selection
- [ ] Tile inspection
- [ ] Interactive mining designations
- [ ] Interactive tree-felling designations
- [ ] Interactive dig-up/down designations
- [ ] Stockpile painting
- [ ] Stockpile configuration
- [ ] Cancel designation mode
- [ ] Simulation speed controls
- [ ] Follow selected goblin

The goal of this phase is to remove hardcoded starting work from `main.cpp` and allow the player to create work entirely through the game interface.

## Phase 6 — Construction

- [ ] Builder labor
- [ ] Construction designations
- [ ] Walls
- [ ] Floors
- [ ] Stairs
- [ ] Ramps
- [ ] Doors
- [ ] Furniture
- [ ] Material requirements
- [ ] Construction hauling

## Phase 7 — Workshops and Industry

- [ ] Workshop entities
- [ ] Workshop job queues
- [ ] Carpenter
- [ ] Mason
- [ ] Smelter
- [ ] Forge
- [ ] Metal bars
- [ ] Tools
- [ ] Weapons
- [ ] Furniture
- [ ] Production chains

Example:

```text
Iron Ore
   ↓
Smelter
   ↓
Iron Bar
   ↓
Forge
   ↓
Iron Axe
   ↓
Woodcutter
```

## Phase 8 — Goblin Simulation

- [ ] Skills
- [ ] Labor permissions
- [ ] Needs
- [ ] Hunger
- [ ] Thirst
- [ ] Sleep
- [ ] Mood
- [ ] Personality
- [ ] Relationships
- [ ] Health
- [ ] Injuries
- [ ] Equipment

## Phase 9 — World Simulation

- [ ] Biomes
- [ ] Climate
- [ ] Rivers
- [ ] Weather
- [ ] Wildlife
- [ ] Farming
- [ ] Civilizations
- [ ] Settlements
- [ ] Trade
- [ ] Diplomacy
- [ ] Warfare
- [ ] Historical figures
- [ ] Artifacts
- [ ] Procedural history

---

# Design Philosophy

ASCII Universe is intended to prioritize **simulation over scripting**.

The target is not:

```text
Goblin performs canned event A
then canned event B
then canned event C
```

The target is:

```text
World State
    +
Agent Capabilities
    +
Needs
    +
Jobs
    +
Resources
    +
Environment
    ↓
Emergent Behavior
```

A miner should mine because mining work exists.

A hauler should move iron because an item exists and a valid stockpile needs it.

A woodcutter should cut a tree because the player requested wood.

Eventually, settlements, shortages, accidents, relationships, conflicts, and history should emerge from the same underlying systems.

---

# Development State

ASCII Universe is under active development.

Many systems are intentionally still simple and exist primarily to establish the correct architecture before additional complexity is added.

Current priorities are:

```text
Playable fortress controls
        ↓
Construction
        ↓
Workshops / production
        ↓
Goblin needs and skills
        ↓
Larger world simulation
        ↓
Civilizations and history
```

The project is not currently intended to be a finished or balanced game.

It is the foundation of one.
