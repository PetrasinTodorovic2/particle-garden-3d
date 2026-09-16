# Particle Garden 3D — v4.1 Performance Pass

Particle Garden is a generative-art instrument and interactive **C++17 / SFML** particle sandbox built around one idea: **sculpt a living universe**.

Version 4 turns the original 2D field into a spatial particle system. Version 4.1 keeps the same visual target while substantially reducing per-frame CPU work through cached camera transforms, squared-distance rejection, ring-buffer trails and shared particle projection data. Every particle now has X, Y and Z position/velocity, the camera uses a custom perspective projection written in C++, and trails are stored in world space so they remain attached to particles while the camera orbits.

The project stays intentionally asset-free: the particles, glows, scene previews, animated background, UI panels and pixel font are all generated directly in code.

## Version 4.1 performance pass

- camera basis vectors and focal length are cached and recomputed only when the camera actually changes
- current particle projections are shared across trail, particle and interaction rendering
- trail points are projected once per sample instead of once per segment endpoint
- trail history now uses a ring buffer, eliminating thousands of per-frame history shifts
- force fields reject out-of-range particles with squared distances before paying for `sqrt`
- bloom ring bounds are precomputed once per frame
- velocity damping is computed once per frame instead of once per particle
- speed limiting avoids `sqrt` unless the particle is actually over the cap
- the HUD exposes smoothed **FPS** and **frame time in milliseconds** for real performance measurements
- Release builds explicitly request `/O2` on MSVC and `-O3` on GCC/Clang

## Version 4 highlights

- true 3D particle state with X/Y/Z position, velocity, forces and world-space trail history
- custom C++ perspective camera with orbit, zoom, FOV and sensitivity controls
- world-to-screen projection plus screen-to-world interaction on the central plane
- five distinct spatial scenes:
  - **Binary Galaxy** — twin orbital systems with braided spirals
  - **Nebula River** — a deep, winding volumetric current
  - **Solar Bloom** — radial pulses and rotating outflow from a luminous core
  - **Cosmic Web** — particles gather into moving filament structures
  - **Aurora Drift** — layered sheets of slowly waving light
- Scene Gallery with five animated procedural preview cards
- soft additive glows and velocity-stretched particle cores
- depth-based particle scale/brightness for a stronger sense of perspective
- trails projected from world-space history, so camera movement does not leave trails stuck to the screen
- Cinematic Mode with automated orbit, subtle camera breathing, hidden HUD and slowed simulation
- interactive 3D force tools and persistent nodes placed on the interaction plane
- expanding 3D bloom impulse
- six live color palettes: Aurora, Ember, Ocean, Mono, Neon and Solar
- settings for particle count, particle size, trail length, force, background, camera FOV and orbit sensitivity
- PNG artwork capture, persistent settings, resizable window, pause menu and built-in controls screen

## Controls

| Input | Action |
| --- | --- |
| Left mouse | Attract particles |
| Shift + left mouse | Create a vortex |
| Middle mouse | Repel particles |
| Right mouse drag | Orbit the 3D camera |
| Mouse wheel | Zoom camera |
| Ctrl + mouse wheel | Change interaction radius |
| Ctrl + left mouse | Place attractor node |
| Ctrl + right mouse | Place repulsor node |
| Ctrl + middle mouse | Place vortex node |
| Drag a node | Move it on the interaction plane |
| Alt + left mouse on node | Delete node |
| Space | Create an energy bloom |
| F1–F5 | Open Binary Galaxy / Nebula River / Solar Bloom / Cosmic Web / Aurora Drift |
| 1–6 | Change color palette |
| `[` / `]` | Decrease / increase particle count |
| C | Toggle Cinematic Mode |
| Home | Reset camera |
| T | Clear trail history |
| S | Save a clean PNG in `captures/` |
| X | Remove all persistent nodes |
| R | Reset the scene |
| H | Hide/show HUD |
| Esc | Open/close pause menu |

## Architecture

```text
ParticleGarden/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp              # application state, gallery, input and UI flow
    ├── Camera3D.hpp          # custom orbit camera + perspective math
    ├── ParticleSystem.hpp
    ├── ParticleSystem.cpp    # 3D simulation, five scenes and batched rendering
    ├── AppSettings.hpp
    ├── AppSettings.cpp
    ├── Background.hpp
    ├── Background.cpp
    ├── PixelText.hpp
    └── UI.hpp
```

The SFML layer is used for the window and final 2D draw calls. The spatial simulation, scene behavior, camera basis, perspective projection, interaction math and trail history are implemented in the project itself in C++.

## Performance note

For meaningful performance testing, build and run the **Release** configuration. Debug builds intentionally disable or reduce many compiler optimizations and can be dramatically slower in a math-heavy real-time simulation. VSync can also cap the displayed FPS to the monitor refresh rate.

## Build on Windows

Install CMake and Visual Studio 2022 with **Desktop development with C++** (or another C++17 compiler).

In PowerShell from the project folder:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ParticleGarden.exe
```

SFML 2.6.2 is fetched automatically by CMake during the first configure, so the first build needs an internet connection.

You can also open the folder containing `CMakeLists.txt` directly in Visual Studio 2022, let CMake configure, then build and run `ParticleGarden`.

## Portfolio value

This version demonstrates real-time 3D vector simulation, custom camera mathematics, perspective projection, coordinate-space conversion, procedural scene design, world-space history, batched rendering, input/state management, immediate-mode UI construction, persistence and filesystem output — without hiding the core logic behind a game engine.

## License

MIT
