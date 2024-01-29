if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vtk-m/CMakeLists.txt")
  message(FATAL_ERROR
    "VTKm requested, but the VTKm submodule is not initialized. Please run "
    "'git submodule update --init --recursive' in the source directory.")
endif ()

get_property(vtk_smp_backends GLOBAL
  PROPERTY _vtk_smp_backends)

if ("OpenMP" IN_LIST vtk_smp_backends)
  set(VTKm_ENABLE_OPENMP ON)
endif ()

if ("TBB" IN_LIST vtk_smp_backends)
  set(VTKm_ENABLE_TBB ON)
endif ()

set(VTKm_ENABLE_CUDA "${VTK_USE_CUDA}")
set(VTKm_ENABLE_KOKKOS "${VTK_USE_KOKKOS}")

set(VTKm_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
set(VTKm_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkvtkm/vtk-m")
if (DEFINED _vtk_build_LIBRARY_NAME_SUFFIX)
  set(VTKm_CUSTOM_LIBRARY_SUFFIX "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
endif ()
set(VTKm_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(VTKm_LIBRARY_OUTPUT_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(VTKm_BUILD_CMAKE_BASE_DIR "${CMAKE_BINARY_DIR}")
set(VTKm_INSTALL_CONFIG_DIR "${_vtk_build_CMAKE_DESTINATION}/vtkm")
set(VTKm_INSTALL_CMAKE_MODULE_DIR "${VTKm_INSTALL_CONFIG_DIR}/cmake")
# Currently VTK-m only installs its readme and license.
set(VTKm_INSTALL_SHARE_DIR "${_vtk_build_LICENSE_DESTINATION}/vtkvtkm")

if (_vtk_build_INSTALL_HEADERS)
  set(VTKm_INSTALL_ONLY_LIBRARIES OFF)
else ()
  set(VTKm_INSTALL_ONLY_LIBRARIES ON)
endif ()

# Make sure VTK-m is compiled for VTK types.
set(VTKm_USE_DEFAULT_TYPES_FOR_VTK ON)

set(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY OFF)
set(VTKm_ENABLE_RENDERING OFF)
set(VTKm_ENABLE_TESTING OFF)
set(VTKm_ENABLE_TESTING_LIBRARY ON)
set(VTKm_ENABLE_BENCHMARKS OFF)
set(VTKm_ENABLE_DOCUMENTATION OFF)
set(VTKm_ENABLE_EXAMPLES OFF)
set(VTKm_ENABLE_CPACK OFF)
set(VTKm_USE_DOUBLE_PRECISION OFF)
set(VTKm_USE_64BIT_IDS "${VTK_USE_64BIT_IDS}")
set(VTKm_NO_ASSERT ON)

set(VTKm_SKIP_LIBRARY_VERSIONS "${VTK_WHEEL_BUILD}")

# VTK-m uses stricter warning checks resulting in more warnings when VTK-m
# is enabled.
# Disable VTK-m warning flags and just rely on VTK's warning flags.
set(VTKm_ENABLE_DEVELOPER_FLAGS OFF)
set(VTKm_NO_DEPRECATED_VIRTUAL ON)

# Ignore existing CMake variable in vtk-m
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# vtk-m prior to v2.2 does not set rpath
set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
if (UNIX)
  if (APPLE)
    list(APPEND CMAKE_INSTALL_RPATH "@loader_path")
  else ()
    list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")
  endif ()
endif ()

add_subdirectory(vtk-m)
vtkm_setup_job_pool()

vtk_module_add_module(VTK::vtkvtkm
  HEADER_ONLY)
# vtkm_cont_testing does not refer to unit tests but to the vtkm testing library.
vtk_module_link(VTK::vtkvtkm INTERFACE vtkm_cont vtkm_cont_testing vtkm_filter)
if(VTK_USE_KOKKOS)
  if (VTK_KOKKOS_BACKEND STREQUAL "HIP" AND NOT TARGET vtkm_kokkos_hip)
    message(FATAL_ERROR "VTK_KOKKOS_BACKEND=HIP but Kokkos HIP backend not found, exiting.")
  endif()
  if (VTK_KOKKOS_BACKEND STREQUAL "CUDA" AND NOT TARGET vtkm_kokkos_cuda)
    message(FATAL_ERROR "VTK_KOKKOS_BACKEND=CUDA but Kokkos CUDA backend not found, exiting.")
  endif()
endif()

if (TARGET vtkm_cuda)
  add_library("vtkm::cuda" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("vtkm::cuda" INTERFACE vtkm_cuda)
endif()

if (TARGET vtkm_kokkos_cuda)
  add_library("vtkm::kokkos_cuda" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("vtkm::kokkos_cuda" INTERFACE vtkm_kokkos_cuda)
endif()

if (TARGET vtkm_kokkos_hip)
  add_library("vtkm::kokkos_hip" INTERFACE IMPORTED GLOBAL)
  target_link_libraries("vtkm::kokkos_hip" INTERFACE vtkm_kokkos_hip)
endif()
