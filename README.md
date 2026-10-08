# GSND 6250 – Spatial + Temporal Design (Fall 2026)

Level design projects for GSND 6250, Northeastern University (Prof. Larissa Erin Greer).

**Team:** Rachel (Ray) Hsiao · Jiayi Zhang · Qitao Zheng
**Engine:** Unreal Engine 5.6 (C++ project, Third Person template)

## Layout

```
GSND6250_LevelDesign/       Unreal project (one project for the whole semester)
  Source/                   C++ gameplay code
  Content/GP1_LightHide/    Group Project 1 map and materials
  Content/GP2_.../          (future projects get their own folder)
```

## Getting started

1. Install [Git LFS](https://git-lfs.com) and run `git lfs install` once. Maps and assets (`.umap`, `.uasset`) are stored with LFS; without it they download as small pointer files that Unreal cannot open.
2. Clone the repo and open `GSND6250_LevelDesign/GSND6250_LevelDesign.uproject` with **Unreal Engine 5.6**. The first launch compiles the C++ module (Visual Studio 2022 with the "Game development with C++" workload is required).
3. Open the project's map from the Content Browser (see the table below) and press Play.

## Projects

### GP1 – Formal + Functional Elements

**Map:** `Content/GP1_LightHide/Maps/L_GP1_LightHide`

The player is trapped in a room with a monster and must escape. The start corridor and the exit are brightly lit by windows; the monster will not walk into the light, so those areas are safe. The danger is the dark stretch between them: a central wall forces the player down a lane the monster patrols, and a locker along that lane is the only place to hide. The monster glows red so the player can track it in the dark and plan when to move.

| Pattern | Type | How it shows up in the level |
|---|---|---|
| A Spark in the Dark | Formal (seed: light is safe, darkness is dangerous) | Warm window light and yellow floors mark the two safe zones; the room between them is dark |
| Hiding Buys Time | Functional (seed: hide & seek) | Locker on the patrol lane lets the player wait out the monster; its red glow is the readable threat cue |
