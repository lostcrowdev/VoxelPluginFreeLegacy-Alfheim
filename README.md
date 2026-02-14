# Alfheim for Voxel Plugin Free Legacy, by Lost Crow Dev

<img width="1335" height="749" alt="image" src="https://github.com/user-attachments/assets/ebf8a180-d194-4f1a-a1d6-7957290f484e" />

**Alfheim** is a C++ toolset that extends **Voxel Plugin (Free, Legacy)** to help create procedural voxel biomes and PCG-driven scenes. It’s an early work-in-progress that aims to give you Valheim-style terrain and biome feel: large organic islands, rolling hills, mountains and plains, with point and spline-driven modifiers layered onto the voxel landscape.

---

## What’s included

* A set of C++ scripts and editor tools that plug into Voxel Plugin Free (Legacy).
* An **example map** and an **example voxel graph** (the “Alfheim” voxel graph).
* A tool pipeline for combining broad terrain shaping, PCG generation, and per-asset terrain conforming.

---

## Examples

* The included **Alfheim** example currently produces **one large organic island**. It is provided as a working reference:

  * Many noise and generation values are exposed so you can tweak them in the graph (frequency, amplitude, octaves/layers, falloff, etc.).
  * The graph is designed to produce a continuous landmass with slope falloff around the edges and internal variation for hills and mountains.
  * The idea is that the core look (island outline, slope falloff, major elevation zones) is built into the generator itself so you get a coherent terrain base to work from.

---

## Goals and roadmap

* Allow game develoeprs to create multiple biomes driven by PCG (forests, plains, mountains, coastal zones, etc.).
* Provide reusable voxel graphs and PCG rules so you can combine and tile different biome types.
* Improve noise controls from the official examples.
* Continue iterating on performance, editor UX and the example content. Right now it’s **early WIP**.

---

## Voxel Conformer tools — three parts

The Conformer is a separate set of editor/runtime helpers that make spawned environment assets sit naturally in the voxel world. It has three main parts:

1. **Spline flattening (large areas)**

   * Use splines to gently flatten or smooth areas of terrain (towns, city footprints, dungeon entrances, biome clearings).
   * Splines are applied with a soft falloff so the terrain adjusts naturally rather than producing a hard clipped edge.

2. **Conform point tool (per-asset adjustment)**

   * Add small “conform point” components (or child actors/components tagged as conform points) to building tiles, paths, roads, dungeon pieces, or other environment assets.
   * When the conformer runs it reads those points and nudges the voxel surface so floors, roads and tiles sit flush with the ground.
   * The goal is a gentle, natural transition so assets look like part of the world rather than pasted on top of it.

3. **PCG volume referencing**

   * The conformer can reference PCG volumes selected in the editor. The workflow supports a three-step order:

     1. Create large shaped areas first (spline-based terraforming) to prepare the generated voxel mesh.
     2. Run PCG in those areas to spawn meshes and actors (trees, props, towns, spawners, etc.).
     3. Run the voxel conformer: it finds conform points on the spawned actors and adjusts the voxel mesh to match.
   * This ordering produces consistent results: broad terrain shaping → content spawn → per-asset terrain fit.

---

## Guide 

1. Open the provided **Alfheim** to see the included voxel graph and tools in action.
2. Inspect the **Alfheim voxel graph** and tweak the exposed noise/generation parameters to change island shape, hilliness, and slope falloff.
3. Use spline actors to mark large areas you want flattened or softened and add custom tags to them.
4. Place or spawn PCG volumes to generate scene content.
5. Verify conform points (components/tags) are present on your assets so the conformer can adjust the voxel surface after PCG generation.

---

## Current limitations / notes

* Early WIP: features, API calls and editor bindings may change as development continues.
* The example graph currently emits a single island; multi-biome, more complex generation and deeper PCG integration are planned.
* Behavior depends on the Voxel Plugin (Free Legacy) and the engine PCG plugin—API differences between versions may require small updates.

---



## Requirements:
Unreal Engine 5.7.3. UE 5.6 and other version of 5.7 may work, but are untested.

Add PCG dependency to your project's Build.cs

Example:

using UnrealBuildTool;

public class VoxelPluginPCG : ModuleRules
{
    public VoxelPluginPCG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Voxel",
            "PCG"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}

