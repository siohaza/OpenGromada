#!/usr/bin/env bash
set -euo pipefail

WINEPREFIX="${WINEPREFIX:-$HOME/.local/share/alien-vc6}"
MSVC_DRIVE_C="$WINEPREFIX/drive_c/msvc6"
MSVC64_URL="https://github.com/OmniBlade/decomp.me/releases/download/msvcwin9x/msvc6.4.tar.gz"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DECOMP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LIB_SRC="${LIB_SRC:-$DECOMP_DIR/../dev-docs/msvc600_sp4/VC98}"

export WINEPREFIX WINEDEBUG="${WINEDEBUG:--all}"

echo ">> Wine prefix: $WINEPREFIX"
mkdir -p "$WINEPREFIX"
wineboot --init
wineserver -w

echo ">> Installing compiler (OmniBlade msvc6.4: Bin/Include/ATL/MFC)"
mkdir -p "$MSVC_DRIVE_C"
tmp_tar="$(mktemp --suffix=.tar.gz)"
trap 'rm -f "$tmp_tar"' EXIT
curl -fsSL -o "$tmp_tar" "$MSVC64_URL"
tar xzf "$tmp_tar" -C "$MSVC_DRIVE_C" Bin Include ATL MFC

echo ">> Installing libraries (msvc600_sp4 VC98/lib -> msvc6/Lib)"
if [ ! -f "$LIB_SRC/lib/kernel32.lib" ]; then
	echo "!! Missing $LIB_SRC/lib -- set LIB_SRC to the msvc600_sp4/VC98 dir" >&2
	exit 1
fi
rm -rf "$MSVC_DRIVE_C/Lib"
cp -r "$LIB_SRC/lib" "$MSVC_DRIVE_C/Lib"
if [ -d "$LIB_SRC/crt/src" ]; then
	rm -rf "$MSVC_DRIVE_C/crt"
	cp -r "$LIB_SRC/crt" "$MSVC_DRIVE_C/crt"
fi

echo ">> Seeding HKCU\\Environment from set-env.reg (INCLUDE/LIB/PATH/TMP)"
wine regedit /S "$SCRIPT_DIR/set-env.reg"
wineserver -w

echo ">> Done. Verify with: WINEPREFIX=$WINEPREFIX wine C:\\msvc6\\Bin\\CL.EXE"
