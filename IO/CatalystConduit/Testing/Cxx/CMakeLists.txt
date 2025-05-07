vtk_add_test_cxx(vtkConduitCxxTests tests
  NO_VALID NO_OUTPUT
  TestDataObjectToConduit.cxx
  TestConduitSource.cxx)

if (TARGET VTK::AcceleratorsVTKmDataModel)
  vtk_add_test_cxx(vtkConduitCxxTests tests
    NO_VALID NO_OUTPUT
    TestConduitSourceDeviceMemory.cxx)
endif()


vtk_test_cxx_executable(vtkConduitCxxTests tests)

if (TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkConduitCxxTests-MPI mpitests
    TESTING_DATA NO_VALID NO_OUTPUT
    TestDataObjectToConduit.cxx
    TestConduitSource.cxx)

  if (TARGET VTK::AcceleratorsVTKmDataModel)
    vtk_add_test_mpi(vtkConduitCxxTests-MPI mpitests
      TESTING_DATA NO_VALID NO_OUTPUT
      TestConduitSourceDeviceMemory.cxx)
  endif()

  vtk_test_cxx_executable(vtkConduitCxxTests-MPI mpitests)
endif()

if (TARGET viskores::cuda)
  set_source_files_properties(TestConduitSourceDeviceMemory.cxx PROPERTIES LANGUAGE CUDA)


  #the tests aren't scoped as a child directory of vtkAcceleratorsVTKmDataModel
  #so we need to redo this logic
  viskores_get_cuda_flags(CMAKE_CUDA_FLAGS)

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")

  # When cuda is enabled VTK::AcceleratorsVTKmDataModel is built statically but with fpic
  # enabled so the tests are also built with fpic enabled
  set_target_properties(vtkConduitCxxTests PROPERTIES
    CUDA_ARCHITECTURES OFF
    POSITION_INDEPENDENT_CODE ON
    )
if (TARGET VTK::ParallelMPI)
  set_target_properties(vtkConduitCxxTests-MPI PROPERTIES
    CUDA_ARCHITECTURES OFF
    POSITION_INDEPENDENT_CODE ON
    )
endif()
endif()
