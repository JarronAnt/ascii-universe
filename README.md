# ASCII Universe

ASCII Universe is a C++20 colony and world simulation engine inspired by **Dwarf Fortress**, **Goblin Camp**, **KeeperRL**, and traditional roguelikes.

The goal is to progressively build a persistent simulated world in which autonomous agents perform jobs, gather resources, construct settlements, satisfy needs, form relationships, create civilizations, and eventually participate in centuries of procedurally generated history.

The project is being built from the ground up as both a learning project and the foundation for a much larger ASCII simulation game.

---

# Current Milestone

ASCII Universe has now moved beyond being a simple ASCII renderer.

The current simulation supports the complete first autonomous work loop:

```text
Player Mining Designation
          ↓
Designation Validation
          ↓
Duplicate Detection
          ↓
Mining Job Creation
          ↓
Job Board
          ↓
Profession Check
          ↓
Worker Assignment
          ↓
A* Pathfinding
          ↓
Autonomous Movement
          ↓
Mining Execution
          ↓
Wall Becomes Floor
          ↓
Job Complete
```

The player does not directly command individual goblins.

Instead, the player creates work that qualified agents autonomously claim and perform.

This is the foundation for the colony simulation architecture used throughout the rest of the project.

---

# Current Features

## Core Engine

- [x] C++20
- [x] CMake build system
- [x] Ninja support
- [x] macOS support
- [x] Linux support
- [x] Tile-based world
- [x] Terminal ASCII rendering
- [x] ECS architecture using EnTT
- [x] Separation between simulation and rendering

---

## Simulation

- [x] Fixed timestep simulation
- [x] 10 simulation ticks per second
- [x] Real-time accumulator
- [x] Manual single-step capable simulation architecture
- [x] Deterministic system ordering
- [x] Simulation tick counter
- [x] Seeded deterministic random number generator

The renderer and simulation run independently.

```text
Real Time
    ↓
Accumulator
    ↓
Fixed Simulation Tick
    ↓
Simulation Systems
    ↓
World State
    ↓
Renderer
```

This allows the simulation to eventually support:

- Pause
- Single-step debugging
- Fast-forward
- Replays
- Historical simulation
- Headless simulation
- Automated testing

without tying simulation behavior to rendering speed.

---

# Deterministic Randomness

World randomness is controlled through a seeded pseudo-random number generator.

Example:

```cpp
constexpr std::uint64_t WorldSeed =
    123456789ULL;

Simulation simulation{
    40,
    20,
    WorldSeed
};
```

A simulation started with the same seed begins with the same deterministic random sequence.

This will eventually be used for:

```text
World Seed
   ├── terrain
   ├── climate
   ├── rivers
   ├── geology
   ├── creatures
   ├── civilizations
   ├── artifacts
   ├── personalities
   └── history
```

The RNG state is designed so it can eventually be serialized along with world saves.

---

# World Map

The current world uses a contiguous two-dimensional tile grid.

Current tile types:

```text
# = Wall
. = Floor
```

Example:

```text
########################################
#......................................#
#......................................#
#.......................#######........#
#.......................#######........#
#.......................#######........#
#......................................#
########################################
```

Tiles expose properties such as whether they can be walked across.

The current world is two-dimensional, but the architecture is intended to eventually support Dwarf Fortress-style Z levels.

---

# Entity Component System

ASCII Universe uses **EnTT** for its entity-component system.

Entities represent identity while components store data.

For example, a miner might contain:

```text
Entity #12
├── Goblin
├── Miner
├── Name("Uru")
├── Position
├── Glyph('g')
└── AssignedJob
```

Components currently include or support:

```text
Name
Goblin
Glyph
Position
MovementPath
Miner
Hauler
AssignedJob
MineDesignation
DesignationLifecycle
```

This allows systems to operate only on the entities containing the data they need.

---

# Professions

Goblin capabilities are represented through ECS marker components.

Examples:

```cpp
struct Miner {};
struct Hauler {};
```

A goblin can therefore possess multiple professions:

```text
Goblin
├── Miner
├── Hauler
└── Builder
```

rather than being restricted to one large profession enum.

Currently implemented:

- [x] Miner
- [x] Hauler

Currently only miners have executable work.

Haulers will become active when the item and stockpile systems are implemented.

Planned professions include:

- [ ] Builder
- [ ] Farmer
- [ ] Carpenter
- [ ] Smith
- [ ] Cook
- [ ] Doctor
- [ ] Soldier

---

# A* Pathfinding

The project contains a working A* pathfinder.

Movement currently supports:

```text
    ↑

←       →

    ↓
```

Diagonal movement is not currently enabled.

The pathfinder uses Manhattan distance:

```text
|goal.x - current.x|
+
|goal.y - current.y|
```

as its heuristic.

A path:

- Does not contain the starting tile
- Does contain the destination
- Rejects non-walkable destinations
- Avoids wall tiles
- Uses deterministic tie-breaking

Example:

```text
####################
#..................#
#.g................#
#.......#####......#
#...........#......#
#...........#......#
#...........#......#
#...............X..#
#..................#
####################
```

The goblin calculates a path around obstacles rather than attempting to move through them.

---

# Movement

Entities can receive a `MovementPath` component containing:

```text
Path Nodes
Current Path Index
```

The movement system executes during fixed simulation ticks.

Currently:

```text
1 movement step
=
1 simulation tick
```

The architecture will later allow different movement speeds without changing the global simulation tick rate.

Examples:

```text
Goblin
1 tile every tick

Armoured Goblin
1 tile every 2 ticks

Wolf
2 movement actions per tick
```

---

# Designations

The project now supports player-created mining designations.

Example:

```text
#######
##X####
##X####
##X####
#######
```

`X` indicates a wall selected for mining.

A mining designation is represented by an ECS entity containing:

```text
Position
MineDesignation
DesignationLifecycle
Glyph
```

---

# Designation Lifecycle

Designations currently have three states:

```text
Active
Ignored
Consumed
```

### Active

The designation is valid and awaiting conversion into a job.

### Ignored

The designation was invalid or was a duplicate of another designation.

### Consumed

The designation has already produced a job.

---

# Designation Deduplication

Multiple mining designations cannot create multiple jobs for the same wall.

For example:

```cpp
designateMine({24, 8});
designateMine({24, 8});
designateMine({24, 8});
```

will produce only one valid mining operation.

Conceptually:

```text
Designation A ──→ Active

Designation B ──→ Ignored

Designation C ──→ Ignored
```

Deduplication happens before jobs are generated.

This prevents duplicate agents from trying to perform the same operation.

---

# Job Board

The simulation now owns a central `JobBoard`.

Jobs have:

```text
Job ID
Job Type
Target Position
Work Position
State
Worker
Source Designation
```

Current job types:

```text
Mine
Haul
```

Current job states:

```text
Available
Assigned
Complete
Cancelled
```

Example job:

```text
Job #4

Type:
    Mine

Target:
    (24, 8)

Work Position:
    (23, 8)

State:
    Assigned

Worker:
    Uru
```

The job target and worker position are deliberately different.

A mining target is a wall:

```text
#
```

which cannot be walked onto.

The miner therefore finds a valid adjacent floor tile:

```text
g#
```

and performs the mining operation from there.

---

# Autonomous Worker Assignment

Workers now autonomously claim jobs based on their profession.

For example:

```text
Uru
├── Goblin
└── Miner
```

can accept:

```text
Mine
```

while:

```text
Kesh
├── Goblin
└── Hauler
```

will ignore mining jobs.

The assignment pipeline is:

```text
Available Job
     ↓
Find Idle Goblin
     ↓
Check Profession
     ↓
Find Valid Work Position
     ↓
Check Reachability
     ↓
Run A*
     ↓
Assign Worker
```

Workers therefore do not need to be directly commanded by the player.

---

# Mining

Mining is the first fully autonomous job type implemented in ASCII Universe.

The complete current pipeline is:

```text
Player
   ↓
Mine Wall
   ↓
MineDesignation
   ↓
Designation Deduplication
   ↓
Create Mine Job
   ↓
JobBoard
   ↓
Find Idle Miner
   ↓
Find Adjacent Walkable Tile
   ↓
A* Path
   ↓
MovementPath
   ↓
Goblin Walks
   ↓
Goblin Reaches Work Position
   ↓
MiningSystem
   ↓
Wall → Floor
   ↓
Job Complete
```

Example:

Before:

```text
.......#
.......#
.....g.#
.......#
```

After the miner arrives:

```text
........
......g.
........
........
```

The world has actually changed.

The wall was not merely visually hidden.

Its underlying tile changed from:

```cpp
TileType::Wall
```

to:

```cpp
TileType::Floor
```

making it walkable for future pathfinding.

---

# System Ordering

Simulation systems currently run in a deterministic order.

Conceptually:

```text
Simulation::step()
│
├── designationDedupSystem()
│
├── designationToJobsSystem()
│
├── jobAssignmentSystem()
│
├── movementSystem()
│
├── miningSystem()
│
└── tick++
```

Ordering is important.

For example:

```text
Designation Deduplication
```

must happen before:

```text
Designation → Job
```

otherwise duplicate jobs could be created.

As more systems are introduced, the simulation tick will grow into something similar to:

```text
Input Commands
      ↓
Designation Processing
      ↓
Job Generation
      ↓
Job Assignment
      ↓
AI
      ↓
Path Planning
      ↓
Movement
      ↓
Job Execution
      ↓
Item Events
      ↓
Hauling
      ↓
Needs
      ↓
Environment
      ↓
Cleanup
      ↓
Time
```

---

# Terminal Renderer

The current renderer uses the terminal as an ASCII framebuffer.

The rendering process is:

```text
Create Character Buffer
        ↓
Draw Terrain
        ↓
Draw Entities
        ↓
Draw Designations
        ↓
Present Buffer
```

This means simulation code does not depend on the terminal renderer.

Eventually:

```text
Simulation
    │
    ├── TerminalRenderer
    │
    └── SDLRenderer
```

can display exactly the same underlying simulation.

---

# Current Demo

The current development demo contains:

```text
Uru
    Miner

Kesh
    Hauler
```

The player creates several mining designations.

Example:

```text
########################################
#......................................#
#......................................#
#.......................#######........#
#.......................X######........#
#.......................X######........#
#....g..................X######........#
#.......................X######........#
#....g..................#######........#
#.......................#######........#
#......................................#
########################################
```

Uru automatically:

```text
finds mining work
      ↓
claims a job
      ↓
calculates a path
      ↓
walks to the wall
      ↓
mines it
      ↓
claims the next job
```

Kesh does nothing because Kesh is currently a hauler and there are no hauling jobs yet.

That is intentional.

---

# Architecture

The project currently resembles:

```text
ASCII Universe
│
├── Simulation
│   │
│   ├── Fixed Timestep
│   ├── Simulation Time
│   ├── Random
│   └── System Ordering
│
├── World
│   │
│   ├── GameMap
│   └── Tiles
│
├── ECS
│   │
│   ├── Goblins
│   ├── Positions
│   ├── Professions
│   ├── Movement Paths
│   └── Designations
│
├── AI
│   │
│   └── A* Pathfinder
│
├── Jobs
│   │
│   ├── JobBoard
│   ├── Job Assignment
│   ├── Mine Jobs
│   └── Haul Jobs [planned execution]
│
├── Systems
│   │
│   ├── Designation Deduplication
│   ├── Designation → Job
│   ├── Job Assignment
│   ├── Movement
│   └── Mining
│
└── Rendering
    │
    └── TerminalRenderer
```

---

# Project Structure

The project is currently organized around headers in the `ascii` namespace:

```text
ascii-universe/
│
├── CMakeLists.txt
│
├── include/
│   └── ascii/
│       ├── Components.hpp
│       ├── Designations.hpp
│       ├── GameMap.hpp
│       ├── Jobs.hpp
│       ├── Pathfinder.hpp
│       ├── Position.hpp
│       ├── Random.hpp
│       ├── Simulation.hpp
│       ├── TerminalRenderer.hpp
│       └── Tile.hpp
│
├── src/
│   └── main.cpp
│
└── build/
```

As the project grows, large systems will eventually be moved into separate source files.

A future structure may look like:

```text
ascii-universe/
│
├── include/ascii/
│   ├── ai/
│   ├── core/
│   ├── ecs/
│   ├── items/
│   ├── jobs/
│   ├── persistence/
│   ├── render/
│   ├── simulation/
│   ├── systems/
│   └── world/
│
├── src/
│   ├── ai/
│   ├── items/
│   ├── jobs/
│   ├── render/
│   ├── simulation/
│   ├── systems/
│   └── world/
│
├── tests/
├── assets/
└── saves/
```

---

# Dependencies

Current dependencies:

- C++20
- CMake
- Ninja
- EnTT
- nlohmann/json

Dependencies are fetched through CMake.

Planned future dependencies:

- SDL3
- SDL3_ttf

SDL will eventually provide a richer graphical ASCII interface while preserving the same simulation layer.

---

# Building

## macOS

Install Apple's command-line development tools:

```bash
xcode-select --install
```

Install CMake and Ninja:

```bash
brew install cmake ninja
```

Clone the repository:

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

Install the required development tools:

```bash
sudo pacman -S --needed base-devel cmake ninja git
```

Clone and build:

```bash
git clone https://github.com/JarronAnt/ascii-universe.git
cd ascii-universe

cmake -S . -B build -G Ninja
cmake --build build

./build/ascii_universe
```

---

## Ubuntu / Debian

Install:

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    git
```

Then:

```bash
git clone https://github.com/JarronAnt/ascii-universe.git
cd ascii-universe

cmake -S . -B build -G Ninja
cmake --build build

./build/ascii_universe
```

---

# Development Workflow

After the initial CMake configuration:

```bash
cmake --build build && ./build/ascii_universe
```

For a clean rebuild:

```bash
rm -rf build

cmake -S . -B build -G Ninja

cmake --build build

./build/ascii_universe
```

---

# Roadmap

## Phase 1 — Core Simulation

- [x] C++20 project
- [x] CMake
- [x] Tile map
- [x] Terminal renderer
- [x] ECS
- [x] Goblin entities
- [x] Fixed timestep simulation
- [x] Deterministic RNG
- [x] A* pathfinding
- [x] Autonomous movement

**Complete**

---

## Phase 2 — Colony Work System

- [x] Mining designations
- [x] Designation lifecycle
- [x] Duplicate designation detection
- [x] Central job board
- [x] Job states
- [x] Worker assignment
- [x] Miner profession
- [x] Hauler profession
- [x] Reachability checks
- [x] Mining execution
- [x] Terrain mutation
- [x] Sequential autonomous jobs

**Complete**

---

## Phase 3 — Items and Logistics

Next milestone:

- [ ] Item entities
- [ ] Item types
- [ ] Item spawn events
- [ ] Stone generated from mining
- [ ] Ground item state
- [ ] Carrying state
- [ ] Stockpile zones
- [ ] Stockpile filters
- [ ] Hauling job generation
- [ ] Automatic hauler assignment
- [ ] Item pickup
- [ ] Item transport
- [ ] Item drop-off
- [ ] Item reservations

The target pipeline is:

```text
Mine Wall
    ↓
Stone Spawn Event
    ↓
Stone Entity
    ↓
Ground Item
    ↓
Find Stockpile
    ↓
Create Haul Job
    ↓
Find Hauler
    ↓
Walk to Stone
    ↓
Pick Up
    ↓
Walk to Stockpile
    ↓
Drop Stone
```

This will make the currently idle Hauler profession functional.

---

## Phase 4 — Production

- [ ] Workshops
- [ ] Workshop jobs
- [ ] Recipes
- [ ] Material requirements
- [ ] Job dependencies
- [ ] Resource reservations
- [ ] Crafting
- [ ] Smelting
- [ ] Smithing
- [ ] Carpentry
- [ ] Farming
- [ ] Cooking

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
Sword
```

---

## Phase 5 — Goblin Simulation

- [ ] Hunger
- [ ] Thirst
- [ ] Sleep
- [ ] Health
- [ ] Injuries
- [ ] Mood
- [ ] Skills
- [ ] Experience
- [ ] Personality
- [ ] Relationships
- [ ] Memories
- [ ] Social interaction
- [ ] Utility AI

Eventually goblins will choose between competing desires:

```text
Mining Job     48 utility
Eat            91 utility
Sleep          31 utility
Socialize      15 utility

→ Eat
```

---

## Phase 6 — Fortress Systems

- [ ] Construction
- [ ] Buildings
- [ ] Doors
- [ ] Furniture
- [ ] Rooms
- [ ] Ownership
- [ ] Zones
- [ ] Farming
- [ ] Food storage
- [ ] Equipment
- [ ] Weapons
- [ ] Combat

---

## Phase 7 — Persistence

- [ ] Save files
- [ ] Load files
- [ ] Save schema version
- [ ] Entity serialization
- [ ] Map serialization
- [ ] Job serialization
- [ ] RNG state serialization
- [ ] Save migration support

---

## Phase 8 — Procedural World Generation

Planned pipeline:

```text
World Seed
    ↓
Elevation
    ↓
Sea Level
    ↓
Temperature
    ↓
Rainfall
    ↓
Drainage
    ↓
Rivers
    ↓
Lakes
    ↓
Erosion
    ↓
Geology
    ↓
Biomes
    ↓
Flora
    ↓
Fauna
    ↓
Civilization Sites
```

---

## Phase 9 — Z Levels

Current position:

```cpp
struct Position
{
    int x;
    int y;
};
```

Future:

```cpp
struct Position
{
    int x;
    int y;
    int z;
};
```

Allowing:

```text
Z +2    Sky
Z +1    Tree Canopy / Hills
Z  0    Surface
Z -1    Soil
Z -2    Stone
Z -3    Mineral Layers
Z -4    Caverns
Z -5    Deep Underground
```

---

## Phase 10 — World History

World generation will eventually continue beyond terrain generation.

```text
Generate World
      ↓
Generate Species
      ↓
Generate Civilizations
      ↓
Generate Settlements
      ↓
Generate Historical Figures
      ↓
Simulate Population
      ↓
Simulate Expansion
      ↓
Simulate Politics
      ↓
Simulate Trade
      ↓
Simulate Wars
      ↓
Simulate Collapse
      ↓
Generate Ruins
      ↓
Player Enters World
```

Historical events will exist as simulation data rather than prewritten lore.

Example:

```text
YEAR 74

Khar Molun was founded by
King Varak Ironhand.


YEAR 102

The Ashen Tribes declared war
on Khar Molun.


YEAR 108

The Battle of Red Ford occurred.


YEAR 114

Khar Molun fell.


YEAR 151

Mazra founded the Ashen Empire.
```

The player may later discover:

```text
Ruins of Khar Molun
```

because Khar Molun actually existed and was destroyed during world simulation.

---

# Long-Term Goal

The final goal is not simply to create an ASCII roguelike.

It is to build a persistent simulation capable of operating at multiple scales:

```text
Universe
   ↓
Galaxy
   ↓
Star System
   ↓
Planet
   ↓
Continent
   ↓
Region
   ↓
Civilization
   ↓
Settlement
   ↓
Fortress
   ↓
Individual Agent
```

Nearby entities can eventually run detailed simulations while distant locations use increasingly abstract simulation models.

This should allow a large world to remain alive without requiring every individual entity in the universe to run full-resolution AI every simulation tick.

---

# Development Philosophy

## Simulation First

Gameplay systems should modify simulation state.

The renderer should only display that state.

## Autonomous Agents

The player creates intent:

```text
Mine this.
Build this.
Store this.
Craft this.
```

Agents determine how to execute that intent.

## Determinism

The same world seed and sequence of player commands should produce reproducible simulation behavior wherever practical.

## Emergent Systems

Complex events should emerge from interacting systems rather than depending entirely on scripted sequences.

## Incremental Complexity

A system should work end-to-end before another major simulation layer is added.

For example, mining was completed as:

```text
Designation
    ↓
Job
    ↓
Assignment
    ↓
Path
    ↓
Movement
    ↓
Execution
```

before introducing items or hauling.

The next system will extend that existing chain instead of bypassing it.

---

# Current Status

```text
CORE ENGINE
[✓] Map
[✓] ASCII framebuffer renderer
[✓] ECS
[✓] Entities
[✓] Fixed timestep
[✓] Deterministic RNG

NAVIGATION
[✓] A*
[✓] Obstacle avoidance
[✓] Movement paths
[✓] Autonomous movement

WORK SYSTEM
[✓] Designations
[✓] Designation lifecycle
[✓] Duplicate prevention
[✓] Job board
[✓] Job states
[✓] Job assignment
[✓] Profession filtering

PROFESSIONS
[✓] Miner
[✓] Hauler

MINING
[✓] Mining jobs
[✓] Adjacent work-position selection
[✓] Pathfinding to mining locations
[✓] Autonomous mining
[✓] Wall → floor mutation

LOGISTICS
[ ] Items
[ ] Stone drops
[ ] Stockpiles
[ ] Hauling
[ ] Reservations

WORLD
[ ] Procedural terrain
[ ] Biomes
[ ] Geology
[ ] Z levels
[ ] Civilizations
[ ] Historical simulation

PERSISTENCE
[ ] Save
[ ] Load
```

---

# Next Milestone

The next milestone is the first complete resource logistics loop:

```text
Uru mines wall
      ↓
Stone appears
      ↓
Stone needs storage
      ↓
Haul job generated
      ↓
Kesh claims job
      ↓
Kesh walks to stone
      ↓
Kesh picks it up
      ↓
Kesh walks to stockpile
      ↓
Kesh drops stone
      ↓
Stone becomes stockpiled
```

Once that works, ASCII Universe will have its first complete **resource economy loop** rather than only a work-order system.

---

# Inspiration

ASCII Universe is inspired by:

- Dwarf Fortress
- Goblin Camp
- KeeperRL
- Traditional roguelikes
- Colony simulation games

ASCII Universe is an independent project and is not affiliated with those games or their developers.

---

# Status

**Early development — autonomous mining milestone complete.**

Current focus:

**Items → mined resources → stockpiles → autonomous hauling.**
