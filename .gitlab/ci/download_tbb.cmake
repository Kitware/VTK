cmake_minimum_required(VERSION 3.12)

# Input variables.
set(tbb_version "2021.1.1")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(tbb_platform "win")
  set(tbb_ext "zip")
  set(tbb_hash "544e61a490ceafd6756f000926850685ccaf59dbdf01ec5f2bef4855a3b50974")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  set(tbb_platform "mac")
  set(tbb_ext "tgz")
  set(tbb_hash "c17ce0773401acfe17f4f2b6c6fb798694daa5763c0d13e2f20ba3bc8f27d339")
else ()
  message(FATAL_ERROR
    "Unknown platform for TBB")
endif ()

set(tbb_url_root "https://github.com/oneapi-src/oneTBB/releases/download/v${tbb_version}")
set(tbb_file "oneapi-tbb-${tbb_version}-${tbb_platform}.${tbb_ext}")
set(tbb_subdir "oneapi-tbb-${tbb_version}")

# Download the file.
file(DOWNLOAD
  "${tbb_url_root}/${tbb_file}"
  ".gitlab/${tbb_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${tbb_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${tbb_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${tbb_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${tbb_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/${tbb_subdir}"
  ".gitlab/tbb")
