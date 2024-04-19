cmake_minimum_required(VERSION 3.12)

set(osmesa_url_root "https://www.paraview.org/files/dependencies")

set(osmesa_version "22.3.2")
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "osmesa")
  return ()
endif ()

set(date "20230709")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
  set(arch "x86_64")
  set(sha256sum "88712deb82dac88811e61ec2d72135b294cfb36be142c39a97e73d717331f1ea")
else ()
  message(FATAL_ERROR
    "Unknown platform for OSMesa")
endif ()
set(filename "vtk-osmesa-${osmesa_version}-${arch}-${date}.tar.xz")

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
  WORKING_DIRECTORY "/"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${filename}: ${err}")
endif ()
