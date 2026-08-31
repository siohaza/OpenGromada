# Alien Shooter

## Features

- Cross-platform and cross-arch support
- Widescreen resolution support
- Gamepad support
- Bumped engine limits. Use `GETSPRITE_VID + nvid` for the
entire range

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
  fit gameplay inside a same aspect 1280x720 envelope. A 16:9 output renders a
  1280x720 game frame and a 16:10 output renders 1152x720. Terrain constrained
  maps can select a smaller safe frame so unbuilt areas outside the authored
  terrain remain hidden. Menus and other fixed presentations remain at the
  exact native resolution. HUD art is counter-scaled inside the gameplay frame;
  automatic desktop HUD scaling is capped at 2x and explicit `--ui-scale`
  choices remain exact. `--no-native-resolution` returns to a 480-pixel-tall
  retail-scale Hor+ framebuffer.
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
