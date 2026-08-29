# Alien Shooter

## Building

Requirements:

- CMake
- [Visual C++ 6.0 SP4 compiler](https://github.com/OmniBlade/decomp.me/releases/tag/msvcwin9x)
- [Visual C++ 6.0 SP4 libraries](https://github.com/archaic-msvc/msvc600_sp4)

From the repository root, run the following commands:

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

The resulting executable is `build/AlienShooter.exe`.

## License
[Sustainable Use License](LICENSE.md)
