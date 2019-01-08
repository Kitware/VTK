find_package(ADIOS1 QUIET)

vtk_add_test_mpi(vtkIOADIOSCxx-MPI tests
  NO_DATA NO_VALID NO_OUTPUT

  # Basic Create-write-read test
  TestADIOSSphereWR.cxx
)

vtk_test_cxx_executable(vtkIOADIOSCxx-MPI tests)
target_link_libraries(vtkIOADIOSCxx-MPI
  PRIVATE
    adios1::adios)
