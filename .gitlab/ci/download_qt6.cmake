cmake_minimum_required(VERSION 3.12)

# Input variables.
set(qt_version_major "6")
set(qt_version_minor "9")
set(qt_version_patch "0")
# This URL is only visible inside of Kitware's network. Please use your own Qt
# Account to obtain these files.
set(qt_url_root "https://paraview.org/files/dependencies/internal/qt")

# Mindeps isn't testing Qt6.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "mindeps")
  return ()
endif ()

# Only build for jobs which include Qt.
if (NOT "$ENV{CMAKE_CONFIGURATION}" MATCHES "qt")
  return ()
endif ()

# Determine the ABI to fetch for Qt.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "vs2022")
  set(qt_platform "windows_x86")
  set(msvc_year "2022")
  set(qt_abi "win64_msvc${msvc_year}_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  set(qt_platform "mac_x64")
  set(qt_abi "clang_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "^(el7|fedora)" AND
        "$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
  set(qt_platform "linux_x64")
  set(qt_abi "linux_gcc_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "^(el7|fedora)" AND
        "$ENV{CMAKE_CONFIGURATION}" MATCHES "aarch64")
  set(qt_platform "linux_arm64")
  set(qt_abi "linux_gcc_arm64")
else ()
  message(FATAL_ERROR
    "Unknown ABI to use for Qt")
endif ()

# Combined version variables.
set(qt_version "${qt_version_major}.${qt_version_minor}.${qt_version_patch}")
set(qt_version_nodot "${qt_version_major}${qt_version_minor}${qt_version_patch}")

# Files needed to download.
set(qt_files)
if (qt_platform STREQUAL "windows_x86")
  set(qt_build_stamp "202503301022")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")
  list(APPEND qt_files
    "qt.qt6.${qt_version_nodot}.${qt_abi}/${qt_file_name_prefix}d3dcompiler_47-x64.7z"
    "qt.qt6.${qt_version_nodot}.${qt_abi}/${qt_file_name_prefix}opengl32sw-64-mesa_11_2_2-signed_sha256.7z")

  foreach (qt_component IN ITEMS qtbase qtdeclarative qttools)
    list(APPEND qt_files
      "qt.qt6.${qt_version_nodot}${}.${qt_abi}/${qt_file_name_prefix}${qt_component}-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/clang_64")
elseif (qt_platform STREQUAL "mac_x64")
  set(qt_build_stamp "202503301021")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")

  foreach (qt_component IN ITEMS qtbase qtdeclarative qttools)
    list(APPEND qt_files
      "qt.qt6.${qt_version_nodot}.${qt_abi}/${qt_file_name_prefix}${qt_component}-MacOS-MacOS_14-Clang-MacOS-MacOS_14-X86_64-ARM64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/clang_64")
elseif (qt_platform STREQUAL "linux_x64")
  set(qt_build_stamp "202503301022")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")

  foreach (qt_component IN ITEMS qtbase qtdeclarative qttools)
    list(APPEND qt_files
      "qt.qt6.${qt_version_nodot}.${qt_abi}/${qt_file_name_prefix}${qt_component}-Linux-RHEL_8_10-GCC-Linux-RHEL_8_10-X86_64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/clang_64")
elseif (qt_platform STREQUAL "linux_arm64")
  set(qt_build_stamp "202503301022")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")

  foreach (qt_component IN ITEMS qtbase qtdeclarative qttools)
    list(APPEND qt_files
      "qt.qt6.${qt_version_nodot}.${qt_abi}/${qt_file_name_prefix}${qt_component}-Linux-Ubuntu_24_04-GCC-Linux-Ubuntu_24_04-AARCH64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/clang_64")
else ()
  message(FATAL_ERROR
    "Unknown files for ${qt_platform}")
endif ()

# Verify that we know what directory will be extracted.
if (NOT qt_subdir)
  message(FATAL_ERROR
    "The extracted subdirectory is not set")
endif ()

# Build up the path to the file to download.
set(qt_url_path "${qt_platform}/desktop/qt6_${qt_version_nodot}/qt6_${qt_version_nodot}")
set(qt_url_prefix "${qt_url_root}/${qt_url_path}")

# Include the file containing the hashes of the files that matter.
include("${CMAKE_CURRENT_LIST_DIR}/download_qt6_hashes.cmake")

file(MAKE_DIRECTORY ".gitlab/qt6")

# Download and extract each file.
foreach (qt_file IN LISTS qt_files)
  get_filename_component(qt_filename "${qt_file}" NAME)

  # Ensure we have a hash to verify.
  if (NOT DEFINED "${qt_filename}_hash")
    message(FATAL_ERROR
      "Unknown hash for ${qt_file}")
  endif ()

  # Download the file.
  file(DOWNLOAD
    "${qt_url_prefix}/${qt_file}"
    ".gitlab/${qt_filename}"
    STATUS download_status
    EXPECTED_HASH "SHA256=${${qt_filename}_hash}")

  # Check the download status.
  list(GET download_status 0 res)
  if (res)
    list(GET download_status 1 err)
    message(FATAL_ERROR
      "Failed to download ${qt_file}: ${err}")
  endif ()

  # Extract the file.
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}"
      -E tar
      xf "../${qt_filename}"
    WORKING_DIRECTORY ".gitlab/qt6"
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    ERROR_STRIP_TRAILING_WHITESPACE)
  if (res)
    message(FATAL_ERROR
      "Failed to extract ${qt_file}: ${err}")
  endif ()
endforeach ()
