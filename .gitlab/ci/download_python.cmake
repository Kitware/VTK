cmake_minimum_required(VERSION 3.12)

set(python_url_root "https://www.paraview.org/files/dependencies/python-for-wheels")

set(python_version "3.11.1")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(python_subdir "python-${python_version}-windows-x86_64")
  set(sha256sum "b1261ea4cdef506cc49ff7c4c25678a926e9382e961f1dd844326de3c5d25310")
else ()
  message(FATAL_ERROR
    "Unknown platform for Python")
endif ()
set(filename "${python_subdir}.zip")

# Download the file.
file(DOWNLOAD
  "${python_url_root}/${filename}"
  ".gitlab/${filename}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${sha256sum}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${filename}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${filename}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${filename}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/${python_subdir}"
  ".gitlab/python")
