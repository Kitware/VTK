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
    # All macOS wheel interpreters come from python-build-standalone (via
    # uv), which targets 10.15 for x86_64.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
  elseif ("$ENV{CMAKE_CONFIGURATION}" MATCHES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
  endif ()
endif ()

# Is this a free-threading python dist?
if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "(linux|macos|windows)([0-9]+)t")
  set(python_version_no_dot "${CMAKE_MATCH_2}")

  # Python xarray deps not available yet for free threading
  set(VTK_MODULE_ENABLE_VTK_IONetCDF NO CACHE STRING "")

  # CPython in Windows shares `pyconfig.h` between the regular and
  # free-threaded ABIs; the build system must define `Py_GIL_DISABLED` itself:
  # https://docs.python.org/3/howto/free-threading-extensions.html
  # The CI CMake also predates `FindPython`'s free-threading support, so point
  # it at the free-threaded import library explicitly.
  # XXX(ci-cmake): 3.30
  if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "windows")
    # Setting `CMAKE_C_FLAGS`/`CMAKE_CXX_FLAGS` directly as CACHE entries here
    # (before `project()` runs) would pre-empt CMake's own MSVC default flags,
    # since it only populates them from `CMAKE_<LANG>_FLAGS_INIT` when the
    # cache entry doesn't already exist; `add_compile_definitions()` has no
    # effect at all from a `-C` script (it never reaches the generated build
    # files). Go through `CFLAGS`/`CXXFLAGS` instead: CMake's own
    # `CMAKE_<LANG>_FLAGS_INIT` logic prepends the environment variable, so
    # this combines with the MSVC defaults rather than replacing them.
    set(ENV{CFLAGS} "$ENV{CFLAGS} -DPy_GIL_DISABLED=1")
    set(ENV{CXXFLAGS} "$ENV{CXXFLAGS} -DPy_GIL_DISABLED=1")
    set(Python3_LIBRARY "$ENV{PYTHON_PREFIX}/libs/python${python_version_no_dot}t.lib" CACHE FILEPATH "")
  endif()
endif()

set(VTK_WHEEL_BUILD ON CACHE BOOL "")
set(VTK_INSTALL_SDK ON CACHE BOOL "")

set(CMAKE_PREFIX_PATH "$ENV{PYTHON_PREFIX}" CACHE STRING "")
set(Python3_EXECUTABLE "$ENV{PYTHON_PREFIX}/${python_subdir}python$ENV{PYTHON_VERSION_SUFFIX}" CACHE FILEPATH "")
# We always want the Python specified here, not the system one.
set(Python3_FIND_STRATEGY LOCATION CACHE STRING "")

# Official wheels never include remote modules (because they are not under
# VTK's software process).
set(VTK_ENABLE_REMOTE_MODULES OFF CACHE BOOL "")

# Disable debug leaks in wheels.
set(VTK_DEBUG_LEAKS OFF CACHE BOOL "")

# Enable `.pyi` files.
set(VTK_BUILD_PYI_FILES ON CACHE BOOL "")

# Disable modules we cannot build for wheels.
set(VTK_GROUP_ENABLE_Qt NO CACHE STRING "") # Qt
set(VTK_MODULE_ENABLE_VTK_CommonArchive NO CACHE STRING "") # libarchive
set(VTK_MODULE_ENABLE_VTK_DomainsMicroscopy NO CACHE STRING "") # OpenSlide
set(VTK_MODULE_ENABLE_VTK_FiltersONNX NO CACHE STRING "") # onnxruntime
set(VTK_MODULE_ENABLE_VTK_FiltersOpenTURNS NO CACHE STRING "") # OpenTURNS
set(VTK_MODULE_ENABLE_VTK_FiltersReebGraph NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_IOADIOS2 NO CACHE STRING "") # ADIOS2
set(VTK_MODULE_ENABLE_VTK_IOAlembic NO CACHE STRING "") # alembic
set(VTK_MODULE_ENABLE_VTK_IOFFMPEG NO CACHE STRING "") # FFMPEG
set(VTK_MODULE_ENABLE_VTK_IOGDAL NO CACHE STRING "") # GDAL
set(VTK_MODULE_ENABLE_VTK_IOLAS NO CACHE STRING "") # liblas
set(VTK_MODULE_ENABLE_VTK_IOMySQL NO CACHE STRING "") # MariaDB
set(VTK_MODULE_ENABLE_VTK_IONanoVDB NO CACHE STRING "") # NanoVDB
set(VTK_MODULE_ENABLE_VTK_IOODBC NO CACHE STRING "") # odbc
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB NO CACHE STRING "") # OpenVDB
set(VTK_MODULE_ENABLE_VTK_IOPDAL NO CACHE STRING "") # PDAL
set(VTK_MODULE_ENABLE_VTK_IOPostgreSQL NO CACHE STRING "") # PostgreSQL
set(VTK_MODULE_ENABLE_VTK_InfovisBoost NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig NO CACHE STRING "") # fontconfig
set(VTK_MODULE_ENABLE_VTK_RenderingOpenVR NO CACHE STRING "") # OpenVR
set(VTK_MODULE_ENABLE_VTK_conduit NO CACHE STRING "") # conduit

# PCH causes issues on macOS CI due to issues from sccache.
# sccache issue: https://github.com/mozilla/sccache/issues/2558
# vtk issue: https://gitlab.kitware.com/vtk/vtk/-/issues/19903
set(VTK_USE_PCH OFF CACHE BOOL "")

if(NOT WIN32)
  set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # OpenXR disable on every system except Windows
endif()

set(VTK_MODULE_ENABLE_VTK_RenderingZSpace NO CACHE STRING "") # zSpace
set(VTK_MODULE_ENABLE_VTK_fides NO CACHE STRING "") # ADIOS2
set(VTK_MODULE_ENABLE_VTK_xdmf3 NO CACHE STRING "") # Boost
set(VTK_MODULE_ENABLE_VTK_IOOCCT NO CACHE STRING "") # occt
set(VTK_MODULE_ENABLE_VTK_IOIFC NO CACHE STRING "") # IFC based on IfcOpenShell
set(VTK_MODULE_ENABLE_VTK_IOUSD NO CACHE STRING "") # usd
set(VTK_ENABLE_CATALYST OFF CACHE BOOL "") # catalyst

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
