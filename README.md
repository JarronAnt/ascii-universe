# ASCII Universe

ASCII Universe is a C++20 simulation engine project inspired by games such as **Dwarf Fortress**, **Goblin Camp**, and traditional roguelikes.

The long-term goal is to build a persistent ASCII world simulation featuring autonomous agents, jobs, mining, hauling, stockpiles, procedural world generation, civilizations, history, and eventually a large-scale simulated universe.

The project is being built from the ground up as a learning project, with an emphasis on clean architecture and simulation systems rather than simply creating a small roguelike.

---

## Current Features

The engine currently supports:

- 2D tile-based maps
- Floor and wall tiles
- Terminal ASCII rendering
- ANSI terminal screen clearing
- ECS-based entities using EnTT
- Entity components
- Goblin entities
- Entity positions
- Entity glyph rendering
- CMake-based builds
- Cross-platform development on macOS and Linux

Example output:

```text
########################################
#......................................#
#......................................#
#......................................#
#......................................#
#.........g............................#
#......................................#
#......................................#
########################################
```

Where:

```text
# = Wall
. = Floor
g = Goblin
```

---

# Goals

The goal is to progressively develop the project from a simple ASCII renderer into a full colony and world simulation engine.

The intended gameplay model is similar to Dwarf Fortress.

Instead of directly controlling individual units, the player issues orders such as:

```text
Mine this wall
Build here
Create a stockpile
Craft an item
```

Agents then autonomously determine how to complete those tasks.

For example:

```text
Mining Designation
        ↓
Mining Job
        ↓
Worker Assignment
        ↓
Pathfinding
        ↓
Goblin Movement
        ↓
Mining
        ↓
Stone Created
        ↓
Hauling Job
        ↓
Stockpile
```

---

# Planned Systems

Development is planned roughly in the following order.

## Core Engine

- [x] C++20 project
- [x] CMake build system
- [x] Tile map
- [x] Terminal renderer
- [x] ECS entity system
- [x] Entity rendering
- [x] Fixed timestep simulation
- [x] Deterministic random number generation
- [ ] Event system

---

## Movement and Navigation

- [x] Autonomous movement
- [x] A* pathfinding
- [x] Path caching
- [ ] Collision and occupancy
- [ ] Movement costs

Eventually:

- [ ] Flow-field pathfinding
- [ ] Multi-level pathfinding
- [ ] Large-scale pathfinding optimizations

---

## Jobs and Designations

- [ ] Mining designations
- [ ] Designation lifecycle
- [ ] Job board
- [ ] Worker job assignment
- [ ] Job priorities
- [ ] Job cancellation
- [ ] Worker professions
- [ ] Work skill levels

---

## Mining

- [ ] Select walls for mining
- [ ] Generate mining jobs
- [ ] Goblins navigate to jobs
- [ ] Convert walls into floors
- [ ] Spawn mined resources

---

## Items

- [ ] Item entities
- [ ] Materials
- [ ] Item ownership
- [ ] Item states
- [ ] Carrying
- [ ] Item stacks
- [ ] Equipment

Example materials may include:

```text
Stone
Wood
Iron Ore
Coal
Food
Weapons
Tools
Furniture
```

---

## Stockpiles and Hauling

- [ ] Stockpile zones
- [ ] Stockpile filters
- [ ] Automatic hauling jobs
- [ ] Worker carrying
- [ ] Storage capacity
- [ ] Item reservations

The eventual flow should look like:

```text
Mine stone
    ↓
Stone appears on ground
    ↓
Hauling job generated
    ↓
Hauler finds stone
    ↓
Stone transported
    ↓
Stone stored in stockpile
```

---

## Agent Simulation

Agents will eventually have their own state and needs.

Planned systems include:

- [ ] Hunger
- [ ] Thirst
- [ ] Sleep
- [ ] Health
- [ ] Mood
- [ ] Skills
- [ ] Personality
- [ ] Relationships
- [ ] Memories
- [ ] Social interactions

Agents will use utility-based decision making to determine whether they should work, eat, sleep, socialize, or perform other actions.

---

## Production

Planned production systems include:

- [ ] Workshops
- [ ] Recipes
- [ ] Resource requirements
- [ ] Crafting
- [ ] Farming
- [ ] Food production
- [ ] Smelting
- [ ] Smithing

For example:

```text
Iron Ore
   ↓
Smelter
   ↓
Iron Bars
   ↓
Forge
   ↓
Sword
```

---

# Procedural World Generation

The long-term world generator will create more than random terrain.

Planned generation layers include:

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
Water Flow
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
Vegetation
    ↓
Wildlife
    ↓
Civilization Sites
```

The same seed should always generate the same world.

Example:

```text
^^^^^^^^^^^^^^TTTTTTTT~~~~~~~~
^^^^^^^^^^^^TTTTTTTTT~~~~~~~~~
^^^^^^^..TTTTTTTTTTT~~~~~~~~~
^^^^^.....TTTTTTTTT~~~~~~~~~~~
^^^^.......TTTTT~~~~~~~≈≈~~~~~
^^.........TT~~~~~~~~≈≈≈~~~~~~
...........~~~~~~~~~~≈~~~~~~~~~
```

---

# Z Levels

The current engine uses two-dimensional positions:

```cpp
struct Position
{
    int x;
    int y;
};
```

Eventually this will become:

```cpp
struct Position
{
    int x;
    int y;
    int z;
};
```

allowing worlds such as:

```text
Z +2  Sky
Z +1  Trees / Hills
Z  0  Surface
Z -1  Soil
Z -2  Stone
Z -3  Ore Deposits
Z -4  Caverns
Z -5  Deep Underground
```

---

# Civilization Simulation

One of the eventual goals is to simulate a world before the player enters it.

World generation will create:

- Species
- Civilizations
- Settlements
- Historical figures
- Governments
- Religions
- Armies
- Wars
- Alliances
- Artifacts
- Ruins

The game can then simulate hundreds of years of history.

For example:

```text
YEAR 72

King Varak founded Khar Molun.


YEAR 103

The Ashen Tribes declared war on Khar Molun.


YEAR 108

The Battle of Red Ford occurred.


YEAR 114

Khar Molun fell.


YEAR 147

Queen Mazra founded the Ashen Empire.
```

These events would be generated by the simulation rather than written beforehand.

The player may later discover:

```text
Ruins of Khar Molun
```

because that settlement actually existed during world history.

---

# Project Structure

Current structure:

```text
ascii-universe/
├── CMakeLists.txt
│
├── include/
│   └── ascii/
│       ├── Components.hpp
│       ├── GameMap.hpp
│       ├── Position.hpp
│       ├── TerminalRenderer.hpp
│       └── Tile.hpp
│
├── src/
│   └── main.cpp
│
└── build/
```

The project will eventually grow toward something like:

```text
ascii-universe/
├── CMakeLists.txt
│
├── assets/
│
├── include/ascii/
│   ├── ai/
│   ├── core/
│   ├── ecs/
│   ├── jobs/
│   ├── persistence/
│   ├── render/
│   ├── simulation/
│   ├── systems/
│   └── world/
│
├── src/
├── tests/
└── saves/
```

---

# Dependencies

The project currently uses:

- C++20
- CMake
- EnTT
- nlohmann/json

Dependencies are retrieved automatically through CMake `FetchContent`.

Future versions may also use:

- SDL3
- SDL3_ttf

SDL will eventually provide a graphical ASCII renderer while keeping the simulation independent from rendering.

---

# Building

## Requirements

You need:

```text
C++20 compiler
CMake
Ninja
Git
```

---

## macOS

Install the Apple development tools:

```bash
xcode-select --install
```

Install CMake and Ninja:

```bash
brew install cmake ninja
```

Clone the project:

```bash
git clone <repository-url>
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
sudo pacman -S --needed base-devel cmake ninja git
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

## Ubuntu / Debian

Install dependencies:

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
cmake -S . -B build -G Ninja
cmake --build build
./build/ascii_universe
```

---

# Development

After the initial CMake configuration, the normal development cycle is:

```bash
cmake --build build && ./build/ascii_universe
```

To rebuild from scratch:

```bash
rm -rf build

cmake -S . -B build -G Ninja

cmake --build build
```

---

# Architecture

ASCII Universe separates the simulation from the renderer.

```text
Simulation
    │
    ├── World
    ├── ECS
    ├── AI
    ├── Jobs
    ├── Items
    └── Events

        ↓

Renderer

    ├── Terminal Renderer
    └── SDL Renderer
```

This means the simulation should not depend on the terminal.

The terminal is simply one way to view the world.

---

# ECS

Entities are implemented using EnTT.

An entity represents an identity.

Components contain its data.

For example:

```text
Entity #1
├── Goblin
├── Name("Uru")
├── Position(10, 8)
└── Glyph('g')
```

Possible components include:

```cpp
struct Goblin {};
struct Miner {};
struct Hauler {};
struct Position {};
struct Name {};
struct Glyph {};
struct Inventory {};
struct Needs {};
struct AssignedJob {};
```

Systems operate on entities containing specific components.

For example:

```text
MovementSystem
MiningSystem
HaulingSystem
JobAssignmentSystem
NeedsSystem
```

---

# Design Philosophy

ASCII Universe follows several core principles.

### Simulation first

The renderer should never contain gameplay logic.

### Determinism

A given seed and set of commands should produce the same simulation whenever possible.

### Data-oriented architecture

World state should remain reasonably cache-friendly and efficient enough to eventually support large numbers of simulated entities.

### Emergent behavior

Interesting events should emerge from interacting systems instead of relying entirely on scripted sequences.

### Incremental complexity

Systems should be completed and tested before adding more simulation layers.

The immediate goal is therefore not civilizations or procedural history.

The current target is:

```text
Wall
 ↓
Mining Designation
 ↓
Mining Job
 ↓
Worker
 ↓
Pathfinding
 ↓
Mining
 ↓
Stone
 ↓
Hauling Job
 ↓
Stockpile
```

Once that entire loop works autonomously, the project has the foundation of a true colony simulation.

---

# Inspiration

ASCII Universe is inspired by:

- Dwarf Fortress
- Goblin Camp
- KeeperRL
- Traditional roguelikes
- Colony simulation games

This project is an independent learning project and is not affiliated with those games or their developers.

---

# Status

Early development.

Current milestone:

```text
[✓] Map
[✓] ASCII renderer
[✓] ECS
[✓] Goblin entity rendering
[ ] Simulation loop
[ ] Autonomous movement
[ ] A* pathfinding
[ ] Mining designations
[ ] Jobs
[ ] Mining
[ ] Items
[ ] Hauling
[ ] Stockpiles
```

Next major milestone:

**Autonomous goblin movement and A* pathfinding.**
