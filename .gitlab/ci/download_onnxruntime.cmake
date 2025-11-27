cmake_minimum_required(VERSION 3.12)

set(onnx_version "1.22.0")

# Determine the platform.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  set(onnx_platform "osx-arm64")
  set(onnx_ext "tgz")
  set(onnx_hash "cab6dcbd77e7ec775390e7b73a8939d45fec3379b017c7cb74f5b204c1a1cc07")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64")
  set(onnx_platform "osx-x86_64")
  set(onnx_ext "tgz")
  set(onnx_hash "e4ec94a7696de74fb1b12846569aa94e499958af6ffa186022cfde16c9d617f0")
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  set(onnx_platform "linux-x64")
  set(onnx_ext "tgz")
  set(onnx_hash "8344d55f93d5bc5021ce342db50f62079daf39aaafb5d311a451846228be49b3")
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "aarch64")
    set(onnx_platform "linux-aarch64")
    set(onnx_ext "tgz")
    set(onnx_hash "bb76395092d150b52c7092dc6b8f2fe4d80f0f3bf0416d2f269193e347e24702")
  endif()
elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "el")
  return()
else ()
  message(FATAL_ERROR "Unknown platform for onnxruntime")
endif ()

set(onnx_url "https://vtk.org/files/support")
set(onnx_file "onnxruntime-${onnx_platform}-${onnx_version}.${onnx_ext}")

# Download the file
file(DOWNLOAD
  "${onnx_url}/${onnx_file}"
  ".gitlab/${onnx_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${onnx_hash}")

# Check download status
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${onnx_file}: ${err}")
endif ()

# Extract the file
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${onnx_file}"
  WORKING_DIRECTORY ".gitlab"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${onnx_file}: ${err}")
endif ()

# Move to a predictable location
file(RENAME
  ".gitlab/onnxruntime-${onnx_platform}-${onnx_version}"
  ".gitlab/onnxruntime")

# The linux and osx binaries are broken and require some renaming and moving
# Moving files from onnxruntime/include to onnxruntime/include/onnxruntime
# Issue here : https://github.com/microsoft/onnxruntime/issues/25242
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora" OR
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_x86_64" OR
    "$ENV{CMAKE_CONFIGURATION}" MATCHES "macos_arm64")
  file(GLOB include_files
    ".gitlab/onnxruntime/include/*")

  file(MAKE_DIRECTORY ".gitlab/onnxruntime/include/onnxruntime")

  foreach(f ${include_files})
    get_filename_component(name "${f}" NAME)
    file(RENAME
      "${f}"
      ".gitlab/onnxruntime/include/onnxruntime/${name}")
  endforeach()
endif()

# Rename the lib directory to lib64
# Issue here : https://github.com/microsoft/onnxruntime/issues/25279
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "fedora")
  file(RENAME
  ".gitlab/onnxruntime/lib"
  ".gitlab/onnxruntime/lib64")
endif()
