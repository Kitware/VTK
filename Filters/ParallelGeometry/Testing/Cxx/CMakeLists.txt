include(vtkMPI)

set(Tests_SRCS
  TestPStructuredGridConnectivity.cxx
  TestPStructuredGridGhostDataGenerator.cxx
  TestPUnstructuredGridGhostCellsGenerator.cxx
  TestPUniformGridGhostDataGenerator.cxx)

if (NOT VTK_LEGACY_REMOVE)
  # only add those tests if not removing legacy code.
  list(APPEND Tests_SRCS
    TestPUnstructuredGridConnectivity.cxx
    TestPUnstructuredGridGhostDataGenerator.cxx)
endif()

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests ${Tests_SRCS})

vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests
  UnstructuredGhostZonesCommon.cxx)
