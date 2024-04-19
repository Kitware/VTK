cmake_minimum_required(VERSION 3.12)

# Where Python for wheels are stored.
set(python_url_root "https://www.paraview.org/files/dependencies/python-for-wheels")

# Python version specifics.
set(python38_version "3.8.10") # Source-only releases after this.
set(python39_version "3.9.13") # Source-only releases after this.
set(python310_version "3.10.11") # Source-only releases after this.
set(python311_version "3.11.6")
set(python312_version "3.12.0")

# Hashes for various deployments.
set(python38_windows_x86_64_hash "d4e7e83de0db659697eea034dd2b1620ff26ac7062709b60d723307a68aa5d81")
set(python39_windows_x86_64_hash "004683810c0e0b4ff10025392ac95e699e99d8c3566f415aa7fa35c6d4882f88")
set(python310_windows_x86_64_hash "b02692c7905dea2829e4204eab2343b226f0c9f244df89502ba8d483d5f8f9d3")
set(python311_windows_x86_64_hash "2a8393087e0cac9e3c0eeb69a3a34f3b01734266a0bb276621aec7a159b576b6")
set(python312_windows_x86_64_hash "782f1b9db7e8ff78c928ea94861549820c8abc70cde76a3dfb7ef9e54a06e326")

set(python38_macos_x86_64_hash "8c49fa50d34529e58769d3901e9e079554424d59bc1aa7dceb82c8c63f09cbc1")
set(python39_macos_arm64_hash "e6b95bb926feff99e38bcd4986feb8897b36170a6c6c01b36da7d8e3daac5b6b")
set(python39_macos_x86_64_hash "357fffe2efe80eef7136362db2e6616341c046dac5e26614478c7c0248c16709")
set(python310_macos_arm64_hash "5e5a2124abfdc3bb85751e6a544ab81d0624473afe7bab41a7cb78c72e3ccc8d")
set(python310_macos_x86_64_hash "edb762a34ea20c6876f9f583158e7f65c1a428aecea1971717fc05e26415f55b")
set(python311_macos_arm64_hash "ad1380cd383e0bab7d988f9ee7bfb8693d5de28736ad3d8d256937fad7d02ec7")
set(python311_macos_x86_64_hash "6dba6a561f02ae1a93707dd87386f2adca4a4765a9a73929c121290489ca14db")
set(python312_macos_arm64_hash "475b1626769b6e517eb0630accba08c4669c4e16573c6d4137cef0aaee67b785")
set(python312_macos_x86_64_hash "22772ed975c06f931c9b9f1d633afe34b04fbab6d0824d9a46a698d561ee5e90")

# Extracting information from the build configuration.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(python_platform "windows")
  set(python_ext "zip")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  set(python_platform "macos")
  set(python_ext "tar.xz")
else ()
  message(FATAL_ERROR
    "Unknown platform for Python")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "38_")
  set(python_version 38)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "39_")
  set(python_version 39)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "310_")
  set(python_version 310)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "311_")
  set(python_version 311)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "312_")
  set(python_version 312)
else ()
  message(FATAL_ERROR
    "Unknown version for Python")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
  set(python_arch "x86_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
  set(python_arch "arm64")
else ()
  message(FATAL_ERROR
    "Unknown architecture for Python")
endif ()

# Figure out what file we're supposed to download.
set(python_subdir "python-${python${python_version}_version}-${python_platform}-${python_arch}")
set(filename "${python_subdir}.${python_ext}")
set(sha256sum "${python${python_version}_${python_platform}_${python_arch}_hash}")

# Verify that we have a hash to validate.
if (NOT sha256sum)
  message(FATAL_ERROR
    "Unsupported configuration ${python_platform}/${python_arch} ${python${python_version}_version}")
endif ()

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
