cmake_minimum_required(VERSION 3.12)

# Input variables.
set(dawn_version "20250621.085109")
set(dawn_build_date "20250630.1")

# Avoids matching wasm(32|64)_emscripten_windows_chrome_ext_vtk
# This is equivalent to `if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows(?!.*wasm)")`
# if CMake supported regex lookaheads
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "c626c476823261e77460d494c8b187ac58573974482f6993aed36e310c6d910e")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "2d988707f53fca93667b263de7f7155ce8dc7cdcb0dfab6346f01df2c6af7597")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "8801308bb8a0635012ac1b0379070201d5954116cad83389613d33203844dd9d")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "f1ca00e9eb356ef14f31affd312071953958d56aef57cc2b05001ebab338dc7d")
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
