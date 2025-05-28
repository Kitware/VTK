get_property(vtk_smp_backends GLOBAL
  PROPERTY _vtk_smp_backends)

if ("OpenMP" IN_LIST vtk_smp_backends)
  set(Viskores_ENABLE_OPENMP ON)
else()
  set(Viskores_ENABLE_OPENMP OFF)
endif ()

if ("TBB" IN_LIST vtk_smp_backends)
  set(Viskores_ENABLE_TBB ON)
else()
  set(Viskores_ENABLE_TBB OFF)
endif ()

# Make sure Viskores is compiled for VTK types.
set(Viskores_USE_DEFAULT_TYPES_FOR_VTK ON)
set(Viskores_USE_DEFAULT_SYMBOL_VISIBILITY OFF)
set(Viskores_USE_DOUBLE_PRECISION OFF)
set(Viskores_USE_64BIT_IDS "${VTK_USE_64BIT_IDS}")
set(Viskores_ADD_XGC_TO_DEFAULT_TYPES_VTK OFF)
set(Viskores_USE_DEFAULT_TYPES_FOR_ASCENT OFF)

set(Viskores_ENABLE_BENCHMARKS OFF)
set(Viskores_ENABLE_CPACK OFF)
set(Viskores_ENABLE_CUDA "${VTK_USE_CUDA}")
set(Viskores_ENABLE_DOCUMENTATION OFF)
set(Viskores_ENABLE_EXAMPLES OFF)
set(Viskores_ENABLE_EXAMPLES_ISSET OFF)
set(Viskores_ENABLE_HDF5_IO OFF)
set(Viskores_ENABLE_KOKKOS "${VTK_USE_KOKKOS}")
set(Viskores_ENABLE_LOGGING ON)
set(Viskores_ENABLE_MPI OFF)
set(Viskores_ENABLE_RENDERING OFF)
set(Viskores_ENABLE_TESTING_LIBRARY ON)
set(Viskores_ENABLE_TESTING_LIBRARY_ISSET ON)
set(Viskores_ENABLE_TESTING OFF)
set(Viskores_ENABLE_TUTORIALS OFF)

set(Viskores_NO_ASSERT OFF)
set(Viskores_NO_ASSERT_CUDA ON)
set(Viskores_NO_ASSERT_HIP ON)

set(Viskores_HDF5_IS_PARALLEL OFF)

set(Viskores_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
set(Viskores_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkviskores/viskores")
if (DEFINED _vtk_build_LIBRARY_NAME_SUFFIX)
  set(Viskores_CUSTOM_LIBRARY_SUFFIX "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
endif ()
set(Viskores_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(Viskores_LIBRARY_OUTPUT_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(Viskores_BUILD_CMAKE_BASE_DIR "${CMAKE_BINARY_DIR}")
set(Viskores_INSTALL_CONFIG_DIR "${_vtk_build_CMAKE_DESTINATION}/viskores")
set(Viskores_INSTALL_CMAKE_MODULE_DIR "${Viskores_INSTALL_CONFIG_DIR}/cmake")
# Currently Viskores only installs its readme and license.
set(Viskores_INSTALL_SHARE_DIR "${_vtk_build_LICENSE_DESTINATION}/vtkviskores")

if (_vtk_build_INSTALL_HEADERS)
  set(Viskores_INSTALL_ONLY_LIBRARIES OFF)
else ()
  set(Viskores_INSTALL_ONLY_LIBRARIES ON)
endif ()

set(Viskores_BUILD_ALL_LIBRARIES ON)
set(Viskores_HIDE_PRIVATE_SYMBOLS ON)
set(Viskores_INSTALL_EXAMPLES OFF)
set(Viskores_INSTALL_EXAMPLES_ISSET ON)
set(Viskores_NO_INSTALL_README_LICENSE OFF)
set(Viskores_OVERRIDE_CTEST_TIMEOUT OFF)
set(Viskores_VERBOSE_MODULES OFF)

set(Viskores_SKIP_LIBRARY_VERSIONS "${VTK_WHEEL_BUILD}")

# Viskores uses stricter warning checks resulting in more warnings when Viskores
# is enabled.
# Disable Viskores warning flags and just rely on VTK's warning flags.
set(Viskores_ENABLE_DEVELOPER_FLAGS OFF)
set(Viskores_NO_DEPRECATED_VIRTUAL ON)

# Ignore existing CMake variable in viskores
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# viskores prior to v2.2 does not set rpath
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX)
  if (APPLE)
    list(APPEND CMAKE_INSTALL_RPATH "@loader_path")
  else ()
    list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")
  endif ()
endif ()

add_subdirectory(viskores)
viskores_setup_job_pool()

vtk_module_add_module(VTK::vtkviskores
  HEADER_ONLY)
# viskores_cont_testing does not refer to unit tests but to the viskores testing library.
vtk_module_link(VTK::vtkviskores INTERFACE viskores_cont viskores_cont_testing viskores_filter viskores_worklet)
# Use of VTK_KOKKOS_BACKEND is deprecated.
if(VTK_KOKKOS_BACKEND)
  if (VTK_KOKKOS_BACKEND STREQUAL "HIP" AND NOT TARGET viskores_kokkos_hip)
    message(FATAL_ERROR "VTK_KOKKOS_BACKEND=HIP but Kokkos HIP backend not found, exiting.")
  endif()
  if (VTK_KOKKOS_BACKEND STREQUAL "CUDA" AND NOT TARGET viskores_kokkos_cuda)
    message(FATAL_ERROR "VTK_KOKKOS_BACKEND=CUDA but Kokkos CUDA backend not found, exiting.")
  endif()
endif()

if (TARGET viskores_cuda)
  add_library("viskores::cuda" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("viskores::cuda" INTERFACE viskores_cuda)
endif()

if (TARGET viskores_kokkos_cuda)
  add_library("viskores::kokkos_cuda" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("viskores::kokkos_cuda" INTERFACE viskores_kokkos_cuda)
endif()

if (TARGET viskores_kokkos_hip)
  add_library("viskores::kokkos_hip" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("viskores::kokkos_hip" INTERFACE viskores_kokkos_hip)
endif()
