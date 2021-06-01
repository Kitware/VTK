cmake_minimum_required(VERSION 3.12)

# Wheels do not build against Qt.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "wheel")
  return ()
endif ()

# Input variables.
set(qt_version_major "5")
set(qt_version_minor "15")
set(qt_version_patch "2")
# This URL is only visible inside of Kitware's network. Please use your own Qt
# Account to obtain these files.
set(qt_url_root "https://paraview.org/files/dependencies/internal/qt")

# Determine the ABI to fetch for Qt.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "vs2015")
  set(qt_platform "windows_x86")
  set(msvc_year "2015")
  set(qt_abi "win64_msvc${msvc_year}_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "vs2017" OR
        "$ENV{CMAKE_CONFIGURATION}" MATCHES "vs2019")
  set(qt_platform "windows_x86")
  set(msvc_year "2019")
  set(qt_abi "win64_msvc${msvc_year}_64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(qt_platform "mac_arm64")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(qt_platform "mac_x64")
  set(qt_abi "clang_64")
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
  set(qt_build_stamp "202011130602")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")
  list(APPEND qt_files
    "${qt_file_name_prefix}d3dcompiler_47-x64.7z"
    "${qt_file_name_prefix}opengl32sw-64-mesa_12_0_rc2.7z")

  foreach (qt_component IN ITEMS qtbase qttools qtdeclarative qtquickcontrols2)
    list(APPEND qt_files
      "${qt_file_name_prefix}${qt_component}-Windows-Windows_10-MSVC${msvc_year}-Windows-Windows_10-X86_64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/msvc${msvc_year}_64")
elseif (qt_platform STREQUAL "mac_x64")
  set(qt_build_stamp "202011130601")
  set(qt_file_name_prefix "${qt_version}-0-${qt_build_stamp}")

  foreach (qt_component IN ITEMS qtbase qttools qtdeclarative qtquickcontrols2)
    list(APPEND qt_files
      "${qt_file_name_prefix}${qt_component}-MacOS-MacOS_10_13-Clang-MacOS-MacOS_10_13-X86_64.7z")
  endforeach ()

  set(qt_subdir "${qt_version}/clang_64")
elseif (qt_platform STREQUAL "mac_arm64")
  set(qt_subdir "qt-${qt_version}-macosx11.0-arm64")
  set(qt_files "${qt_subdir}.tar.xz")
  set("${qt_files}_hash" "31796cf6dbbc3ae867862636127795a3b463a655b15f7c663d188eb31218ac6f")
  set(qt_url_prefix "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/qt/v${qt_version}-20210524.0") # XXX: see below
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
if (NOT qt_url_prefix) # XXX: Replace this when Qt ships official arm64 binaries.
  set(qt_url_path "${qt_platform}/desktop/qt5_${qt_version_nodot}/qt.qt5.${qt_version_nodot}.${qt_abi}")
  set(qt_url_prefix "${qt_url_root}/${qt_url_path}")
endif ()

# Include the file containing the hashes of the files that matter.
include("${CMAKE_CURRENT_LIST_DIR}/download_qt_hashes.cmake")

# Download and extract each file.
foreach (qt_file IN LISTS qt_files)
  # Ensure we have a hash to verify.
  if (NOT DEFINED "${qt_file}_hash")
    message(FATAL_ERROR
      "Unknown hash for ${qt_file}")
  endif ()

  # Download the file.
  file(DOWNLOAD
    "${qt_url_prefix}/${qt_file}"
    ".gitlab/${qt_file}"
    STATUS download_status
    EXPECTED_HASH "SHA256=${${qt_file}_hash}")

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
      xf "${qt_file}"
    WORKING_DIRECTORY ".gitlab"
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    ERROR_STRIP_TRAILING_WHITESPACE)
  if (res)
    message(FATAL_ERROR
      "Failed to extract ${qt_file}: ${err}")
  endif ()
endforeach ()

# The Windows tarballs have some unfortunate permissions in them that prevent
# deletion when `git clean -ffdx` tries to clean up the directory.
if (qt_platform STREQUAL "windows_x86")
  # Fix permissions.
  file(TO_NATIVE_PATH ".gitlab/${qt_subdir}/*.*" native_qt_dir)
  execute_process(
    # Remove any read-only flags that aren't affected by `icacls`.
    COMMAND
      attrib
      -r # Remove readonly flag
      "${native_qt_dir}"
      /d # Treat as a directory
      /s # Recursive
      /l # Don't dereference symlinks
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    ERROR_STRIP_TRAILING_WHITESPACE)
  if (res)
    message(FATAL_ERROR
      "Failed to fix remove read-only flags in ${qt_file}: ${err}")
  endif ()
endif ()

# Move to a predictable prefix.
file(RENAME
  ".gitlab/${qt_subdir}"
  ".gitlab/qt")
