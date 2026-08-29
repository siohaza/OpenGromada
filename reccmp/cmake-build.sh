#!/usr/bin/env bash
set -euo pipefail

export WINEPREFIX="${WINEPREFIX:-$HOME/.local/share/alien-vc6}"
export WINEDEBUG="${WINEDEBUG:--all}"

DECOMP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DRIVE_C="$WINEPREFIX/drive_c"
CMAKE_VER="3.26.6"
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"

if [ ! -x "$DRIVE_C/cmake/bin/cmake.exe" ]; then
	echo ">> Installing Windows CMake $CMAKE_VER into the prefix"
	zip="$(mktemp --suffix=.zip)"
	curl -fsSL -o "$zip" \
		"https://github.com/Kitware/CMake/releases/download/v$CMAKE_VER/cmake-$CMAKE_VER-windows-i386.zip"
	rm -rf "$DRIVE_C/cmake"
	unzip -q "$zip" -d "$DRIVE_C"
	mv "$DRIVE_C/cmake-$CMAKE_VER-windows-i386" "$DRIVE_C/cmake"
	rm -f "$zip"
	wine regedit /S "$(dirname "${BASH_SOURCE[0]}")/set-env.reg"
	wineserver -w
fi

src_win="$(winepath -w "$DECOMP_DIR")"
rm -rf "$DECOMP_DIR/build-cmake"
mkdir -p "$DECOMP_DIR/build-cmake"

echo ">> configure (NMake Makefiles, $BUILD_TYPE)"
( cd "$DECOMP_DIR/build-cmake" && \
  wine cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    "$src_win" )

echo ">> build"
( cd "$DECOMP_DIR/build-cmake" && wine cmake --build . )

if [ -f "$DECOMP_DIR/build-cmake/reccmp-build.yml" ]; then
	sed -i 's|Z:||g; s|\\\\|/|g; s|\\|/|g' "$DECOMP_DIR/build-cmake/reccmp-build.yml"
fi

echo ">> Done. reccmp-build.yml:"
cat "$DECOMP_DIR/build-cmake/reccmp-build.yml"
