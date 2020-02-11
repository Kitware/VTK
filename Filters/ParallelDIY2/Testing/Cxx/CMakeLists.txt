if (TARGET VTK::mpi)
  set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI tests
    TESTING_DATA
    TestPResampleToImageCompositeDataSet.cxx
    TestPResampleToImage.cxx
    TestPResampleWithDataSet2.cxx
    TestPResampleWithDataSet.cxx
    TestRedistributeDataSetFilter.cxx
  )

# We want 4 processes to test the vtkAggregateDataSetFilter
# properly.
  set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 4)
  vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI no_data_tests_4_procs
    DIYAggregateDataSet.cxx
    TestAdaptiveResampleToImage.cxx
    TestGenerateGlobalIds.cxx
    )

  set(all_tests
    ${tests}
    ${no_data_tests_4_procs}
    )

  vtk_test_cxx_executable(vtkFiltersParallelDIY2CxxTests-MPI all_tests)
endif()

# non-mpi tests
vtk_add_test_cxx(vtkFiltersParallelDIY2CxxTests non_mpi_tests
  TestExtractSubsetWithSeed.cxx
  TestAdaptiveResampleToImage.cxx,NO_VALID
  TestGenerateGlobalIds.cxx,NO_VALID
  TestRedistributeDataSetFilter.cxx,NO_VALID)
vtk_test_cxx_executable(vtkFiltersParallelDIY2CxxTests non_mpi_tests)
