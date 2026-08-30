# Alien Shooter

## Building

Requirements:

- CMake
- C++20 compiler
- SDL3

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel
```

The single-configuration executable is `build/native/AlienShooter` on Unix-like
systems or `AlienShooter.exe` on Windows.

Game assets are not distributed here. Use the data from the
[GOG release](https://www.gog.com/en/game/alien_shooter_expansions) or retail
Alien Shooter 1.2.

Place the executable beside `objects.res` and
the game's data directories. Alternatively, point to that directory with
`--data-path="/path/to/Alien Shooter"`.

## Runtime options

- `--resolution=WIDTHxHEIGHT`, `--resolution=auto` (`desktop` is an alias),
  or separate `--width` and `--height`. New and automatically migrated
  profiles default to fullscreen at the exact desktop resolution. Use
  `--windowed` to opt out; automatic windowed mode fits a desktop-aspect
  client within 1280x800.
- Automatic fullscreen profiles use the desktop's exact output resolution and
  a same-aspect gameplay framebuffer capped to 2000 pixels wide and to the
  authored map bounds. This keeps campaign scale consistent and limits exposure
  of the large unbuilt areas inside maps whose outer headers are much bigger than
  their visible terrain (for example, 2560x1600 output uses a 2000x1250 game
  frame). Menus and other fixed presentations remain exact native resolution.
  HUD art is counter-scaled inside a capped gameplay
  frame so its final presented size remains the selected `--ui-scale` and its
  authored rows do not overlap. `--no-native-resolution` returns to a
  480-pixel-tall retail-scale Hor+ framebuffer.
- `--render-width=WIDTH` selects a non-native logical framebuffer capped at
  that width. Supplying `--native-resolution` as well keeps the map-bounded
  native policy; the native choice wins regardless of argument order.
- `--native-resolution` / `--no-native-resolution` explicitly select
  map-bounded native-aspect rendering or the retail-height/explicit-width
  policy.
- `--ui-scale=auto|1|2|3`
- `--fullscreen` / `--windowed`
- `--vsync` / `--no-vsync`
- `--red-blood` forces the original red blood effects
- `--data-path=PATH` and `--pref-path=PATH`
- `--config=PATH` or `--script=PATH` for the legacy startup argument

## File locations

Writable configuration, progress, saves, and logs use SDL's
[per-user preference directory](https://wiki.libsdl.org/SDL3/SDL_GetPrefPath).
The default locations are:

- Linux: `${XDG_DATA_HOME:-$HOME/.local/share}/SigmaTeam/AlienShooter/`
- Windows: `%APPDATA%\SigmaTeam\AlienShooter\`
- macOS: `~/Library/Application Support/SigmaTeam/AlienShooter/`

## License

[Sustainable Use License](LICENSE.md)
