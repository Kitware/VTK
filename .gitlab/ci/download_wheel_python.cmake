cmake_minimum_required(VERSION 3.12)

# Where Python for wheels are stored.
set(python_url_root "https://www.paraview.org/files/dependencies/python-for-wheels")

# Python version specifics.
set(python310_version "3.10.11") # Source-only releases after this.
set(python311_version "3.11.9") # Source-only releases after this.
set(python312_version "3.12.7")
set(python313_version "3.13.0")
set(python314_version "3.14.0")

# Windows only; macOS Python (including free-threaded builds) comes from
# python-build-standalone via `uv` instead (see `.gitlab/ci/python_macos.sh`).
set(python314t_version "3.14.2t")

# Hashes for various deployments.
set(python310_windows_x86_64_hash "b02692c7905dea2829e4204eab2343b226f0c9f244df89502ba8d483d5f8f9d3")
set(python311_windows_x86_64_hash "d2e7567c29d4c02b708f42a2ed0be51859df42566faee4df844b5ee00094b8a1")
set(python312_windows_x86_64_hash "f4edfaa23ee00a9b1afc8072ea823d485496637cfeb8129057e92d05f1b80a80")
set(python313_windows_x86_64_hash "b733a8c7d8d30aa5d0742c00de419294ec385797586672076c337885a440d701")
set(python314_windows_x86_64_hash "1cf39a0c36aa6047f0982deaf400dbb327e676be60aedeef274088520a5a2887")
set(python314t_windows_x86_64_hash "28a94269cf82bd3a4dab3229e7ebe9357e777c012b4a65608c7737b74b3b10b1")

# Extracting information from the build configuration.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(python_platform "windows")
  set(python_ext "zip")
else ()
  message(FATAL_ERROR
    "Unknown platform for Python")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "310_")
  set(python_version 310)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "311_")
  set(python_version 311)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "312_")
  set(python_version 312)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "313_")
  set(python_version 313)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "314_")
  set(python_version 314)
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "314t_")
  set(python_version 314t)
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
