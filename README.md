# Alfheim for Voxel Plugin Free Legacy
### Lost Crow Dev
<img width="1335" height="749" alt="image" src="https://github.com/user-attachments/assets/0d669c04-a1d9-43c5-9a12-9650c6cd6aa7" />


**Alfheim** is a C++ toolset that extends **Voxel Plugin Free Legacy** to help create procedural voxel biomes and PCG-driven scenes. It’s an early work-in-progress that aims to give you Valheim-style terrain and biome feel: large organic islands, rolling hills, mountains and plains... and with point and spline-driven modifiers layered onto the voxel landscape (coming soon).

---

## Requirements:
- Unreal Engine 5.7.3.
  - UE 5.6 and other version of 5.7 may work, but are untested.

---

## What’s included

* An **example map** and an **example voxel graph** (the “Alfheim” voxel graph).

---

## Examples

* The included **Alfheim** example currently produces **natural looking islands**. It is provided as a working reference:

  * Many noise and generation values are exposed so you can tweak them in the graph (frequency, amplitude, octaves/layers, falloff, etc.).
  * The graph is designed to produce a continuous landmass with slope falloff around the edges and internal variation for hills and mountains.
  * The idea is that the core look (island outline, slope falloff) is built into the generator itself so you get a coherent terrain base to work from.

---

## Goals and roadmap

* Allow game develoeprs to create multiple biomes driven by PCG (forests, plains, mountains, coastal zones, etc.).
* Provide reusable voxel graphs and PCG rules so you can combine and tile different biome types.
* Improve noise controls from the official examples.
* Continue iterating on performance, editor UX and the example content. Right now it’s **early WIP**.

---

## Guide 

1. Open the provided **Alfheim** level to see the included voxel graph and tools in action.
2. Open the **Alfheim voxel graph** and tweak the exposed noise/generation parameters to change island shape, hilliness, and slope falloff.

---

## Current limitations / notes

* Early WIP: features, API calls and editor bindings may change as development continues.
* The example graph currently emits a single island; multi-biome, more complex generation and deeper PCG integration are planned.
* Behavior depends on the Voxel Plugin (Free Legacy) and the engine PCG plugin—API differences between versions may require small updates.


---

