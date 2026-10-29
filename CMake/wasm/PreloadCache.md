# Harvesting the Emscripten platform-check cache

A fresh VTK configure runs hundreds of serial `try_compile`/`try_run` checks.
Each one starts `emcc`'s Python driver for both compilation and linking, which
dominates configure time for the wasm builds. `vtkEmscriptenCache-<version>-wasm32.cmake`
and `vtkEmscriptenCache-<version>-wasm64.cmake` preload the results of those
checks as `CACHE INTERNAL` entries, so CMake's check modules see them already
answered and skip the probe. They're consumed by `vtkEmscriptenPreloadCache.cmake`
(included from both `vtkEmscriptenToolchain.cmake` and the `.gitlab/ci/configure_wasm*.cmake`
CI files) whenever `VTK_EMSCRIPTEN_PRELOAD_CACHE` is on.

These files are specific to one `(emsdk version, architecture)`
pair. The wasm32 and wasm64 need separate files because their ABI sizes (`long`, `size_t`, `ptrdiff_t`, `uintptr_t`, ...)
differ. Regenerate both whenever `vtk_emsdk_version` in `vtkToolVersions.cmake`
is bumped; a version with no matching cache file is simply not preloaded.

## 1. Run a real configure and harvest it, inside the CI container

Do this once per architecture, using the CI container so the environment
matches what CI actually runs with. The image name and tag are whatever the
wasm CI jobs currently use: in `.gitlab/os-linux.yml`, the
`.wasm32_emscripten_linux`/`.wasm64_emscripten_linux` jobs extend
`.fedora44_x86_64`, whose `image:` line names it (e.g.
`kitware/vtk:ci-fedora44-<tag>`). Check there rather than trusting the
example below to be current.

```bash
git diff .gitlab/ci/configure_wasm_common.cmake   # should be empty before you start
# comment out the VTK_EMSCRIPTEN_PRELOAD_CACHE set() and the two include()s,
# run the harvest below, then:
git checkout .gitlab/ci/configure_wasm_common.cmake
```

```bash
./CMake/wasm/run_harvest.sh 32 # generates cache for 32-bit wasm
./CMake/wasm/run_harvest.sh 64 # generates cache for 64-bit wasm
```

## 2. What `generate_emscripten_cache.py` does

`generate_emscripten_cache.py`, next to this document, scans the fresh
`CMakeCache.txt` for the `INTERNAL` entries CMake's check modules use to
record a probe result, and writes `vtkEmscriptenCache-<version>-<arch>.cmake`
into `CMake/wasm/`.

With `--build-dir` it works out `<version>` and `<arch>` from the configure
itself: the architecture from `VTK_WEBASSEMBLY_64_BIT` in the cache, and the
version from the `emscripten-version.txt` next to the emsdk toolchain file the
cache points at (which is why the script is best run inside the container,
where that path exists).

Run the harvest once per architecture. The auto-detected version is whatever
emsdk actually installed. Please confirm it matches the `vtk_emsdk_version` you're
pinning in `vtkToolVersions.cmake` before committing the result.
