cmake_minimum_required(VERSION 3.12)

# Input variables.
set(emsdk_version "4.0.10")
set(emsdk_ext "zip")
set(emsdk_hash "ec058146a1aa7c166e04fc7af560adfed05a9bbaed42821ad67fb2acbe666fd0")
set(emsdk_suffix "${emsdk_version}.${emsdk_ext}")

set(emsdk_url "https://github.com/emscripten-core/emsdk/archive/refs/tags")
set(emsdk_file "emsdk-${emsdk_suffix}")

# Download the file.
file(DOWNLOAD
  "${emsdk_url}/${emsdk_suffix}"
  ".gitlab/${emsdk_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${emsdk_hash}")

  # Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${emsdk_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${emsdk_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${emsdk_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/emsdk-${emsdk_version}"
  ".gitlab/emsdk")
