cmake_minimum_required(VERSION 3.12)

# Where Python for wheels are stored.
set(python_url_root "https://www.paraview.org/files/dependencies/python-for-wheels")

# Python version specifics.
set(python39_version "3.9.13") # Source-only releases after this.
set(python310_version "3.10.11") # Source-only releases after this.
set(python311_version "3.11.9") # Source-only releases after this.
set(python312_version "3.12.7")
set(python313_version "3.13.0")
set(python314_version "3.14.0")

# Hashes for various deployments.
set(python39_windows_x86_64_hash "004683810c0e0b4ff10025392ac95e699e99d8c3566f415aa7fa35c6d4882f88")
set(python310_windows_x86_64_hash "b02692c7905dea2829e4204eab2343b226f0c9f244df89502ba8d483d5f8f9d3")
set(python311_windows_x86_64_hash "d2e7567c29d4c02b708f42a2ed0be51859df42566faee4df844b5ee00094b8a1")
set(python312_windows_x86_64_hash "f4edfaa23ee00a9b1afc8072ea823d485496637cfeb8129057e92d05f1b80a80")
set(python313_windows_x86_64_hash "b733a8c7d8d30aa5d0742c00de419294ec385797586672076c337885a440d701")
set(python314_windows_x86_64_hash "1cf39a0c36aa6047f0982deaf400dbb327e676be60aedeef274088520a5a2887")

set(python39_macos_arm64_hash "e6b95bb926feff99e38bcd4986feb8897b36170a6c6c01b36da7d8e3daac5b6b")
set(python39_macos_x86_64_hash "357fffe2efe80eef7136362db2e6616341c046dac5e26614478c7c0248c16709")
set(python310_macos_arm64_hash "5e5a2124abfdc3bb85751e6a544ab81d0624473afe7bab41a7cb78c72e3ccc8d")
set(python310_macos_x86_64_hash "edb762a34ea20c6876f9f583158e7f65c1a428aecea1971717fc05e26415f55b")
set(python311_macos_arm64_hash "ea62d4a6b7b3c397280994d6204227095188c8641671286bf5a220ec95ccc637")
set(python311_macos_x86_64_hash "c296ba92c7fa47bb2953d466846ca46ff9d6f822a7b342afbd2e17ba4c0f16d8")
set(python312_macos_arm64_hash "866db20c9153509bc2edff7799d96ed2a467e48efd1a18c43cf9c84f8b922522")
set(python312_macos_x86_64_hash "96d3149615bf76e1ea0ba2d6fa5590ad3d4f7dbe039647a98a9ccfdba6642742")
set(python313_macos_arm64_hash "0833901d0b91c5c59bfeff3e155e3bc0bfe41d4ba730e94e5f6db4f6eb72cbe4")
set(python313_macos_x86_64_hash "367962d9e5e7cb7346e78840d806c117cae12830fa57e739f88d5bb7b052924e")
set(python314_macos_arm64_hash "51e27f2ee656cd578cd95caa8178e7f0e058514e1bb01357f597cf6a2805cd83")
set(python314_macos_x86_64_hash "5da3ede989b800d5997df0a160b7dc5dc15bf8e98233f9dafb8763fd83e7e17f")

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

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "39_")
  set(python_version 39)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "310_")
  set(python_version 310)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "311_")
  set(python_version 311)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "312_")
  set(python_version 312)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "313_")
  set(python_version 313)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "314_")
  set(python_version 314)
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
