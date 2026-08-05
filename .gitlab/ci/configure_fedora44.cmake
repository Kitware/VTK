# Enable SPDX generation
set(VTK_GENERATE_SPDX ON CACHE BOOL "")

# Disable IOUSD module
set(VTK_MODULE_ENABLE_VTK_IOUSD NO CACHE STRING "") # usd

# Enable IOOpenVDB module (depends on blosc)
set(Blosc_INCLUDE_DIR "/usr/include" CACHE PATH "Path to Blosc include files")
set(Blosc_LIBRARY_RELEASE "/usr/lib64/libblosc.so" CACHE FILEPATH "Path to release Blosc library")
set(Blosc_LIBRARY_DEBUG "/usr/lib64/libblosc.so" CACHE FILEPATH "Path to debug Blosc library")
set(OpenVDB_CMAKE_PATH "/usr/local/lib64/cmake/OpenVDB" CACHE PATH "Path to OpenVDB CMake files")
set(OpenVDB_DIR "/usr/local/lib64/cmake/OpenVDB" CACHE PATH "Path to OpenVDB CMake files")
set(VTK_MODULE_ENABLE_VTK_IOOpenVDB WANT CACHE STRING "")

# Enable IONanoVDB module
set(VTK_MODULE_ENABLE_VTK_IONanoVDB WANT CACHE STRING "")

# Finding nanovdb requires a little help as it is not automatically found by CMake on all fedora configs
set(OpenVDB_nanovdb_INCLUDE_DIR "/usr/local/include/nanovdb" CACHE PATH "Path to NanoVDB include files")

# Disable wayland testing as CI machines do not have a wayland compositor.
set(VTK_USE_Wayland OFF CACHE BOOL "")

# Add rpath entries for dependencies.
set(CMAKE_INSTALL_RPATH "/usr/local/lib64:$ENV{CI_PROJECT_DIR}/.gitlab/qt6/lib" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
