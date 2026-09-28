# ASCII Universe

ASCII Universe is a **C++20 colony and world simulation game** inspired by **Dwarf Fortress**, **Goblin Camp**, **KeeperRL**, and classic roguelikes.

The goal is to build a persistent, systemic fortress simulation where autonomous inhabitants mine, gather resources, haul items, construct settlements, operate workshops, satisfy needs, form relationships, interact with civilizations, and eventually participate in procedurally generated history.

The project is designed around indirect player control:

```text
Player Intent
     ↓
Designations / Orders
     ↓
Jobs
     ↓
Autonomous Goblins
     ↓
Simulation Systems
     ↓
Persistent World State
     ↓
SDL ASCII Frontend
```

The player does not directly steer individual goblins. Instead, the player designates work, creates stockpiles, changes priorities through labor roles, and shapes the fortress while qualified goblins autonomously claim and perform jobs.

---

# Current Development State

ASCII Universe currently includes a playable simulation foundation with:

- A full 3D Z-level world
- SDL3 ASCII rendering
- Camera movement and tile cursor
- Mouse-based area selection
- Interactive mining designations
- Tree-felling designations
- Dig-up and dig-down designations
- Interactive stockpile creation and configuration
- Designation cancellation
- Goblin selection and follow-camera mode
- Pause and multiple simulation speeds
- Autonomous jobs and worker assignment
- 3D A* pathfinding
- Cross-Z-level hauling
- Material-aware items
- Persistent JSON saves
- Deterministic seeded world generation
- Large-scale landforms
- Climate variation
- Geological strata
- Host-rock-aware ore generation
- Rivers and lakes
- Regional forests
- Underground cavern systems
- Runtime water simulation
- Natural embark-site selection

New worlds are currently generated at:

```text
96 × 64 × 24
```

with a visible fortress viewport of:

```text
48 × 24
```

The next major gameplay milestone is **fortress construction**.

---

# Gameplay Loop

The current playable loop is:

```text
Generate World
      ↓
Inspect Embark
      ↓
Create Stockpile
      ↓
Designate Trees
      ↓
Woodcutter Fells Trees
      ↓
Hauler Stores Logs
      ↓
Designate Excavation
      ↓
Miner Excavates
      ↓
Stone / Soil / Ore Items Spawn
      ↓
Hauler Moves Resources
      ↓
Expand Fortress
```

The player now issues these orders entirely through the game frontend rather than through hard-coded test jobs.

---

# World Model

ASCII Universe uses a 3D tile volume.

Each horizontal slice is a separate Z-level:

```text
Z 23    Air / peaks
Z 22    Surface
Z 21    Soil / exposed rock
Z 20    Upper geology
...
Z 12    Rock strata
...
Z 6     Deep geology / caverns
...
Z 0     Deepest world layer
```

Only one Z-level is rendered at a time, but the entire world continues simulating.

A goblin can therefore:

```text
walk across surface
      ↓
descend stairs
      ↓
enter underground fortress
      ↓
cross cavern
      ↓
mine ore
      ↓
return to surface stockpile
```

without changing simulation models.

---

# Tile System

Tiles separate geometry, material, features, and liquids.

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

This allows combinations such as:

```text
Granite Wall
Shale Floor
Iron Ore Wall
Loam Floor
Basalt Ramp
Gold Ore Wall
```

without needing a separate tile type for every material.

---

# Phase 5 — Playable Fortress Controls

The player can now issue fortress orders directly through the SDL interface.

## Camera

```text
WASD            Pan camera
Home            Center camera on cursor
```

## Cursor

```text
Arrow Keys      Move tile cursor
Mouse           Move/select tiles
```

## Z-Levels

```text
[ / , / PgDn    Move down one Z-level
] / . / PgUp    Move up one Z-level
Mouse Wheel     Change Z-level
```

## Work Modes

```text
I               Inspect
M               Mine
T               Fell trees
V               Dig down
U               Dig up
P               Create stockpile
X               Cancel designation
```

## Selection

```text
Left Click      Select / begin designation
Left Drag       Paint rectangular designation
Enter           Apply current mode to cursor
Right Click     Cancel mode / return to Inspect
Escape          Cancel mode, or exit from Inspect mode
```

## Stockpiles

```text
R               Cycle stockpile filter
C               Configure stockpile under cursor
```

Current stockpile presets include:

```text
All
Stone + Ore
Stone
Ore
Soil
Logs
```

## Goblins

```text
Tab             Cycle selected goblin
G               Select goblin at cursor
F               Follow selected goblin
```

## Simulation

```text
Space           Pause / resume

1               1x speed
2               2x speed
3               4x speed
4               8x speed

F5              Save
Q               Save and quit
```

---

# Autonomous Job System

Player commands create designations rather than directly changing the world.

For example:

```text
Player paints mining area
        ↓
Mine Designations
        ↓
Job Generation
        ↓
Available Mine Jobs
        ↓
Miner searches for work
        ↓
Reachability check
        ↓
3D A*
        ↓
Worker assigned
        ↓
Movement
        ↓
Excavation
        ↓
Resource item created
```

Jobs currently include:

```text
Mine
Dig Down
Dig Up
Fell Tree
Haul
```

Job states include:

```text
Available
Assigned
Complete
Cancelled
```

Cancelling a designation also cancels its unfinished generated job and releases its worker.

---

# Mining

Mining removes solid terrain and produces an item based on the material being mined.

Examples:

```text
Granite
   ↓
Stone Item
```

```text
Shale
   ↓
Stone Item
```

```text
Iron Ore
   ↓
Ore Item
```

```text
Gold Ore
   ↓
Ore Item
```

```text
Loam
   ↓
Soil Item
```

The resulting resources exist as actual ECS entities rather than abstract inventory counters.

---

# Vertical Excavation

The fortress can expand vertically.

## Dig Down

```text
Z 14      .
           │
           ▼
Z 13      #
```

becomes:

```text
Z 14      >
           │
           ▼
Z 13      <
```

## Dig Up

The inverse allows underground miners to create access upward.

Repeated excavation can create full shafts:

```text
Z 15      >
           │
Z 14      X
           │
Z 13      X
           │
Z 12      <
```

The pathfinder uses these vertical connections automatically.

---

# Tree Felling

Trees are world features attached to surface tiles.

The current logging pipeline is:

```text
Player designates tree
        ↓
FellTree Job
        ↓
Woodcutter claims job
        ↓
Woodcutter pathfinds to tree
        ↓
Tree removed
        ↓
Log item created
        ↓
Haul job generated
        ↓
Log moved to stockpile
```

Current wood materials:

```text
Oak Wood
Pine Wood
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

Each item retains its exact material.

For example:

```text
Item
├── Type: Stone
└── Material: Granite
```

or:

```text
Item
├── Type: Ore
└── Material: Gold Ore
```

or:

```text
Item
├── Type: Log
└── Material: Oak Wood
```

This distinction becomes important for future construction, crafting, trade, and workshops.

---

# Stockpiles

Stockpiles occupy rectangular areas of the 3D world.

Each stockpile tracks:

```text
Bounds
Accepted Item Categories
Stored Items
Reserved Destination Cells
Optional Capacity
```

Destination cells are reserved when haul jobs are generated so multiple haulers do not attempt to deliver items to the same tile.

Stockpiles can be created and configured interactively through the SDL frontend.

---

# Hauling

Hauling is autonomous.

```text
Loose Item
    ↓
Find Compatible Stockpile
    ↓
Reserve Destination
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

Because pathfinding operates in 3D, items can be transported between surface and underground areas.

---

# 3D Pathfinding

ASCII Universe uses A* across the full `(x, y, z)` world.

Horizontal movement uses four cardinal directions:

```text
    ↑

←       →

    ↓
```

Vertical movement is enabled through:

```text
Up Stairs
Down Stairs
Up/Down Stairs
Ramps
```

The heuristic accounts for all three dimensions.

---

# Phase 5.5 — Procedural World Generation

The current generator has been rebuilt around larger-scale environmental systems rather than small random height variations.

New worlds are generated in several stages:

```text
Seed
 │
 ├── Large-Scale Landform
 │
 ├── Terrain Noise
 │
 ├── Erosion
 │
 ├── Climate
 │
 ├── Moisture / Temperature
 │
 ├── Rivers
 │
 ├── Lakes
 │
 ├── Riparian Moisture
 │
 ├── Soil Formation
 │
 ├── Geological Provinces
 │
 ├── Geological Strata
 │
 ├── Ore Veins
 │
 ├── Caverns
 │
 ├── Surface Ramps
 │
 ├── Forest Regions
 │
 └── Natural Embark Selection
```

World generation uses deterministic seeded noise independently from runtime simulation randomness.

---

# Landforms

Every generated world has a dominant large-scale landform.

Current landforms:

```text
Plains
Rolling Hills
Highlands
Mountain Range
River Valley
Plateau
```

These are not simple labels.

Each landform uses a different elevation model.

For example:

### Plains

Low-amplitude terrain with large relatively flat areas.

### Rolling Hills

Moderate elevation changes with broad smooth hills.

### Highlands

Generally elevated terrain with more exposed rock.

### Mountain Range

A large directional mountain system using ridged noise and warped range orientation.

### River Valley

Higher terrain surrounding a broad lower corridor.

### Plateau

Elevated interior terrain with steeper outer transitions.

The generated landform is stored as persistent world metadata and displayed in the SDL sidebar.

---

# Climate

Each world also has a broad regional climate.

Current climates:

```text
Temperate
Wet Temperate
Boreal
Dry
```

Climate influences:

```text
Base Moisture
Temperature
Forest Density
Tree Species
River Frequency
Lake Frequency
Soil Formation
```

The simulation stores the generated climate in save files and displays it in the SDL sidebar.

Example:

```text
Landform: Mountain Range
Climate: Boreal
```

---

# Terrain Noise

World generation contains a custom deterministic noise implementation.

The current noise utilities include:

```text
Coordinate hashing
2D value noise
3D value noise
Fractional Brownian motion
Ridged noise
```

Multiple noise scales allow the generator to combine:

```text
regional terrain
+
large hills
+
mountain structures
+
local variation
```

rather than producing independent random elevations per tile.

---

# Erosion

After raw elevation generation, the surface passes through several thermal-style erosion/smoothing passes.

This reduces isolated one-tile spikes while preserving major terrain features.

The current system is intentionally lightweight rather than a full geological erosion simulation.

---

# Hydrology

## Rivers

Rivers preferentially begin at higher terrain and travel toward lower elevations.

The river generator includes:

```text
High-elevation source selection
Downhill preference
Seeded meandering
Local basin breaching
Channel erosion
Occasional widening
River merging
```

Rivers modify the actual height field instead of simply drawing water over unrelated terrain.

## Lakes

Lakes are generated around low terrain basins.

Lake generation includes:

```text
Low-ground basin selection
Irregular shoreline noise
Terrain depression
Variable water depth
```

Water depth ranges from:

```text
0 = dry

1
2
3
4
5
6
7 = full
```

---

# Water and Ecology

Surface water affects nearby environmental moisture.

The generator calculates distance from rivers and lakes and increases moisture around them.

Conceptually:

```text
River
  ↓
Nearby Moisture
  ↓
Different Soil
  ↓
Denser Vegetation
```

This allows river valleys and lake shores to become ecologically distinct from dry regions.

---

# Soil Formation

Soil generation now depends on environmental context rather than being chosen entirely at random.

Current soil and sediment materials:

```text
Soil
Clay
Sand
Silt
Loam
Peat
```

Soil depth can vary with:

```text
Moisture
Slope
Landform
Local Noise
```

Steep mountain terrain can therefore expose bedrock while flatter wet terrain accumulates deeper soil.

---

# Geology

The world contains regional geological provinces.

Current broad geological groups include:

```text
Sedimentary
Granitic
Volcanic
Metamorphic
```

These provinces produce different underground strata.

## Sedimentary

Possible materials include:

```text
Shale
Limestone
Sandstone
Dolomite
```

## Granitic

Possible materials include:

```text
Gneiss
Granite
Diorite
```

## Volcanic

Possible materials include:

```text
Basalt
Gabbro
Obsidian
```

## Metamorphic

Possible materials include:

```text
Slate
Schist
Gneiss
Marble
```

Geological layers can warp and dip rather than remaining perfectly horizontal.

---

# Current Materials

## Surface / Soil

```text
Grass
Soil
Clay
Sand
Silt
Loam
Peat
```

## Sedimentary Rock

```text
Limestone
Sandstone
Shale
Dolomite
```

## Igneous Rock

```text
Granite
Basalt
Obsidian
Diorite
Gabbro
```

## Metamorphic Rock

```text
Marble
Slate
Schist
Gneiss
```

## Minerals / Ores

```text
Coal
Iron Ore
Copper Ore
Tin Ore
Silver Ore
Gold Ore
Quartz
```

## Wood

```text
Oak Wood
Pine Wood
```

---

# Geology-Aware Ore Deposits

Ore generation is tied to host rock.

Minerals are not simply inserted into arbitrary stone.

Examples:

```text
Coal
→ Shale
→ Sandstone
→ Limestone
```

```text
Tin
→ Granite
→ Gneiss
→ Diorite
```

```text
Gold
→ Granite
→ Gneiss
→ Schist
```

```text
Quartz
→ Granite
→ Gneiss
→ Schist
→ Slate
→ Diorite
```

Ore rarity also varies with underground depth.

Deposits are generated as wandering 3D veins rather than isolated random blocks.

---

# Forest Regions

Forests are generated as regional structures rather than independent random tree rolls.

Forest suitability combines:

```text
Moisture
Temperature
Regional Vegetation Noise
Slope
Climate
```

This produces areas such as:

```text
Open Field
      ↓
Forest Edge
      ↓
Dense Woodland
      ↓
Forest Edge
      ↓
Open Terrain
```

rather than evenly scattered trees.

Cold or elevated environments favor pine.

Warmer environments favor oak.

---

# Caverns

The underground contains procedural cavern systems.

Cavern generation includes:

```text
Multiple cavern layers
Irregular chambers
Connecting tunnels
Underground pools
```

Caverns preserve surrounding geology.

A cavern carved through limestone remains limestone.

A cavern intersecting an ore vein can expose that ore.

This creates naturally explorable underground spaces rather than only solid rock.

---

# Natural Embark Selection

The generator no longer creates a large artificial embark plateau or test fortress.

Instead:

```text
Generate Whole World
        ↓
Generate Terrain
        ↓
Generate Water
        ↓
Generate Geology
        ↓
Generate Caverns
        ↓
Generate Forests
        ↓
Evaluate Natural Locations
        ↓
Choose Embark
```

The embark finder considers:

```text
Usable Terrain
Local Elevation
Nearby Trees
Nearby Fresh Water
Moisture
Distance from Map Center
```

Only the exact starting cells are cleared when required.

No underground fortress is pre-carved.

The player must actually begin the fortress through in-game designations.

---

# Water Simulation

Generated rivers, lakes, and cavern pools become part of the same runtime liquid system used by the simulation.

Water simulation currently handles:

```text
Gravity
Vertical Movement
Ramps / Connections
Horizontal Equalization
```

Deep water can prevent normal goblin movement.

The system is intentionally game-oriented rather than a full physical fluid solver.

---

# ECS Architecture

ASCII Universe uses **EnTT**.

Entities provide identity while components provide state and capabilities.

Example miner:

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

Example woodcutter:

```text
Brakka
│
├── Goblin
├── Name
├── Woodcutter
├── Position
└── Glyph
```

Current worker roles:

```text
Miner
Hauler
Woodcutter
```

These are currently capability marker components.

A future labor/skill system is expected to replace this with more dynamic profession assignment.

---

# Fixed-Timestep Simulation

Simulation operates independently from rendering.

Current simulation rate:

```text
10 ticks per second
```

with:

```cpp
FixedStep = 100ms
```

Rendering speed and simulation speed are independent.

The player can currently run the simulation at:

```text
1x
2x
4x
8x
```

This architecture also supports future:

```text
Long-running world simulation
Replays
Automated tests
History simulation
Fast-forward
```

---

# System Order

The current simulation tick is approximately:

```text
Designation Deduplication
        ↓
Designation → Job Generation
        ↓
Haul Job Generation
        ↓
Excavation Assignment
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
Item Spawn Processing
        ↓
Item Pickup Processing
        ↓
Item Drop Processing
        ↓
Additional Haul Generation
        ↓
Water Simulation
        ↓
Tick++
```

System order is explicit because later systems frequently consume state generated earlier in the same tick.

---

# SDL Frontend

ASCII Universe uses SDL3 instead of terminal rendering.

The frontend currently provides:

```text
Resizable window
Colored ASCII
Fixed logical rendering surface
Camera viewport
Mouse interaction
Tile cursor
Selection highlighting
Designation painting
Goblin selection
World inspection
HUD / sidebar
Climate display
Landform display
Simulation speed controls
```

The SDL renderer remains separate from simulation rules.

The current visual layer uses SDL's built-in fixed-width ASCII font.

A proper tileset or SDL_ttf-based rendering layer can be added later without replacing the simulation.

---

# Sidebar

The current sidebar displays information including:

```text
Tick
Current Z-Level
Landform
Climate
Pause State
Simulation Speed
Player Mode
Cursor Position
Tile Shape
Material
Water Depth
Tree Material
Designation
Item Information
Stockpile Information
Selected Goblin
Goblin Profession
Goblin Position
Goblin Job
Follow State
Controls
Status Messages
```

---

# Save / Load

ASCII Universe uses versioned JSON saves.

Current save version:

```text
2
```

The save currently persists:

```text
World seed
World dimensions
Landform
Climate
3D tile state
Tile materials
Tile features
Liquids
Goblins
Positions
Roles
Movement paths
Designations
Designation lifecycle
Items
Item materials
Item state
Carried items
Stockpiles
Stockpile filters
Stockpile reservations
Jobs
Job assignments
Simulation tick
Runtime RNG state
```

Landform and climate were added as optional version-2 metadata so saves created before Phase 5.5 can still load.

Older saves without those fields display:

```text
Landform: Unknown
Climate: Unknown
```

---

# Command-Line Options

Generate a world using a random seed:

```bash
./build/ascii_universe
```

Generate a deterministic world:

```bash
./build/ascii_universe --seed 123456
```

Choose a save file:

```bash
./build/ascii_universe \
    --seed 123456 \
    --save saves/world.json
```

Load an existing fortress:

```bash
./build/ascii_universe \
    --load saves/world.json
```

Run until a fixed number of additional simulation ticks:

```bash
./build/ascii_universe \
    --seed 123456 \
    --stop-after 100
```

This is useful for deterministic testing.

---

# Dependencies

ASCII Universe currently uses:

```text
C++20
CMake 3.24+
Ninja
SDL3
EnTT 3.15.0
nlohmann/json 3.12.0
```

EnTT and nlohmann/json are fetched automatically through CMake.

SDL3 is currently provided by the host system.

---

# Building

## macOS

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

Clone:

```bash
git clone https://github.com/JarronAnt/ascii-universe.git

cd ascii-universe
```

Configure and build:

```bash
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

Run a known seed:

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
│       ├── PlayerController.hpp
│       ├── PlayerControls.hpp
│       ├── Position.hpp
│       ├── Random.hpp
│       ├── SaveManager.hpp
│       ├── SDLFrontend.hpp
│       ├── Simulation.hpp
│       ├── Stockpiles.hpp
│       ├── Tile.hpp
│       ├── WorldEnvironment.hpp
│       ├── WorldGenerator.hpp
│       │
│       ├── worldgen/
│       │   └── Noise.hpp
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
    ├── PlayerController.cpp
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

# Architecture

```text
                      ┌─────────────────┐
                      │   SDL Frontend  │
                      └────────┬────────┘
                               │
                               ▼
                      ┌─────────────────┐
                      │ PlayerController│
                      └────────┬────────┘
                               │
                               ▼
                      ┌─────────────────┐
                      │  Designations   │
                      └────────┬────────┘
                               │
                               ▼
                      ┌─────────────────┐
                      │    JobBoard     │
                      └────────┬────────┘
                               │
             ┌─────────────────┼─────────────────┐
             ▼                 ▼                 ▼
          Miners            Haulers         Woodcutters
             │                 │                 │
             └─────────────────┼─────────────────┘
                               ▼
                      ┌─────────────────┐
                      │      World      │
                      └────────┬────────┘
                               │
                               ▼
                      ┌─────────────────┐
                      │   Save / Load   │
                      └─────────────────┘
```

World generation is a separate deterministic pipeline that creates the initial world state.

Runtime simulation then operates on that generated state.

---

# Roadmap

## Phase 1 — Core Simulation

- [x] C++20
- [x] CMake / Ninja
- [x] EnTT ECS
- [x] Fixed timestep
- [x] Seeded RNG
- [x] Movement
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
- [x] Vertical excavation

## Phase 3 — 3D World

- [x] Z-levels
- [x] 3D positions
- [x] Vertical pathfinding
- [x] Stairs
- [x] Ramps
- [x] Materials
- [x] Water
- [x] Surface features

## Phase 4 — Persistence and SDL

- [x] Versioned JSON saves
- [x] Save/load
- [x] RNG persistence
- [x] SDL3 frontend
- [x] Colored ASCII
- [x] HUD
- [x] Z-level display

## Phase 5 — Playable Fortress Controls

- [x] Camera
- [x] Tile cursor
- [x] Mouse selection
- [x] Tile inspection
- [x] Mining designation
- [x] Tree-felling designation
- [x] Dig-up designation
- [x] Dig-down designation
- [x] Stockpile painting
- [x] Stockpile filters
- [x] Cancellation
- [x] Simulation speeds
- [x] Goblin selection
- [x] Follow camera

## Phase 5.5 — World Variety

- [x] Larger 96×64×24 worlds
- [x] Deterministic noise framework
- [x] Plains
- [x] Rolling hills
- [x] Highlands
- [x] Mountain ranges
- [x] River valleys
- [x] Plateaus
- [x] Climate types
- [x] Moisture fields
- [x] Temperature fields
- [x] Erosion pass
- [x] Rivers
- [x] Lakes
- [x] Riparian moisture
- [x] Variable soil depth
- [x] Expanded soil materials
- [x] Geological provinces
- [x] Geological strata
- [x] Geology-aware ore deposits
- [x] 3D ore veins
- [x] Forest regions
- [x] Underground caverns
- [x] Underground pools
- [x] Natural embark-site selection
- [x] Persistent climate metadata
- [x] Persistent landform metadata

## Phase 6 — Construction

Next major milestone.

Planned:

- [ ] Builder labor
- [ ] Construction plans
- [ ] Material requirements
- [ ] Item reservations
- [ ] Construction hauling
- [ ] Build walls
- [ ] Build floors
- [ ] Build ramps
- [ ] Build stairs
- [ ] Remove construction
- [ ] Doors
- [ ] Furniture
- [ ] Construction cancellation
- [ ] Construction save/load
- [ ] Planned-construction rendering

Target pipeline:

```text
Player Places Construction
        ↓
Construction Plan
        ↓
Material Required
        ↓
Reserve Material
        ↓
Hauling Job
        ↓
Resource Delivered
        ↓
Builder Job
        ↓
Construction Completed
        ↓
World Geometry Changes
```

## Phase 7 — Workshops and Industry

Planned:

- [ ] Workshop entities
- [ ] Production queues
- [ ] Recipes
- [ ] Carpenter
- [ ] Mason
- [ ] Smelter
- [ ] Forge
- [ ] Blocks
- [ ] Furniture
- [ ] Tools
- [ ] Weapons
- [ ] Metal bars
- [ ] Multi-step production chains

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

Planned:

- [ ] Skills
- [ ] Labor permissions
- [ ] Hunger
- [ ] Thirst
- [ ] Sleep
- [ ] Mood
- [ ] Personality
- [ ] Relationships
- [ ] Health
- [ ] Injuries
- [ ] Equipment
- [ ] Preferences

## Phase 9 — World / Embark Layer

The current game chooses a natural embark inside a generated fortress-scale map.

A later phase can introduce a true Dwarf Fortress-style world layer:

- [ ] World-scale map
- [ ] Regional biomes
- [ ] Regional elevation
- [ ] Continental drainage
- [ ] Large river systems
- [ ] Regional geological provinces
- [ ] Climate zones
- [ ] Player-controlled embark selection
- [ ] Local-map generation from selected region
- [ ] Settlements
- [ ] Civilizations
- [ ] Trade
- [ ] Diplomacy
- [ ] Warfare
- [ ] Procedural history

## Later Rendering

The current SDL ASCII renderer is intentionally functional rather than final.

Possible future rendering work:

- [ ] SDL_ttf font rendering
- [ ] Zoom
- [ ] Proper tileset support
- [ ] Optional graphical tiles
- [ ] Improved overlays
- [ ] Worldgen debug views
- [ ] Better menus
- [ ] Construction UI
- [ ] Workshop UI

ASCII mode should remain supported even if graphical tiles are added.

---

# Design Philosophy

ASCII Universe prioritizes **systems over scripts**.

The goal is not:

```text
Trigger Event A
    ↓
Play Script B
    ↓
Produce Result C
```

Instead:

```text
World State
+
Resources
+
Agent Capabilities
+
Player Orders
+
Environment
+
Jobs
+
Simulation Rules
        ↓
Emergent Result
```

A miner mines because valid work exists.

A hauler moves ore because an item needs a compatible destination.

A woodcutter cuts a tree because the player designated it.

Future builders should build because a construction plan requires material and labor.

Future civilizations should rise or collapse through the same principle: interacting systems rather than predetermined stories.

---

# Status

ASCII Universe is in active development.

It is currently a playable fortress-simulation prototype rather than a finished game.

The foundation now includes:

```text
3D World
+
Procedural Environment
+
Geology
+
Water
+
Resources
+
Autonomous Workers
+
Player Designations
+
Stockpiles
+
Hauling
+
Save / Load
+
SDL Interface
```

The next step is to turn those systems into an actual built settlement through **Phase 6: Construction**.
