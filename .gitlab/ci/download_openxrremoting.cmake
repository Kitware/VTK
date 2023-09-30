cmake_minimum_required(VERSION 3.12)

# Input variables.
set(openxrremoting_version "2.9.2")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(openxrremoting_platform "windows")
  set(openxrremoting_ext "nupkg")
  set(openxrremoting_hash "ac7361f036dee18ed702c918eebf55cbff9a0b64cb91b88ea7032d1bd05ead0a")
else ()
  message(FATAL_ERROR
    "Unknown platform for OpenXRRemoting")
endif ()

set(openxrremoting_url_root "https://paraview.org/files/dependencies/")
set(openxrremoting_file "microsoft.holographic.remoting.openxr.${openxrremoting_version}.${openxrremoting_ext}")

# Download the file.
file(DOWNLOAD
  "${openxrremoting_url_root}/${openxrremoting_file}"
  ".gitlab/openxrremoting/${openxrremoting_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${openxrremoting_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${openxrremoting_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar 
    xf "${openxrremoting_file}"
  WORKING_DIRECTORY ".gitlab/openxrremoting"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${openxrremoting_file}: ${err}")
endif ()
