#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "--" ]; then
	shift
fi

export WINEPREFIX=/wineprefix
export WINEDEBUG=-all


cd /build
eval wine cmake -B . "Z:/source" ${CMAKE_FLAGS:-'-G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo'}
wine cmake --build .

sed -i 's|Z:||g; s|\\\\|/|g; s|\\|/|g' /build/reccmp-build.yml

have_original=0
if [ -f /original/AlienShooter.exe ]; then
	cd /source
	reccmp-project detect --what original --search-path /original
	have_original=1
fi

cd /build
if [ "$#" -gt 0 ]; then
	exec "$@"
fi
if [ "$have_original" -eq 1 ]; then
	exec reccmp-reccmp --target ALIEN
fi
echo ">> Build complete but no exe found in original"
