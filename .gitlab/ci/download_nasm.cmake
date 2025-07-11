cmake_minimum_required(VERSION 3.12)

# Input variables.
set(nasm_version "2.16.03")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(nasm_platform "win64")
  set(nasm_hash "3ee4782247bcb874378d02f7eab4e294a84d3d15f3f6ee2de2f47a46aa7226e6")
  set(nasm_executable_extension ".exe")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
    message(WARNING
      "NASM is only needed for the x86_64 architecture")
    return ()
  endif ()
  set(nasm_platform "macosx")
  set(nasm_hash "0d29bcd8a5fc617333f4549c7c1f93d1866a4a0915c40359e0a8585bb1a5aa75")
else ()
  message(FATAL_ERROR
      "Unknown platform for nasm $ENV{CMAKE_CONFIGURATION}")
endif ()

set(nasm_url_root "https://paraview.org/files/dependencies/")
set(nasm_file "nasm-${nasm_version}-${nasm_platform}.zip")

# Download the file.
file(DOWNLOAD
  "${nasm_url_root}/${nasm_file}"
  ".gitlab/nasm/${nasm_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${nasm_hash}")

# Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${nasm_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${nasm_file}"
  WORKING_DIRECTORY ".gitlab/nasm"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${nasm_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/nasm/nasm-${nasm_version}/nasm${nasm_executable_extension}"
  ".gitlab/nasm/nasm${nasm_executable_extension}")
