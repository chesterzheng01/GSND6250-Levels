# GSND 6250 – Spatial + Temporal Design (Fall 2026)

Level design projects for GSND 6250, Northeastern University (Prof. Larissa Erin Greer).

**Team:** Rachel (Ray) Hsiao · Jiayi Zhang · Qitao Zheng
**Engine:** Unreal Engine 5.8 (C++ project, Third Person template; upgraded from 5.6 in October 2026, see [below](#upgrading-from-ue-56-to-58))

## Layout

```
GSND6250_LevelDesign/                   Unreal project (one project for the whole semester)
  Source/GSND6250_LevelDesign/          C++ (template base classes at the top level)
    GPn_<Name>/                         C++ for one group project, classes prefixed GPn (e.g. AGP3Boss)
  Content/
    GPn_<Name>/                         Everything made or imported for one group project
      Maps/  Blueprints/  Materials/  Textures/  Meshes/  Audio/  Input/
      Import/                           Third-party downloads, kept with their original names
    Characters/ Input/ LevelPrototyping/ ThirdPerson/   UE template, shared by all projects
    Materials_Frabric/                  Fabric pack used by GP2 (folder name is misspelled; don't rename)
    Variant_*/                          UE template sample levels, not used by any project
GP1/                                    Screenshots and video linked from this README
```

Current projects: `GP1_LightHide`, `GP2_MonsterHear`, `GP3_BossWeakPoint` (C++ in `Source/GSND6250_LevelDesign/GP3_BossWeakPoint/`), `GP4_TragicHeroism`.

What does not go in the repo: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `Content/Developers/`, packaged builds, course PDFs and write-ups.

Don't edit template assets in place. Duplicate them into your `GPn_` folder first, so one project's change can't break another (GP2 does this with its own character).

## Conventions

**Assets.** `Prefix_GPn_Name`, e.g. `BP_GP3_Boss`, `MI_GP3_Wall`, `L_GP4_TragicHeroism`.

| Prefix | Type | Prefix | Type |
|---|---|---|---|
| `L_` | Level | `T_` | Texture |
| `BP_` | Blueprint | `SM_` / `SK_` | Static / Skeletal mesh |
| `M_` / `MI_` | Material / instance | `S_` | Sound |
| `IA_` / `IMC_` | Input action / mapping context | `WBP_` | Widget |

Third-party files can keep their source names, but only inside `Import/`. Rename or move assets in the editor (never in Explorer), then right-click the folder → **Fix Up Redirectors** before committing.

**Shared assets.** If your project needs to change something another project uses (a GameMode, character, input mapping), duplicate it into your folder instead.

**Commits.** Short imperative English, with the project when it applies: `GP4: Add magma floor material`, `Fix navmesh in GP2`. One topic per commit.

**Branches.** Small changes can go straight to `main`. Use a branch for anything that touches shared assets or many files: `gpN/<topic>` or `<yourname>/<topic>`. Pull before you start working, and push before you stop.

## Getting started

1. Install [Git LFS](https://git-lfs.com) and run `git lfs install` once. Maps and assets (`.umap`, `.uasset`) are stored with LFS; without it they download as small pointer files that Unreal cannot open.
2. Clone the repo and open `GSND6250_LevelDesign/GSND6250_LevelDesign.uproject` with **Unreal Engine 5.8**. The first launch compiles the C++ module (Visual Studio 2022 with the "Game development with C++" workload is required).
3. Open the project's map from the Content Browser (see the table below) and press Play.
4. Install the pre-commit check once (blocks generated folders and assets that skip LFS): `cp .githooks/pre-commit .git/hooks/`. Don't set `core.hooksPath`; it would turn off the Git LFS hooks already in `.git/hooks/`.

## Upgrading from UE 5.6 to 5.8

The project moved to UE 5.8 in commit `241c4f6`. Assets saved in 5.8 cannot be opened in 5.6, so after you pull, use 5.8 only.

1. Install **Unreal Engine 5.8** from the Epic Games Launcher. Keep Visual Studio 2022 with the "Game development with C++" workload.
2. Commit and push (or stash) anything you changed in 5.6, close the editor, then `git pull`.
3. In `GSND6250_LevelDesign/`, delete `Binaries/`, `Intermediate/` and the `.sln` file. They were built for 5.6 and are not in git.
4. Right-click `GSND6250_LevelDesign.uproject` → **Switch Unreal Engine version** → **5.8**, then open it. If 5.8 is not in that list, open the project with `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` instead.
5. When the editor asks to rebuild the missing modules, click **Yes**. The C++ build takes about a minute; the first shader compile takes longer.
6. Open the GP1 and GP2 maps and press Play to check they still work.

Notes:
- The build shows one warning (C4996 in `Variant_Combat/AI/CombatStateTreeUtility.h`). It comes from the UE template code and is safe to ignore.
- Don't re-save assets you didn't change. 5.8 reads the 5.6 assets as they are, and mass re-saving would change hundreds of LFS files for nothing.

## Projects

### GP1 – Formal + Functional Elements

**Map:** `Content/GP1_LightHide/Maps/L_GP1_LightHide`

The player is trapped in a room with a monster and must escape. The start corridor and the exit are brightly lit by windows; the monster will not walk into the light, so those areas are safe. The danger is the dark stretch between them: a central wall forces the player down a lane the monster patrols, and a locker along that lane is the only place to hide. The monster glows red so the player can track it in the dark and plan when to move.

| Pattern | Type | How it shows up in the level |
|---|---|---|
| A Spark in the Dark | Formal (seed: light is safe, darkness is dangerous) | Warm window light and yellow floors mark the two safe zones; the room between them is dark |
| Hiding Buys Time | Functional (seed: hide & seek) | Locker on the patrol lane lets the player wait out the monster; its red glow is the readable threat cue |

**Screenshots and walkthrough video:** see the [`GP1/`](GP1/) folder.
