cmake_minimum_required(VERSION 3.12)

set(node_version "24.0.1")
set(node_baseurl "https://vtk.org/files/support")
# Uncomment to test newer releases before mirroring at vtk.org
# set(node_baseurl "https://nodejs.org/download/release/v${node_version}")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(node_platform "win-x64")
  set(node_ext "zip")
  set(node_hash "8bbbc4860ce03f8a6cc7369039f6497be3157c4bde3ad54651cc22f5f3af0b40")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "linux")
  set(node_platform "linux-x64")
  set(node_ext "tar.gz")
  set(node_hash "58239e217440acffb3889954f1dc0977f46048c54f226e446280fc8feb8c5ab9")
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
