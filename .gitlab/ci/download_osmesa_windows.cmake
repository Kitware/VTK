cmake_minimum_required(VERSION 3.12)

set(osmesa_version "22.3.3")

if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  return ()
endif ()

set(date "20230328")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
  set(platform "windows")
  set(arch "AMD64")
  set(ext "zip")
  set(sha256sum "4fb7f600dc42c3799d4b6ea5182a12bf1d7f822cc578682be9388ba35fcbbe64")
else ()
  message(FATAL_ERROR
    "Unknown platform for OSMesa")
endif ()
set(basename "osmesa-${osmesa_version}-${platform}-${arch}")
set(filename "${basename}.${ext}")

set(osmesa_url_root "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/osmesa-sdk/v${osmesa_version}-${date}.1")

# Download the file.
file(DOWNLOAD
  "${osmesa_url_root}/${filename}"
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
    xf "$ENV{CI_PROJECT_DIR}/.gitlab/${filename}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${filename}: ${err}")
endif ()

file(RENAME
  ".gitlab/${basename}"
  ".gitlab/osmesa")
