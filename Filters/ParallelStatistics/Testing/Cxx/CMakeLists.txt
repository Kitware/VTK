if(TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkFiltersParallelStatisticsCxxTests-MPI tests
    TestPDescriptiveStatistics.cxx
    TestPCorrelativeStatistics.cxx
    TestRandomPContingencyStatisticsMPI.cxx
    TestRandomPKMeansStatisticsMPI.cxx
    TestRandomPMomentStatisticsMPI.cxx
    TestRandomPOrderStatisticsMPI.cxx
  )
  if(TARGET VTK::IOParallelXML)
    # Test vtkGenerateStatistics with simple, contrived input data
    vtk_add_test_mpi(vtkFiltersParallelStatisticsCxxTests-MPI tests
      TESTING_DATA
      TestGenerateStatisticsSimple.cxx,NO_VALID
    )
    if(TARGET VTK::IOIOSS)
      # Test vtkGenerateStatistics with complex, real-world input data
      vtk_add_test_mpi(vtkFiltersParallelStatisticsCxxTests-MPI tests
        TESTING_DATA
        TestGenerateStatistics.cxx,NO_VALID
      )
    endif()
  endif()
  vtk_test_cxx_executable(vtkFiltersParallelStatisticsCxxTests-MPI tests)
endif()
