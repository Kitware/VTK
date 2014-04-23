vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestUsePiston.cxx
)
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

if(VTK_MPI_MAX_NUMPROCS GREATER 1)
  include(vtkMPI)

  set(${vtk-module}Cxx-MPI_NUMPROCS 2)
  vtk_add_test_mpi(${vtk-module}Cxx-MPI tests
    TESTING_DATA
    TestDMPFiltering.cxx
    TestCompositeRender.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests)

endif()
