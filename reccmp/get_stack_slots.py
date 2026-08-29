#!/usr/bin/env python3
"""Extract stack slot definitions for a given function from MSVC assembly files."""

import argparse
import glob
import re
import subprocess
import sys
from pathlib import Path
from typing import Optional


def proc_line_pattern(function_name: str) -> re.Pattern:
    """PROC-line matcher covering both C listings (_Random PROC) and MSVC C++
    mangled listings (?Random@@YAHH@Z PROC NEAR ; Random, COMDAT). The name may
    be given bare (Random, Flagman) or qualified (MAP::Flagman)."""
    name = re.escape(function_name)
    return re.compile(rf"(?<![A-Za-z0-9]){name}(?![A-Za-z0-9_]).*\bPROC\b")


def find_function_file(function_name: str, build_dir: Path) -> Optional[Path]:
    """Find which .asm file contains the given function."""
    search_pattern = f"{function_name}.*PROC"
    asm_files = glob.glob(str(build_dir / "**" / "*.asm"), recursive=True)

    if not asm_files:
        return None

    try:
        result = subprocess.run(
            ["grep", "-l", search_pattern] + asm_files,
            capture_output=True,
            text=True,
        )

        if result.returncode == 0 and result.stdout.strip():
            return Path(result.stdout.strip().split('\n')[0])

        return None
    except Exception:
        return None


def extract_stack_slots(asm_file: Path, function_name: str) -> list[str]:
    """Extract stack slot definitions for the given function."""
    proc_pattern = proc_line_pattern(function_name)
    slot_pattern = re.compile(r'^(_[\w$]+) = (-?\d+)$')

    with open(asm_file, 'r') as f:
        lines = f.readlines()

    proc_line_idx = None
    for idx, line in enumerate(lines):
        if proc_pattern.search(line):
            proc_line_idx = idx
            break

    if proc_line_idx is None:
        return []

    stack_slots = []
    for idx in range(proc_line_idx - 1, -1, -1):
        line = lines[idx].strip()

        if "_TEXT SEGMENT" in line:
            break
        if "PROC" in line and not proc_pattern.search(line):
            break

        match = slot_pattern.match(line)
        if match:
            var_name = match.group(1)
            offset = int(match.group(2))

            clean_var_name = var_name.lstrip('_').rstrip('$')

            if offset < 0:
                hex_str = f"[ebp - {hex(-offset)}]"
            else:
                hex_str = f"[ebp + {hex(offset)}]"

            stack_slots.append((offset, f"{clean_var_name} = {hex_str}"))
        elif line and not line.startswith(';'):
            if not (line == "" or line.startswith(";")):
                break

    stack_slots.sort(key=lambda x: x[0], reverse=True)

    return [slot[1] for slot in stack_slots]


def main():
    parser = argparse.ArgumentParser(
        description="Extract stack slot definitions for a function from MSVC assembly"
    )
    parser.add_argument("function_name", help="Function name, bare or CLASS::Method qualified")
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=None,
        help="Directory with the /FAs .asm listings (default: <repo>/build)",
    )
    args = parser.parse_args()

    build_dir = args.build_dir or Path(__file__).resolve().parent.parent / "build"

    if not build_dir.exists():
        print(f"Error: {build_dir} directory not found", file=sys.stderr)
        return 1

    asm_file = find_function_file(args.function_name, build_dir)

    if asm_file is None:
        print(f"Error: Function '{args.function_name}' not found in {build_dir}", file=sys.stderr)
        return 1

    stack_slots = extract_stack_slots(asm_file, args.function_name)

    for slot in stack_slots:
        print(slot)

    return 0


if __name__ == "__main__":
    sys.exit(main())
