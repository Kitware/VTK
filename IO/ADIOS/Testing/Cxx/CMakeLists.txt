find_package(ADIOS REQUIRED)
include_directories(SYSTEM ${ADIOS_INCLUDE_DIRS})

vtk_add_test_mpi(${vtk-module}Cxx-MPI tests
  NO_DATA NO_VALID NO_OUTPUT

  # Basic Create-write-read test
  TestADIOSSphereWR.cxx
)

vtk_test_mpi_executable(${vtk-module}Cxx-MPI tests)
