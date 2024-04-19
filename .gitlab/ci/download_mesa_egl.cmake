cmake_minimum_required(VERSION 3.12)

set(mesa_url_root "https://www.paraview.org/files/dependencies")

set(mesa_version "22.3.2")
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  return ()
endif ()
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "egl")
  return ()
endif ()

set(date "20231213")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
  set(arch "x86_64")
  set(sha256sum "c9e48e9bd32daaba705c379345e05234ba1219794a64fa58f7b49ca417dce8de")
else ()
  message(FATAL_ERROR
    "Unknown platform for Mesa EGL")
endif ()
set(filename "vtk-mesa-egl-${mesa_version}-${arch}-${date}.tar.xz")

# Download the file.
file(DOWNLOAD
  "${mesa_url_root}/${filename}"
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
