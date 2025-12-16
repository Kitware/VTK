set(classes
  vtkConduitSource
  vtkConduitToDataObject
  vtkDataObjectToConduit
  vtkConduitArrayUtilities
)

set(private_classes)
if (TARGET VTK::AcceleratorsVTKmDataModel)
  set(private_classes_cuda vtkConduitArrayUtilitiesDevice)
  set(private_classes ${private_classes} ${private_classes_cuda})
  set(classes_cuda vtkConduitArrayUtilities ${private_classes_cuda})
endif()

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes})


if (TARGET viskores::cuda)
  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")

  set_source_files_properties(${classes_cuda} PROPERTIES LANGUAGE CUDA)

  vtk_module_set_properties(VTK::IOCatalystConduit
    CUDA_SEPARABLE_COMPILATION ON)

  vtk_module_compile_options(VTK::IOCatalystConduit
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
endif ()


vtk_add_test_mangling(VTK::IOCatalystConduit)
add_subdirectory(Catalyst)
