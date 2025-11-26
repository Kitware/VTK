cmake_minimum_required(VERSION 3.12)

# Input variables.
set(emsdk_version "4.0.20")
set(emsdk_ext "zip")
set(emsdk_hash "9153c801da503d266541bf018d68924bda4e405da45aeea608ed30959ed75a17")
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
