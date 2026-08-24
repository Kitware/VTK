# Toolchain wrapper that makes `emcmake` unnecessary.
#
# `emcmake cmake ...` does exactly two things: it appends
# `-DCMAKE_TOOLCHAIN_FILE=<emsdk>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`
# and `-DCMAKE_CROSSCOMPILING_EMULATOR=<node>`. Both can be expressed in a
# CMakePresets.json entry, provided something knows where the emsdk lives. That
# is what this file is for: point `CMAKE_TOOLCHAIN_FILE` at it (see the `wasm*`
# presets in CMakePresets.json) and it will
#
# 1. locate an emsdk, downloading and installing one into the per-user tool
#    cache (see vtkToolCacheDirectory.cmake) if there is none,
# 2. locate a standalone node -- never the older one bundled with the emsdk --
#    and set `CMAKE_CROSSCOMPILING_EMULATOR` to it,
# 3. include the real Emscripten toolchain file.
#
# emcc locates its own clang and wasm-ld relative to the emsdk layout, so
# neither `PATH` nor `EM_CONFIG` needs to be exported by the caller.
#
# Knobs (pass with -D or via a preset):
#
#   VTK_EMSDK_ROOT       Use this emsdk instead of searching. Also honored via
#                        the `EMSDK` environment variable, which
#                        `emsdk_env.sh`/`emsdk_env.bat` already sets.
#   VTK_EMSDK_VERSION    Version to install when bootstrapping. Defaults to the
#                        version CI uses.
#   VTK_BOOTSTRAP_EMSDK  Set to OFF to fail instead of downloading an emsdk or
#                        a node.
#   VTK_EMSCRIPTEN_PRELOAD_CACHE
#                        Preload known platform-check results for supported
#                        Emscripten versions. The wasm presets enable this.
#
# Node comes from FindNodeJS, so `NodeJS_INTERPRETER` and the `NODE_DIR`
# environment variable select one, as they do elsewhere in VTK.

# This file is re-read for every try_compile, so keep everything below cheap
# when the emsdk is already in place.
get_filename_component(_vtk_emsdk_vtk_source_dir "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

include("${CMAKE_CURRENT_LIST_DIR}/vtkToolVersions.cmake")

# The emscripten version selects the cache directory, so that switching versions
# does not overwrite the one already there.
set(_vtk_emsdk_version "${vtk_emsdk_version}")
if (VTK_EMSDK_VERSION)
  set(_vtk_emsdk_version "${VTK_EMSDK_VERSION}")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/vtkEmscriptenPreloadCache.cmake")

set(_vtk_emsdk_bootstrap 1)
if (DEFINED VTK_BOOTSTRAP_EMSDK AND NOT VTK_BOOTSTRAP_EMSDK)
  set(_vtk_emsdk_bootstrap 0)
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/vtkToolCacheDirectory.cmake")

if (VTK_EMSDK_ROOT)
  set(_vtk_emsdk_root "${VTK_EMSDK_ROOT}")
  set(_vtk_emsdk_managed 0)
elseif (DEFINED ENV{EMSDK} AND NOT "$ENV{EMSDK}" STREQUAL "")
  file(TO_CMAKE_PATH "$ENV{EMSDK}" _vtk_emsdk_root)
  set(_vtk_emsdk_managed 0)
elseif (EXISTS "${_vtk_emsdk_vtk_source_dir}/.gitlab/emsdk")
  # A CI-style local build already put one here; do not download a second copy.
  set(_vtk_emsdk_root "${_vtk_emsdk_vtk_source_dir}/.gitlab/emsdk")
  set(_vtk_emsdk_managed 0)
else ()
  # Shared between build trees and checkouts, and keeps a source archive clean.
  set(_vtk_emsdk_root "${vtk_tool_cache_directory}/emsdk-${_vtk_emsdk_version}/emsdk")
  set(_vtk_emsdk_managed 1)
endif ()

set(_vtk_emsdk_toolchain
  "${_vtk_emsdk_root}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

# Download and install an emsdk when we own the location and nothing is there.
if (NOT EXISTS "${_vtk_emsdk_toolchain}" AND _vtk_emsdk_managed)
  if (NOT _vtk_emsdk_bootstrap)
    message(FATAL_ERROR
      "No Emscripten SDK found at '${_vtk_emsdk_root}' and VTK_BOOTSTRAP_EMSDK "
      "is disabled. Install an emsdk and pass -DVTK_EMSDK_ROOT=/path/to/emsdk.")
  endif ()

  # emsdk drives its downloads through python. Use find_program rather than
  # FindPython3 because this runs before any language is enabled.
  find_program(_vtk_emsdk_python NAMES python3 python)
  if (NOT _vtk_emsdk_python)
    message(FATAL_ERROR
      "Bootstrapping the Emscripten SDK requires a python3 interpreter. "
      "Install one, or install an emsdk yourself and pass "
      "-DVTK_EMSDK_ROOT=/path/to/emsdk.")
  endif ()

  if (NOT EXISTS "${_vtk_emsdk_root}/emsdk.py")
    message(STATUS "Downloading the Emscripten SDK into ${_vtk_emsdk_root}")
    get_filename_component(_vtk_emsdk_download_dir "${_vtk_emsdk_root}" DIRECTORY)
    execute_process(
      COMMAND "${CMAKE_COMMAND}"
              "-Dvtk_emsdk_download_dir=${_vtk_emsdk_download_dir}"
              -P "${CMAKE_CURRENT_LIST_DIR}/vtkDownloadEmsdk.cmake"
      RESULT_VARIABLE _vtk_emsdk_result)
    unset(_vtk_emsdk_download_dir)
    if (_vtk_emsdk_result)
      message(FATAL_ERROR "Failed to download the Emscripten SDK.")
    endif ()
  endif ()

  # Fetches clang, the wasm link tools, emscripten and a node to run them with.
  # This takes a while and downloads on the order of a gigabyte.
  message(STATUS "Installing Emscripten ${_vtk_emsdk_version}; this may take several minutes")
  execute_process(
    COMMAND "${_vtk_emsdk_python}" "${_vtk_emsdk_root}/emsdk.py"
            install "${_vtk_emsdk_version}"
    WORKING_DIRECTORY "${_vtk_emsdk_root}"
    RESULT_VARIABLE _vtk_emsdk_result)
  if (_vtk_emsdk_result)
    message(FATAL_ERROR "Failed to install Emscripten ${_vtk_emsdk_version}.")
  endif ()

  execute_process(
    COMMAND "${_vtk_emsdk_python}" "${_vtk_emsdk_root}/emsdk.py"
            activate "${_vtk_emsdk_version}"
    WORKING_DIRECTORY "${_vtk_emsdk_root}"
    RESULT_VARIABLE _vtk_emsdk_result)
  if (_vtk_emsdk_result)
    message(FATAL_ERROR "Failed to activate Emscripten ${_vtk_emsdk_version}.")
  endif ()
endif ()

if (NOT EXISTS "${_vtk_emsdk_toolchain}")
  message(FATAL_ERROR
    "No Emscripten toolchain file at '${_vtk_emsdk_toolchain}'. Pass "
    "-DVTK_EMSDK_ROOT=/path/to/emsdk or source the emsdk's emsdk_env script.")
endif ()

# CMake runs the wrapping tools (vtkWrapHierarchy and friends) and some unit
# tests through this emulator.
#
# Deliberately *not* emsdk's bundled node: it lags the standalone releases, and
# wasm64 needs `--experimental-wasm-memory64` on anything older than node 24.
# The documented developer flow and CI both put a standalone node ahead of it,
# so hide the bundled copy from the search and let FindNodeJS do the rest. Its
# NODE_DIR hint is the same one CI exports, and it already knows that windows
# node releases have no bin/ directory.
if (NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR)
  if (NOT DEFINED ENV{NODE_DIR} OR "$ENV{NODE_DIR}" STREQUAL "")
    if (EXISTS "${_vtk_emsdk_vtk_source_dir}/.gitlab/node")
      set(ENV{NODE_DIR} "${_vtk_emsdk_vtk_source_dir}/.gitlab/node")
    else ()
      set(ENV{NODE_DIR} "${vtk_tool_cache_directory}/node-${vtk_node_version}/node")
    endif ()
  endif ()

  file(GLOB _vtk_emsdk_node_dirs "${_vtk_emsdk_root}/node/*/bin")
  set(_vtk_node_saved_ignore_path "${CMAKE_IGNORE_PATH}")
  list(APPEND CMAKE_IGNORE_PATH ${_vtk_emsdk_node_dirs})

  set(_vtk_node_saved_module_path "${CMAKE_MODULE_PATH}")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
  find_package(NodeJS QUIET)

  # Nothing usable found; fetch the same node CI uses. The download script picks
  # the platform and architecture off the host.
  if (NOT NodeJS_FOUND AND _vtk_emsdk_bootstrap)
    set(_vtk_node_cache_dir "${vtk_tool_cache_directory}/node-${vtk_node_version}")
    message(STATUS "Downloading node into ${_vtk_node_cache_dir}/node")
    execute_process(
      COMMAND "${CMAKE_COMMAND}"
              "-Dvtk_node_download_dir=${_vtk_node_cache_dir}"
              -P "${CMAKE_CURRENT_LIST_DIR}/vtkDownloadNode.cmake"
      RESULT_VARIABLE _vtk_emsdk_result)
    if (NOT _vtk_emsdk_result)
      # The failed lookup is cached; drop it so the download is picked up.
      unset(NodeJS_INTERPRETER CACHE)
      set(ENV{NODE_DIR} "${_vtk_node_cache_dir}/node")
      find_package(NodeJS QUIET)
    endif ()
    unset(_vtk_node_cache_dir)
  endif ()

  set(CMAKE_IGNORE_PATH "${_vtk_node_saved_ignore_path}")
  set(CMAKE_MODULE_PATH "${_vtk_node_saved_module_path}")
  unset(_vtk_emsdk_node_dirs)
  unset(_vtk_node_saved_ignore_path)
  unset(_vtk_node_saved_module_path)

  if (NOT NodeJS_FOUND)
    message(FATAL_ERROR
      "No standalone node found. VTK does not use the node bundled with emsdk "
      "because it is older than the version the wasm build targets. Install "
      "node 24 or newer and put it on PATH, point NODE_DIR at it, or pass "
      "-DNodeJS_INTERPRETER=/path/to/node.")
  endif ()

  set(_vtk_node_emulator "${NodeJS_INTERPRETER}")
  if (NodeJS_VERSION AND NodeJS_VERSION VERSION_LESS "24")
    message(WARNING
      "node ${NodeJS_VERSION} at '${NodeJS_INTERPRETER}' is older than the "
      "node 24 the wasm build targets. Consider a newer one.")
    # wasm64 memory is behind a flag until node 24.
    if (VTK_WEBASSEMBLY_64_BIT)
      list(APPEND _vtk_node_emulator "--experimental-wasm-memory64")
    endif ()
  endif ()

  set(CMAKE_CROSSCOMPILING_EMULATOR "${_vtk_node_emulator}"
    CACHE STRING "Path to the emulator for the target system.")

  # Point emcc's own node at the same binary. This only reaches the emcc
  # invocations CMake makes during configure; the build step runs under a
  # separate process, where emcc resolves node itself (PATH, else the emsdk
  # copy). That only affects emcc-internal JS tooling, not the emitted wasm, so
  # it is left alone. Export EM_NODE_JS yourself to pin it everywhere.
  set(ENV{EM_NODE_JS} "${NodeJS_INTERPRETER}")

  unset(_vtk_node_emulator)
endif ()

include("${_vtk_emsdk_toolchain}")

unset(_vtk_emsdk_bootstrap)
unset(_vtk_emsdk_managed)
unset(_vtk_emsdk_python CACHE)
unset(_vtk_emsdk_root)
unset(_vtk_emsdk_result)
unset(_vtk_emsdk_vtk_source_dir)
unset(_vtk_emsdk_toolchain)
unset(_vtk_emsdk_version)
unset(vtk_emsdk_hash)
unset(vtk_emsdk_version)
unset(vtk_node_hash_linux_x64)
unset(vtk_node_hash_win_x64)
unset(vtk_node_version)
unset(vtk_tool_cache_directory)
