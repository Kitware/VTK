cmake_minimum_required(VERSION 3.12)

set(python_url_root "https://www.paraview.org/files/dependencies/python-for-wheels")

set(python_version "3.11.4")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(python_subdir "python-${python_version}-windows-x86_64")
  set(sha256sum "699df2d656c7227c3ba93d640255cd875e3d92e1a475f5c59408c6125515165f")
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
