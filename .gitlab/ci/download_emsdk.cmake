cmake_minimum_required(VERSION 3.12)

# The implementation lives under CMake/ so that it also ships in the source
# archives, which exclude .gitlab/ via .gitattributes. It defaults to a per-user
# cache directory, which CI does not want: jobs start from a clean checkout and
# the artifact/cache machinery expects the tools under .gitlab.
if (DEFINED ENV{CI_PROJECT_DIR})
  set(vtk_project_dir "$ENV{CI_PROJECT_DIR}")
else ()
  set(vtk_project_dir "${CMAKE_CURRENT_LIST_DIR}/../..")
endif ()

set(vtk_emsdk_download_dir "${vtk_project_dir}/.gitlab")
include("${vtk_project_dir}/CMake/wasm/vtkDownloadEmsdk.cmake")
