include(vtkMPI)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TestRandomPContingencyStatisticsMPI.cxx
  TestRandomPKMeansStatisticsMPI.cxx
  TestRandomPMomentStatisticsMPI.cxx
  TestRandomPOrderStatisticsMPI.cxx
  # # The following file was being compiled but never added as a testin older
  # # vtk. Why is that so?
  # TestRealDataPDescriptiveStatisticsMPI.cxx
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
