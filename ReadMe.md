# ⛏️ Simple Miner

> A Minecraft-style voxel sandbox built entirely from scratch in C++.
---

## Overview

**Simple Miner** is a custom-engine voxel game focused on real-time world streaming, procedural terrain generation, and dynamic lighting.

The project simulates an infinite block world where the terrain is generated around the player, chunks load and unload automatically, and lighting updates in real time when blocks are placed or destroyed.

The goal of this project was to understand how large sandbox games manage memory, rendering cost, and real-time simulation.

---

## Core Systems

### 🌍 Procedural World Generation
- Infinite terrain generated using noise functions
- Biomes and terrain variation
- Cave generation
- Chunk streaming around player

### 🧱 Voxel Chunk System
- World divided into chunks
- Dynamic loading/unloading
- Mesh rebuild only when necessary
- Face culling optimization

### 💡 Lighting Propagation
Minecraft-style flood fill lighting:

- Sunlight from sky
- Torch/block lighting
- Light attenuation
- Real-time updates when terrain changes

### ⚙️ Multithreading
- Worker thread job system
- Background chunk generation
- Mesh building off main thread
- Prevents frame stutter during exploration

### 🎮 Player & Interaction
- First person camera
- Collision & physics
- Block placement and destruction
- Fly mode

---

## Controls

| Input | Action |
|------|------|
| Mouse | Look around |
| W A S D | Move |
| Space | Jump / Fly up |
| Shift | Fly down |
| Left Click | Destroy block |
| Right Click | Place block |
| F | Toggle fly mode |

---

## Technical Highlights

- Custom C++ game engine
- Real-time mesh rebuilding
- Chunk memory management
- Flood-fill lighting algorithm
- Spatial partitioning
- Procedural terrain using noise
- Texture atlas rendering

---

## What I Implemented

This project was implemented entirely by me, including:

- Rendering pipeline
- World streaming system
- Lighting simulation
- Chunk management
- Player controller & physics
- Input system
- Multithreaded job system

---

## Tech Stack

- C++
- Custom Engine
- Multithreading
- Procedural Generation
- Real-time Rendering

---

## About Me

**Leep**  
Game Programmer

Portfolio: https://sites.google.com/view/leepportfolio/ 
LinkedIn: www.linkedin.com/in/peiyi-li-dev

---
