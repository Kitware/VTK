cmake_minimum_required(VERSION 3.13)

# Input variables.
set(dawn_version "20260720.160313")
set(dawn_build_date "20260727.0")

# Avoids matching wasm(32|64)_emscripten_windows
# This is equivalent to `if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows(?!.*wasm)")`
# if CMake supported regex lookaheads
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "9622fec57e4aad13abd41304de4678376a6434a6893242060f1dbcb8d64869ff")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "9ec5be1a4e8a23b32914670cadaf01a7a061db482520643cdb840b7ad2e3aee1")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "9676fdf2f89fd44a1678abdc56feda52d672523c1e54ea446e9c7a049a08f3d9")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "29c8c5912b7232b227a7345e8274cacc4f46bf00dee50fee3c4e4caa0e614be4")
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
