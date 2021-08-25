if (NOT DEFINED "ENV{PYTHON_PREFIX}")
  message(FATAL_ERROR
    "The `PYTHON_PREFIX` environment variable is required.")
endif ()

set(python_subdir "bin/")
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
  set(python_subdir "")
endif ()

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "macos")
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "x86_64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "")
  elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
  endif ()
endif ()

set(VTK_WHEEL_BUILD ON CACHE BOOL "")
set(CMAKE_PREFIX_PATH "$ENV{PYTHON_PREFIX}" CACHE STRING "")
set(Python3_EXECUTABLE "$ENV{PYTHON_PREFIX}/${python_subdir}python$ENV{PYTHON_VERSION_SUFFIX}" CACHE FILEPATH "")
# We always want the Python specified here, not the system one.
set(Python3_FIND_STRATEGY LOCATION CACHE STRING "")

# Official wheels never include remote modules (because they are not under
# VTK's software process).
set(VTK_ENABLE_REMOTE_MODULES OFF CACHE BOOL "")

# Disable debug leaks in wheels.
set(VTK_DEBUG_LEAKS OFF CACHE BOOL "")

# Disable modules we cannot build for wheels.
set(VTK_GROUP_ENABLE_Qt NO CACHE STRING "") # Qt
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # OpenSlide
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # OpenTURNS
set(VTK_MODULE_ENABLE_VTK_FiltersReebGraph NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # ADIOS2
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # FFMPEG
set(VTK_MODULE_ENABLE_VTK_IOGDAL NO CACHE STRING "") # GDAL
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # liblas
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # MariaDB
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # odbc
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # OpenVDB
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # PDAL
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # PostgreSQL
set(VTK_MODULE_ENABLE_VTK_InfovisBoost NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # OpenVR
set(VTK_MODULE_ENABLE_VTK_RenderingRayTracing NO CACHE STRING "") # OSPRay
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # ADIOS2
set(VTK_MODULE_ENABLE_VTK_xdmf3 NO CACHE STRING "") # Boost

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
