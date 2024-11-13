cmake_minimum_required(VERSION 3.12)

set(node_version "22.0.0")
set(node_url "https://vtk.org/files/support")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(node_platform "win-x64")
  set(node_ext "zip")
  set(node_hash "32d639b47d4c0a651ff8f8d7d41a454168a3d4045be37985f9a810cf8cef6174")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(node_platform "linux-x64")
  set(node_ext "tar.gz")
  set(node_hash "74bb0f3a80307c529421c3ed84517b8f543867709f41e53cd73df99e6442af4d")
else ()
  message(FATAL_ERROR
      "Unknown platform for node $ENV{CMAKE_CONFIGURATION}")
endif ()
set(node_file "node-v${node_version}-${node_platform}.${node_ext}")

# Download the file.
file(DOWNLOAD
  "${node_url}/${node_file}"
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
