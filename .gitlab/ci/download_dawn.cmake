cmake_minimum_required(VERSION 3.13)

# Input variables.
set(dawn_version "20260923.214225")
set(dawn_build_date "20260928.0")

# Avoids matching wasm(32|64)_emscripten_windows
# This is equivalent to `if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows(?!.*wasm)")`
# if CMake supported regex lookaheads
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "c5a8e7e4685e0071d18587ca9eef94218056da6777589ba3f6a44a87deff3f91")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "a3666ffb61d330829a009d93e70b88733eff7b76faa3ed79ded68da7d6314392")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "da261a36628c8cca5f746867f49122ba6e0a26c06cf533e752f3d993f9670b03")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "565a55cdb7bb3ee89309063f7483be4a9495d8f1e9bbe2a35f35f62e5d4c2be9")
else ()
  message(FATAL_ERROR "Unknown platform for dawn")
endif ()

set(dawn_url "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/dawn/v${dawn_version}-${dawn_build_date}")
set(dawn_file "dawn-v${dawn_version}-${dawn_platform}.${dawn_ext}")

# Download the file.
file(DOWNLOAD
  "${dawn_url}/${dawn_file}"
  ".gitlab/${dawn_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${dawn_hash}")

  # Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${dawn_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${dawn_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${dawn_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/dawn-v${dawn_version}-${dawn_platform}"
  ".gitlab/dawn")
