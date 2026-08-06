#[==[
Downloads and extracts NodeJS.

Run it with `cmake -P`, optionally passing `-Dvtk_node_download_dir=<dir>`. The
archive is extracted to `<vtk_node_download_dir>/node`; the directory defaults
to a version-stamped subdirectory of the per-user tool cache, so that several
versions coexist (see vtkToolCacheDirectory.cmake). CI passes `.gitlab` instead,
which is unversioned because CI jobs start from a clean checkout.

The platform is taken from the `CMAKE_CONFIGURATION` environment variable when
CI sets one, and from the host system otherwise.
#]==]
cmake_minimum_required(VERSION 3.12)

include("${CMAKE_CURRENT_LIST_DIR}/vtkToolVersions.cmake")
set(node_version "${vtk_node_version}")
set(node_baseurl "https://nodejs.org/download/release/v${node_version}")

# CI names its configurations after the platform; a developer running this
# directly has no such variable, so fall back to the host.
set(node_configuration "$ENV{CMAKE_CONFIGURATION}")
if (node_configuration STREQUAL "")
  set(node_configuration "${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif ()

if (node_configuration MATCHES "[Ww]indows")
  set(node_platform "win-x64")
  set(node_ext "zip")
  set(node_hash "${vtk_node_hash_win_x64}")
elseif (node_configuration MATCHES "[Ll]inux")
  set(node_platform "linux-x64")
  set(node_ext "tar.gz")
  set(node_hash "${vtk_node_hash_linux_x64}")
elseif (node_configuration MATCHES "[Dd]arwin|macos")
  set(node_ext "tar.gz")
  if (node_configuration MATCHES "arm64|aarch64")
    set(node_platform "darwin-arm64")
    set(node_hash "${vtk_node_hash_darwin_arm64}")
  else ()
    set(node_platform "darwin-x64")
    set(node_hash "${vtk_node_hash_darwin_x64}")
  endif ()
else ()
  message(FATAL_ERROR
      "Unknown platform for node ${node_configuration}")
endif ()
set(node_file "node-v${node_version}-${node_platform}.${node_ext}")

if (NOT DEFINED vtk_node_download_dir)
  include("${CMAKE_CURRENT_LIST_DIR}/vtkToolCacheDirectory.cmake")
  set(vtk_node_download_dir "${vtk_tool_cache_directory}/node-${node_version}")
endif ()
file(MAKE_DIRECTORY "${vtk_node_download_dir}")

# Download the file.
file(DOWNLOAD
  "${node_baseurl}/${node_file}"
  "${vtk_node_download_dir}/${node_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${node_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${node_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${node_file}"
  WORKING_DIRECTORY "${vtk_node_download_dir}"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${node_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  "${vtk_node_download_dir}/node-v${node_version}-${node_platform}"
  "${vtk_node_download_dir}/node")
