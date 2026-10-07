# OpenGromada

Reverse engineered engine that is used in variety of games developed by Sigma Games and some of 3rdparty games using this engine. Naming comes from the internal naming of the engine in registry code.

## Features

- Supports AS1/ZS1, Theseus, Crazy Lunch, Locoland/Steamland.
- Cross-platform and cross-arch support
- Widescreen resolution support
- Gamepad support
- Bumped engine limits up to 8192. Use `GETSPRITE_VID + nvid` for the
entire range

## Building

Requirements:

- CMake/Premake
- C++20 compiler
- SDL3
- Steamworks SDK (optional, needed only for Steam's AS1)
- FFmpeg 6+ (optional, needed for Locoland videos)

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel
```

The single-configuration executable is `build/native/OpenGromada` on Unix-like
systems or `OpenGromada.exe` on Windows.


Place the executable beside `objects.res` and
the game's data directories. Alternatively, point to that directory with
`--data-path="/path/to/game"`.

## Runtime options

| Option | What it does |
|---|---|
| `--renderer=auto\|gpu\|software` | Selects actual game rendering. `auto` tries the GPU and falls back to software if initialization fails. `gpu` requires the GPU. `software` uses the CPU renderer. |
| `--gpu-driver=vulkan\|direct3d12\|metal` | Forces a graphics driver. Requires `--renderer=gpu`. Without this option SDL selects the driver. |
| `--resolution=WIDTHxHEIGHT` | Sets the resolution. `--resolution=auto` (alias: `desktop`) uses the desktop resolution. You can also give `--width` and `--height`. |
| `--fullscreen` / `--windowed` | Selects fullscreen or windowed mode. New profiles start in fullscreen at the desktop resolution. Automatic windowed mode fits the window inside 1280x800. |
| `--native-resolution` | The default. The game renders gameplay at the native resolution, inside a frame with a maximum size of 1280x720 (1152x720 on 16:10 screens). Menus always render at the full native resolution. On maps with small terrain, the game can select a smaller frame to hide unbuilt areas. |
| `--no-native-resolution` | Uses the profile's base height frame. The frame width follows the screen aspect. |
| `--render-width=WIDTH` | Limits the width of the render frame to WIDTH. If you also give `--native-resolution`, the native mode wins. |
| `--ui-scale=auto\|1\|2\|3` | Sets the size of the HUD and menu art. `auto` selects a scale from the resolution, with a maximum of 2x during gameplay. Explicit values are exact. |
| `--vsync` / `--no-vsync` | Turns vertical sync on or off. |
| `--red-blood` | Shows the original red blood effects. |
| `--data-path=PATH` / `--data-dir=PATH` | Sets the folder that holds the game data. |
| `--game=as1\|zs1\|theseus\|crazy-lunch\|locoland` | Selects a title explicitly. |
| `--probe-game=json` | Prints detection, configuration, resource sections, candidates and runtime/movie availability, without starting the game. |
| `--pref-path=PATH` | Sets the folder for saves, settings, and logs. |
| `--config=PATH` | Selects the startup configuration before opening resources. |
| `--script=PATH` | Overrides the startup map/script. Directly starting a later campaign map can skip initialization performed by earlier maps. |

## File locations

Writable configuration, progress, saves, and logs use SDL's
[per-user preference directory](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath).
The default locations are:

- Linux: `${XDG_DATA_HOME:-$HOME/.local/share}/SigmaTeam/`
- Windows: `%APPDATA%\SigmaTeam\`
- macOS: `~/Library/Application Support/SigmaTeam/`

## License

[Sustainable Use License](LICENSE.md)
