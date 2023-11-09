if(TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkFiltersParallelStatisticsCxxTests-MPI tests
    TestPDescriptiveStatistics.cxx
    TestPCorrelativeStatistics.cxx
    TestRandomPContingencyStatisticsMPI.cxx
    TestRandomPKMeansStatisticsMPI.cxx
    TestRandomPMomentStatisticsMPI.cxx
    TestRandomPOrderStatisticsMPI.cxx
    )
  vtk_test_cxx_executable(vtkFiltersParallelStatisticsCxxTests-MPI tests)
endif()
