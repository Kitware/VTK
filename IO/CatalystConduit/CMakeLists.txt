set(classes
  vtkConduitSource
  vtkConduitToDataObject
  vtkDataObjectToConduit)

if (TARGET vtkm::cuda)
  list(APPEND classes vtkConduitArrayUtilities)
endif()

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes})

if (VTK_KOKKOS_BACKEND STREQUAL "HIP")
  set(VTK_KOKKOS_BACKEND_HIP 1)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkDeviceMemoryType.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkDeviceMemoryType.h"
  @ONLY)


if (TARGET vtkm::cuda)
  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")

  list(TRANSFORM nowrap_classes APPEND ".cxx" OUTPUT_VARIABLE nowrap_sources)
  set(cuda_impl ${nowrap_sources})
  set_source_files_properties(${cuda_impl} PROPERTIES LANGUAGE CUDA)

  vtk_module_set_properties(VTK::IOCatalystConduit
    CUDA_SEPARABLE_COMPILATION ON)

  vtk_module_compile_options(VTK::IOCatalystConduit
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)

  find_package(CUDAToolkit REQUIRED)
  vtk_module_link(VTK::IOCatalystConduit PRIVATE CUDA::cudart)
endif ()


vtk_add_test_mangling(VTK::IOCatalystConduit)
add_subdirectory(Catalyst)
