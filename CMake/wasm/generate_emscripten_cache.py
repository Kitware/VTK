#!/usr/bin/env python3
"""Generate a vtkEmscriptenCache-<version>-<arch>.cmake preload file.

Usage from inside the CI container; version and arch are detected from the
configure itself:
    python3 generate_emscripten_cache.py --build-dir /output/build-...-cacheprobe

Omit --output to write CMake/wasm/vtkEmscriptenCache-<version>-<arch>.cmake
next to this script.
"""

import argparse
import re
from pathlib import Path

CACHE_LINE_RE = re.compile(r"^([^:=]+):([A-Za-z_]+)=(.*)$")

# The help-string prefixes CMake's check modules write above a probe-result
# INTERNAL cache entry. Seen across CheckTypeSize, CheckSymbolExists,
# CheckIncludeFile(s), CheckFunctionExists, CheckLibraryExists, CheckVariableExists,
# check_c(xx)_source_compiles/runs (via CMAKE_CHECK_* / project-local "Test NAME"
# macros), and CHECK_TYPE_SIZE's own "CHECK_TYPE_SIZE: sizeof(...)" wording.
PROBE_COMMENT_PREFIXES = ("//Result of", "//Have", "//Test", "//CHECK_TYPE_SIZE:")

HEADER = """\
# Generated from a successful VTK {arch} configure with Emscripten {version} and
# the settings in the `{arch}` configure preset. This file intentionally contains
# INTERNAL entries: CMake's check modules use their presence to avoid repeating
# platform probes. Do not use it for {other_arch}, whose ABI sizes differ -- see
# vtkEmscriptenCache-{version}-{other_arch}.cmake for that.
"""


def read_cache_lines(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def read_probe_results(lines: list[str]) -> dict[str, str]:
    results = {}
    for i, line in enumerate(lines):
        m = CACHE_LINE_RE.match(line)
        if not m:
            continue
        var, typ, val = m.groups()
        if typ != "INTERNAL":
            continue
        comment = lines[i - 1] if i > 0 and lines[i - 1].startswith("//") else ""
        if comment.startswith(PROBE_COMMENT_PREFIXES):
            results[var] = val
    return results


def read_cache_entry(lines: list[str], name: str) -> str | None:
    for line in lines:
        m = CACHE_LINE_RE.match(line)
        if m and m.group(1) == name:
            return m.group(3)
    return None


def detect_arch(lines: list[str]) -> str | None:
    """wasm32/wasm64 from the VTK_WEBASSEMBLY_64_BIT cache entry."""
    value = read_cache_entry(lines, "VTK_WEBASSEMBLY_64_BIT")
    if value is None:
        return None
    return "wasm64" if value.upper() in ("ON", "TRUE", "YES", "1") else "wasm32"


def detect_version(lines: list[str]) -> str | None:
    """Emscripten release from emscripten-version.txt above the toolchain file.

    CMAKE_TOOLCHAIN_FILE points at
    <emscripten-root>/cmake/Modules/Platform/Emscripten.cmake; walk up its
    parents until a directory containing emscripten-version.txt appears. Only
    works where that path still exists (i.e. inside the container the
    configure ran in, or on the same host).
    """
    toolchain = read_cache_entry(lines, "CMAKE_TOOLCHAIN_FILE")
    if not toolchain:
        return None
    for parent in Path(toolchain).parents:
        version_file = parent / "emscripten-version.txt"
        if version_file.is_file():
            return version_file.read_text(encoding="utf-8").strip().strip('"')
    return None


def render_set(name: str, value: str, version: str) -> str:
    value_repr = value if value != "" else '""'
    return (
        f'set({name} {value_repr} CACHE INTERNAL "Emscripten {version} probe result")'
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--build-dir", required=True, type=Path, help="Build directory")
    parser.add_argument(
        "--output",
        type=Path,
        help="Output path. Defaults to vtkEmscriptenCache-<version>-<arch>.cmake "
        "next to this script.",
    )
    args = parser.parse_args()

    cmake_cache = args.build_dir / "CMakeCache.txt"
    if not cmake_cache.is_file():
        print(f"error: {cmake_cache} does not exist")
        return 1

    lines = read_cache_lines(cmake_cache)

    arch = detect_arch(lines)
    if arch is None:
        print(
            "error: could not detect the architecture (no VTK_WEBASSEMBLY_64_BIT "
            f"entry in {cmake_cache}); was this a VTK configure?"
        )
        return 1

    version = detect_version(lines)
    if version is None:
        print(
            "error: could not detect the Emscripten version (the emsdk named by "
            f"CMAKE_TOOLCHAIN_FILE in {cmake_cache} is not on disk); "
            "run this where the configure ran, so the emsdk path resolves"
        )
        return 1

    other_arch = "wasm32" if arch == "wasm64" else "wasm64"
    output = args.output or (
        Path(__file__).resolve().parent / f"vtkEmscriptenCache-{version}-{arch}.cmake"
    )

    results = read_probe_results(lines)
    if not results:
        print(f"error: no probe-result INTERNAL entries found in {cmake_cache}")
        return 1

    entries = [
        render_set(name, value, version) for name, value in sorted(results.items())
    ]

    output.write_text(
        HEADER.format(arch=arch, other_arch=other_arch, version=version)
        + "\n".join(entries)
        + "\n"
    )

    print(f"wrote {len(entries)} entries for ({version}, {arch}) to {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
