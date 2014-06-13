if(VTK_USE_LARGE_DATA)
  include(vtkMPI)

  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/LSDyna/hemi.draw/,REGEX:.*}"
    )

  vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
    TESTING_DATA
    PLSDynaReader.cxx
    )
  vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
endif()
