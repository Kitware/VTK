cmake_minimum_required(VERSION 3.12)

# Input variables.
set(dawn_version "7153")
set(dawn_build_date "20250502.0")

# Avoids matching wasm(32|64)_emscripten_windows_chrome_ext_vtk
# This is equivalent to `if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows(?!.*wasm)")`
# if CMake supported regex lookaheads
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows" AND NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "fe52157fd1234010d25d5d6bb0922fc718df95bff1be47a303e176d2c4295cad")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "01810d72ce560958c26132f5e7da62aac30069f3b5484f4c57b340d23d81af23")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "72aaac22a3e9172060fea92a5b2efdd65c0fc82b585642ab5dd9dd35ad1449ad")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "95b253cd52c5c0ab8ceff9a5bc37627353dde6430006b9460a4b0d5467d90f3a")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm32")
  set(dawn_platform "wasm32-emscripten")
  set(dawn_ext "tar.gz")
  set(dawn_hash "dfa56d448ae16df92f45c429582856ac1e575bd335397e6d95e589aeb98e5660")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "wasm64")
  set(dawn_platform "wasm64-emscripten")
  set(dawn_ext "tar.gz")
  set(dawn_hash "5c20ab08d2ef5b1c10a9c27b83202bc837fea9eb1c5fb5ae63f41aaebc98bc61")
else ()
  message(FATAL_ERROR "Unknown platform for dawn")
endif ()

set(dawn_url "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/dawn/v${dawn_version}-${dawn_build_date}")
set(dawn_file "dawn-${dawn_version}-${dawn_platform}.${dawn_ext}")

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
  ".gitlab/dawn-${dawn_version}-${dawn_platform}"
  ".gitlab/dawn")
