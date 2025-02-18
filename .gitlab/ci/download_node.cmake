cmake_minimum_required(VERSION 3.12)

set(node_version "23.8.0")
set(node_baseurl "https://vtk.org/files/support")
# Uncomment to test newer releases before mirroring at vtk.org
# set(node_baseurl "https://nodejs.org/download/release/v${node_version}")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(node_platform "win-x64")
  set(node_ext "zip")
  set(node_hash "9e03646224fcf44fa0b594df5d012da9cb5b137c52f36f33b11def3319cd132c")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(node_platform "linux-x64")
  set(node_ext "tar.gz")
  set(node_hash "f6d98bbdd0a4078c1e422a6a1d8bf07ad693a4dd793bb5772990456faeca6e95")
else ()
  message(FATAL_ERROR
      "Unknown platform for node $ENV{CMAKE_CONFIGURATION}")
endif ()
set(node_file "node-v${node_version}-${node_platform}.${node_ext}")

# Download the file.
file(DOWNLOAD
  "${node_baseurl}/${node_file}"
  ".gitlab/${node_file}"
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
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${node_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/node-v${node_version}-${node_platform}"
  ".gitlab/node")
