# Modules which require software not on CI machines.
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # openslide
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # openturns
set(VTK_MODULE_ENABLE_VTK_FiltersReebGraph NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_GeovisGDAL NO CACHE STRING "") # gdal
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # adios
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOGDAL NO CACHE STRING "") # ffmpeg
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # liblas, boost
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # mysql
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # odbc
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # OpenVDB
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # pdal
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # postgresql
set(VTK_MODULE_ENABLE_VTK_InfovisBoost NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_RenderingExternal NO CACHE STRING "") # glut
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingMatplotlib NO CACHE STRING "") # matplotlib
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # openvr
set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # openxr
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # ospray
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # adios
set(VTK_MODULE_ENABLE_VTK_xdmf3 NO CACHE STRING "") # boost
set(VTK_MODULE_ENABLE_VTK_IOOCCT NO CACHE STRING "") # occt
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "") # catalyst

# Windows-only features
set(VTK_USE_MICROSOFT_MEDIA_FOUNDATION ON CACHE BOOL "")
set(VTK_USE_VIDEO_FOR_WINDOWS ON CACHE BOOL "")
set(VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE ON CACHE BOOL "")
set(VTK_DISABLE_QT_MULTICONFIG_WINDOWS_WARNING ON CACHE BOOL "")

# Set DLL paths for Python modules to work.
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python")
  file(TO_CMAKE_PATH "$ENV{CI_PROJECT_DIR}" ci_project_dir)
  set(vtk_dll_paths)
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "qt")
    list(APPEND vtk_dll_paths
      "${ci_project_dir}/.gitlab/qt/bin")
  endif ()
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "tbb")
    file(TO_CMAKE_PATH "$ENV{TBB_REDIST_DIR}" tbb_redist_dir)
    list(APPEND vtk_dll_paths
      "${ci_project_dir}/.gitlab/tbb/redist/${tbb_redist_dir}")
  endif ()
  set(VTK_DLL_PATHS
    ${vtk_dll_paths}
    CACHE STRING "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
