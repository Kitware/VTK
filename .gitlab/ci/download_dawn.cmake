cmake_minimum_required(VERSION 3.12)

# Input variables.
set(dawn_version "20251002.162335")
set(dawn_build_date "20260130.0")

# Avoids matching wasm(32|64)_emscripten_windows
# This is equivalent to `if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows(?!.*wasm)")`
# if CMake supported regex lookaheads
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "500afd33a3b3ab1e3a7153900a18f32e9979ec4c9021d645c133f0e2db3ba198")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "1e4537f51cc39500fee35cb0ab8f40b0491fe8cd9b1d8c6cc87eba35dcdf16eb")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "02c9f98d9cd30aecdfa8fa96a214f2efc8e002015c539b4a4483d7a20a0aaa34")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "a0f846e06f0ebdfe5f74b0fc193e3b74421ae9c9524a9d37cf9bf39c52110921")
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
