#[==[
Downloads and extracts the Emscripten SDK.

Run it with `cmake -P`, optionally passing `-Dvtk_emsdk_download_dir=<dir>`. The
archive is extracted to `<vtk_emsdk_download_dir>/emsdk`; the directory defaults
to a version-stamped subdirectory of the per-user tool cache, so that several
versions coexist (see vtkToolCacheDirectory.cmake). CI passes `.gitlab` instead,
which is unversioned because CI jobs start from a clean checkout.

This lives under `CMake/` rather than `.gitlab/ci/` because `.gitattributes`
marks `.git*` as `export-ignore`, so `.gitlab/` is absent from the source
archives published on vtk.org. The wasm toolchain file has to be able to
bootstrap an SDK from one of those archives too.
#]==]
cmake_minimum_required(VERSION 3.12)

# Input variables.
include("${CMAKE_CURRENT_LIST_DIR}/vtkToolVersions.cmake")
set(emsdk_version "${vtk_emsdk_version}")
set(emsdk_ext "zip")
set(emsdk_hash "${vtk_emsdk_hash}")
set(emsdk_suffix "${emsdk_version}.${emsdk_ext}")

set(emsdk_url "https://github.com/emscripten-core/emsdk/archive/refs/tags")
set(emsdk_file "emsdk-${emsdk_suffix}")

if (NOT DEFINED vtk_emsdk_download_dir)
  include("${CMAKE_CURRENT_LIST_DIR}/vtkToolCacheDirectory.cmake")
  set(vtk_emsdk_download_dir "${vtk_tool_cache_directory}/emsdk-${emsdk_version}")
endif ()
file(MAKE_DIRECTORY "${vtk_emsdk_download_dir}")

# Download the file.
file(DOWNLOAD
  "${emsdk_url}/${emsdk_suffix}"
  "${vtk_emsdk_download_dir}/${emsdk_file}"
  STATUS download_status
  EXPECTED_HASH "SHA256=${emsdk_hash}")

  # Check the download status.
list(GET download_status 0 res)
if (res)
  list(GET download_status 1 err)
  message(FATAL_ERROR
    "Failed to download ${emsdk_file}: ${err}")
endif ()

# Extract the file.
execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E tar
    xf "${emsdk_file}"
  WORKING_DIRECTORY "${vtk_emsdk_download_dir}"
  RESULT_VARIABLE res
  ERROR_VARIABLE err
  ERROR_STRIP_TRAILING_WHITESPACE)
if (res)
  message(FATAL_ERROR
    "Failed to extract ${emsdk_file}: ${err}")
endif ()

# Move to a predictable prefix.
file(RENAME
  "${vtk_emsdk_download_dir}/emsdk-${emsdk_version}"
  "${vtk_emsdk_download_dir}/emsdk")
