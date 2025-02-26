cmake_minimum_required(VERSION 3.12)

# Input variables.
set(dawn_version "7037")
set(dawn_build_date "20250226.0")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(dawn_platform "windows-x86_64")
  set(dawn_ext "zip")
  set(dawn_hash "a4f1f8bd5366c9d3803f95e16b9f39072a37f7141e8d3a505f179a70ce31dcc4")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(dawn_platform "macos-arm64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "7e7601120da983094635d639b218ea0aa53de33b5a4391dfbfbc53a1b080f6e9")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(dawn_platform "macos-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "19bbd9e8809621c85a2578999a3f48bbfba923c0dedcf37c7f3c95c47d6da9e2")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(dawn_platform "linux-x86_64")
  set(dawn_ext "tar.gz")
  set(dawn_hash "1bee6458e1811b9ba3dcc7ff6ef2f73fafe64edbfb0ac72a3c99827fa23e51d6")
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
