cmake_minimum_required(VERSION 3.12)

# Input variables.
set(adios2_version "2.9.2")
set(adios2_build_date "20231220.0")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(adios2_platform "windows-x86_64")
  set(adios2_ext "zip")
  set(adios2_hash "7e071e43f88c373bc664b5c66489f99feadf6019deef30d000e6fc7dd5f0a8da")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(adios2_platform "macos-arm64")
  set(adios2_ext "tar.gz")
  set(adios2_hash "8e0b86bf9a276743221009ba1c7ad797b24195bdb08b02b843314dcc7eeda548")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(adios2_platform "macos-x86_64")
  set(adios2_ext "tar.gz")
  set(adios2_hash "6638a3cf139c106cdc02a4430190d63868446ba3bf48f7e0ecbbf41e9ac5a666")
else ()
  message(FATAL_ERROR
      "Unknown platform for ADIOS2")
endif ()

set(adios2_url "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/adios2/v${adios2_version}-${adios2_build_date}")
set(adios2_file "adios2-v${adios2_version}-${adios2_platform}.${adios2_ext}")

# Download the file.
file(DOWNLOAD
  "${adios2_url}/${adios2_file}"
  ".gitlab/${adios2_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${adios2_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${adios2_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${adios2_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${adios2_file}: ${err}")
endif ()
