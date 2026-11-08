# OpenGL fluid simulation course — notes for Claude Code sessions

This repo is **the course**: the user rebuilds an SPH fluid simulation (after Sebastian Lague's
Fluid-Sim) step by step to learn OpenGL and fluid physics. Claude works here in **tutor mode**.
This file is the source of truth and the cross-machine memory (it travels through GitHub).

## The user

- Writes in Uzbek; all lessons and answers in Uzbek. Keep technical terms in English (VAO, framebuffer,
  smoothing kernel), explained in Uzbek.
- Learning C++, OpenGL, the math (linear algebra) and the physics behind them. Wants depth: explain
  concepts from the ground up ("why does this exist, what problem does it solve, what breaks
  without it"), with analogies — but keep the pace up.
- Works on **two machines**: Windows PC (MSYS2 MinGW64: g++, cmake, ninja, glfw; RTX 3060,
  i5-12400F) and a **MacBook with the latest macOS**. Code goes through GitHub (user account
  `sardor-c`). The user runs pull/commit/push.

## Tutor mode

1. **The user puts 100% of the code and the setup into the project themselves** — CMakeLists,
   .gitignore, every source and shader file — typing/adapting it from the lessons' code samples.
   Do NOT create or edit `src/`, `shaders/`, `CMakeLists.txt` or other project files. Claude writes only:
   - lessons: `docs/NN-topic.md` (Uzbek), listed in `docs/README.md` (table of contents);
   - this `CLAUDE.md` (course state, cross-machine memory).
2. **Lesson content (the user's request on 2026-11-08): mostly theory and code samples, few tasks.**
   A lesson walks through the build sequence of this project: what we add in this step and why it
   comes at this point, the theory behind it (OpenGL concepts, math, physics), then the **code samples
   for the step** — complete enough that the user can type them into the project and build — with
   each part explained (API calls, parameters, what breaks without it). End with "how to check it
   works" (what you should see on Windows and macOS). Keep exercises and independent assignments to
   a minimum: at most an occasional optional short experiment ("change X and watch Y"), no graded
   tasks, no hidden solutions. Chat reply = short summary + link to the lesson file.
3. Verify every code sample shown in a lesson in the **scratchpad** (never in the project): compile it
   with MSYS2 g++, run it, and validate GLSL with `glslangValidator` as GLSL 4.10 (see Pitfalls).
   The samples must build on both platforms, so keep them free of platform-specific code.
4. One step at a time. Wait until the user says the step is done; then review their code if they ask
   (reading the files and running their build is fine), explain mistakes in detail, and only then
   move on.
5. When the user reports something works or doesn't, make sure it was actually run; prove claims by
   running things, not by assertion.
6. Build/compiler/linker problems: help when the user says they are stuck; don't pre-empt with fixes.
7. Two machines: at session start run `git status -sb` and `git log --oneline -3` and ask whether
   they worked on the other machine and pulled. After each lesson remind them to commit and push.

## Technical constraints

- **OpenGL 4.1 core only**, GLSL `#version 410 core` — the newest macOS supports. No compute shaders,
  SSBOs, image load/store, atomics, DSA, KHR_debug, `layout(binding=)` or explicit uniform locations.
  The user explicitly rejected optional code paths for newer GL, even Windows-only ones.
- Keep the structure flat and simple (`src/`, `shaders/`, `external/`); no extra layers/abstractions.
- Same code on Windows and macOS: no platform-specific code unless unavoidable.
- Dependencies: GLFW only (find_package, else CMake FetchContent). GL loader: either a glad build
  generated for GL 4.1 core, or the reference project's approach (vendored `GL/glcorearb.h` +
  X-macro loader via `glfwGetProcAddress`). To be decided in lesson 01.

## Reference implementation (`../opengl_fluid_sim_tutorial/`, Windows PC only)

Written by Claude. Use it to check physics/values and to compare when the user is stuck — never copy
it wholesale into the course. On the Mac it exists only if the user cloned/copied it — ask.
Git history:
- `afc4227` — simulation in plain C++ on the CPU (ThreadPool, CPU counting-sort spatial hash,
  `sph_kernels.hpp` with the physics explained), rendering with VBO streaming + instancing.
  Read with `git -C ../opengl_fluid_sim_tutorial show afc4227:src/<file>`. Matches course Part II.
- `1ae29c9` (branch `gpu-simulation`) — simulation on the GPU with fragment-shader GPGPU (float
  textures, fullscreen passes, ping-pong, bitonic sort, point-scatter cell ranges, std140 UBO).
  Matches course Part III. `shaders/fluid_sim2d.frag` holds the SPH physics with comments.
- Unity scene values: `src/settings2d.hpp`, `src/settings3d.hpp`; spawners; orbit camera; `math.hpp`.

Other references: `C:\dev\VULKAN_SIMULATIONS\fluid_sim` (Vulkan port), https://github.com/SebLague/Fluid-Sim.

## Course plan (draft — adjust with the user, record changes here)

Part I — setup and OpenGL basics
- 00 Muhit va loyiha: git + .gitignore, CMake (C++17), GLFW (find_package / FetchContent), build on
  Windows (MSYS2 + Ninja) and macOS (Xcode CLT + brew), empty window; push, clone on the Mac, build.
- 01 OpenGL konteksti: 4.1 core + forward-compat hints, function loading, clear colour, GL_VERSION,
  vsync, framebuffer size vs window size (Retina), main loop, glGetError helper.
- 02 Birinchi uchburchak: GLSL 410 shaders (compile/link, logs), VAO/VBO, attributes, glDrawArrays;
  loading shaders from files (path via compile definition), simple `#include`.
- 03 Ko'p zarracha: orthographic world→clip uniform, quad from gl_VertexID, instancing
  (glDrawArraysInstanced + glVertexAttribDivisor), smooth circle (smoothstep + fwidth), blending.

Part II — 2D SPH on the CPU
- 04 Harakat: position/velocity, gravity, Euler step, timestep (frame time, clamp, iterations per
  frame), wall collisions with damping, streaming VBO (GL_STREAM_DRAW), spawner, pause/step/reset.
- 05 Zichlik: the SPH idea, smoothing kernel, normalisation (integral = 1, derive the constant),
  density with an O(n²) loop, density shown as colour.
- 06 Bosim: equation of state, kernel gradient, symmetric (shared) pressure, predicted positions,
  near density/pressure; first liquid; parameters and timestep stability.
- 07 Yopishqoqlik va sichqoncha: viscosity (Poly6), cursor → world (window vs framebuffer coords),
  pull/push, interaction circle, colour by speed (gradient).
- 08 Tez qo'shni qidirish: grid cells, hash, counting sort (histogram → prefix sum → scatter),
  [start, end) ranges, reorder for cache; measure O(n²) vs O(n).
- 09 Ko'p oqim (optional): thread pool / parallel for, which loops are safe, profiling per phase,
  compiler flags.
- 10 Sahna: background fullscreen triangle + SDF (bounds, obstacle, outlines), obstacle collision,
  Unity scenes, scene switching, self-test (`--frames N`).

Part III — the simulation on the GPU, within OpenGL 4.1 (GPGPU)
- 11 Ma'lumot teksturada: RG32F textures, NEAREST + completeness, texelFetch, index ↔ texel,
  vertex shader reading positions from a texture (instancing without VBOs).
- 12 Teksturaga chizish: FBO, glFramebufferTexture2D, MRT (glDrawBuffers), fullscreen pass, ping-pong,
  feedback loops; std140 UBO for parameters; external forces + integration on the GPU.
- 13 GPU'da zichlik/bosim: O(n²) first for small n, then why a neighbour search is needed.
- 14 GPU'da saralash: bitonic sort (sorting networks, pass count), RG32UI textures, cell ranges by
  point scatter (GL_POINTS + colour mask), reorder; check against the CPU version at a fixed dt.

Part IV — 3D
- 15 3D kamera: view/perspective matrices (Unity's left-handed view space vs GL clip space), depth
  buffer, orbit camera, bounds wireframe from gl_VertexID (GL_LINES).
- 16 3D zarrachalar: camera-facing billboards, sphere impostor lighting, 3D kernels, 27 cells,
  block hash, air drag, 3D scenes and scene scaling.
- 17 Yakun: README, measurements, ideas for next steps (e.g. raymarching a 3D density texture).

## Pitfalls already found (verified in the reference project)

- macOS: no `GL/glcorearb.h` in the SDK (vendor it or use glad); needs `GLFW_OPENGL_FORWARD_COMPAT`;
  Retina → glViewport uses `glfwGetFramebufferSize`, the mouse uses `glfwGetWindowSize`; no debug
  output (use glGetError); OpenGL there runs on top of Metal and is deprecated (still present).
- GLSL 410: sampler units via `glUniform1i`, uniform blocks via `glGetUniformBlockIndex` +
  `glUniformBlockBinding`. Validate with `glslangValidator file` — it enforces 410 (rejects
  `layout(binding)`); for kernels selected by `#define`, insert the define after `#version` in a copy
  (its `-D` flag requires `-l`).
- Core profile: a VAO must be bound for every draw, even with no attributes (gl_VertexID geometry).
- Data textures: set MIN/MAG filter to GL_NEAREST (the default needs mipmaps → incomplete texture →
  reads zeros). RG32F/RGBA32F/RG32UI are renderable; RGB32F is not guaranteed. Integer textures are
  cleared with `glClearBufferuiv` (glClear doesn't work, and glColorMask applies to clears).
- A draw must not read a texture attached to the bound framebuffer (feedback loop) → ping-pong, and
  unbind/detach what a pass doesn't use.
- std140: vec3 is 16-byte aligned (a float fits after it), block size rounds to 16 → mirror the
  C++ struct with explicit padding + `static_assert(sizeof(...))`.
- Physics: 2D "Test C" explodes with 3 steps per frame at 60 fps (1/180 s per step); stable at
  ≤ 1/240 s → 4 iterations. `-ffast-math` makes the simulation unstable (3x slower). CPU profile:
  ~93% of the time is density + pressure neighbour loops; `-O3 -march=native` gave ~20% in 3D;
  a power-of-two hash table (`& (n-1)` instead of `% n`) ~15%.
- GPU (fragment GPGPU): a step is ~170 render passes (bitonic sort dominates), so small scenes are
  pass-bound, not particle-bound. RTX 3060: 2D 125k particles ~124 fps, 3D 170k ~81 fps.
- Windows: `C:\msys64\mingw64\bin` must be on PATH (glfw3.dll, libwinpthread); Git Bash paths
  (`/c/...`) don't work in native programs (use `cygpath -m`). Git autocrlf → shader loaders must strip `\r`.
- Testing pattern that worked: `--frames N` self-test (NaN, out-of-bounds, sorted keys), headless
  harnesses in the scratchpad with a fixed dt to compare implementations, screenshots via PowerShell.

## Progress

Update this at the end of every session.

- 2026-11-08: Course started. Lesson 00 written (`docs/00-muhit-va-loyiha.md`), waiting for the user
  to do it. The repo did not exist yet when the lesson was written: the user runs `git init` and
  creates the GitHub repo in lesson 00.

Decisions so far:
- One executable, `fluid_sim`. Compile with `-Wall -Wextra` (g++ and AppleClang only, no MSVC branch).
- `cmake_minimum_required(VERSION 3.24)`: CMP0135 is NEW, so FetchContent needs no policy block.
- Same commands on both machines: `cmake -S . -B build -G Ninja`, `cmake --build build`.
- Lesson 00's window uses `GLFW_NO_API` and `glfwWaitEvents`; lesson 01 adds the context and moves
  to `glfwPollEvents`.

Verification status of lesson 00: everything was built and run on the Windows PC (shared GLFW 3.5.1
from MSYS2, static GLFW 3.4 via FetchContent, git push + fresh clone + build against a local bare
repo). **The macOS steps have not been run by Claude** — the first real test is the user's Mac.
