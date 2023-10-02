cmake_minimum_required(VERSION 3.12)

# Input variables.
set(openxr_version "1.0.29")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(openxr_platform "windows")
  set(openxr_ext "zip")
  set(openxr_hash "88bbba650cccdca089ff956783ff18530e2d1e650b340dc89227364a360c736e")
else ()
  message(FATAL_ERROR
    "Unknown platform for OpenXR")
endif ()

set(openxr_url_root "https://paraview.org/files/dependencies/")
set(openxr_file "openxr_loader_${openxr_platform}-${openxr_version}.${openxr_ext}")

# Download the file.
file(DOWNLOAD
  "${openxr_url_root}/${openxr_file}"
  ".gitlab/openxr/${openxr_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${openxr_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${openxr_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar 
    xf "${openxr_file}"
  WORKING_DIRECTORY ".gitlab/openxr"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${openxr_file}: ${err}")
endif ()
