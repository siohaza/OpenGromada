#!/usr/bin/env bash
set -euo pipefail

export WINEPREFIX="${WINEPREFIX:-$HOME/.local/share/alien-vc6}"
export WINEDEBUG="${WINEDEBUG:--all}"

DECOMP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$DECOMP_DIR/build"
OUT_NAME="AlienShooter"
RESOURCE_DIR="$DECOMP_DIR/resources"
RESOURCE_RES="$RESOURCE_DIR/AlienShooter.res"

CL='C:\msvc6\Bin\CL.EXE'
LINK='C:\msvc6\Bin\LINK.EXE'
CFLAGS='/nologo /c /O2 /G6 /Z7 /MT /W3 /FAs /DWIN32 /D_WINDOWS /DNDEBUG /DENABLE_DECOMP_ASSERTS'
LFLAGS="/nologo /DEBUG /OPT:NOREF /INCREMENTAL:NO /MACHINE:IX86 /SUBSYSTEM:WINDOWS /MAP:AlienShooter.map"
LIBS='libcmt.lib kernel32.lib user32.lib gdi32.lib winmm.lib advapi32.lib ole32.lib comdlg32.lib shell32.lib d3d8.lib d3dx8.lib dsound.lib'

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
if [ ! -f "$RESOURCE_RES" ]; then
	echo "Checked-in resource is missing: $RESOURCE_RES" >&2
	exit 1
fi
cd "$BUILD_DIR"

incflags=("/I$(winepath -w "$DECOMP_DIR/src")")
incflags+=("/I$(winepath -w "$DECOMP_DIR/3rdparty/dx8/inc")")
incflags+=("/I$(winepath -w "$DECOMP_DIR/3rdparty/dx8/inc/dxsdk")")

LFLAGS+=" /LIBPATH:$(winepath -w "$DECOMP_DIR/3rdparty/dx8/lib/x86/dxsdk")"

mapfile -t SRCS < <(find "$DECOMP_DIR/src" \( -name '*.cpp' -o -name '*.c' \) | sort)

objs=()
for src in "${SRCS[@]}"; do
	wsrc="$(winepath -w "$src")"
	echo ">> cl $src"
	wine "$CL" $CFLAGS "${incflags[@]}" "$wsrc" >/dev/null
	objs+=("$(basename "${src%.*}").obj")
done

for provider_obj in picture_makevid_getpixelt.obj named_list_logicvar_expand.obj named_list_string.obj; do
	for i in "${!objs[@]}"; do
		if [ "${objs[i]}" = "$provider_obj" ]; then
			provider="${objs[i]}"
			unset 'objs[i]'
			objs=("$provider" "${objs[@]}")
			break
		fi
	done
done

echo ">> link -> $OUT_NAME.exe"
wine "$LINK" $LFLAGS "/OUT:$OUT_NAME.exe" "/PDB:$OUT_NAME.pdb" "${objs[@]}" \
	"$(winepath -w "$RESOURCE_RES")" $LIBS >/dev/null

echo ">> writing reccmp-build.yml"
cat > "$BUILD_DIR/reccmp-build.yml" <<EOF
project: '$DECOMP_DIR'
targets:
  ALIEN:
    path: '$BUILD_DIR/$OUT_NAME.exe'
    pdb: '$BUILD_DIR/$OUT_NAME.pdb'
EOF

ls -la "$BUILD_DIR/$OUT_NAME.exe" "$BUILD_DIR/$OUT_NAME.pdb"
echo ">> Done. Run reccmp from $BUILD_DIR"
