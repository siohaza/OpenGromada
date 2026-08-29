#!/usr/bin/env bash
set -euo pipefail

DECOMP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$DECOMP_DIR/reccmp/build.sh" >/dev/null

cd "$DECOMP_DIR/build"

report="$(mktemp)"
trap 'rm -f "$report"' EXIT
set +e
reccmp-reccmp --target ALIEN "$@" 2>&1 | tee "$report"
status=${PIPESTATUS[0]}
set -e

if grep -Fq '[ERROR]' "$report"; then
	exit 1
fi
exit "$status"
