# We are splitting the tests into two executables to workaround an issue in
# cuda. With all the tests in the same executable several tests are failing
# in cuda. We have not identified the root cause of the problem yet.
vtk_add_test_cxx(vtkAcceleratorsVTKmDataModelCxxTests tests
  TestVTKMDataSet.cxx,NO_VALID
  )

if (TARGET vtkm::cuda)
  #the enable_language call is scoped! so we have to re-enable
  #cuda in the test directory
  enable_language(CUDA)

  foreach(src IN LISTS tests)
    string(REPLACE "," ";" src ${src})
    list(GET src 0 src)

    set_source_files_properties(${src} PROPERTIES
      LANGUAGE CUDA
      CUDA_SOURCE_PROPERTY_FORMAT OBJ
      CUDA_SEPARABLE_COMPILATION ON)
  endforeach()

  #the tests aren't scoped as a child directory of vtkAcceleratorsVTKmDataModel
  #so we need to redo this logic
  vtkm_get_cuda_flags(CMAKE_CUDA_FLAGS)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif()

vtk_test_cxx_executable(vtkAcceleratorsVTKmDataModelCxxTests tests
  RENDERING_FACTORY
  )

if (TARGET vtkm::cuda)
  # When cuda is enabled VTK::AcceleratorsVTKmDataModel is built statically but with fpic
  # enabled so the tests are also built with fpic enabled
  set_target_properties(vtkAcceleratorsVTKmDataModelCxxTests PROPERTIES
    CUDA_ARCHITECTURES OFF
    POSITION_INDEPENDENT_CODE ON
    )
endif()
