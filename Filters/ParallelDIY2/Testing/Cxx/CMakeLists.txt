if (TARGET VTK::mpi)

  set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI tests
    TESTING_DATA
    TestGhostCellsGenerator.cxx,NO_VALID
    TestOverlappingCellsDetector.cxx,NO_VALID
    TestPResampleHyperTreeGridWithDataSet.cxx
    TestPResampleToImageCompositeDataSet.cxx
    TestPResampleWithDataSet2.cxx
    TestPResampleWithDataSet.cxx
    TestProbeLineFilter.cxx,NO_VALID
    TestPUniformGridGhostDataGenerator.cxx,NO_VALID
    TestPUnstructuredGridGhostCellsGenerator.cxx,NO_VALID
    TestRedistributeDataSetFilter.cxx
    TestRedistributeDataSetFilterImplicitArray.cxx,NO_VALID
    TestRedistributeDataSetFilterOnIOSS.cxx,NO_VALID
    TestStructuredGridGhostDataGenerator.cxx,NO_VALID
    TestUnstructuredGridGeometryFilterGhostCells.cxx,NO_VALID)

  if(TARGET VTK::FiltersParallelMPI)
    vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI tests
      TESTING_DATA
      TestPResampleToImage.cxx)
  endif ()

  # We want 4 processes to test the vtkAggregateDataSetFilter properly.
  set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 4)
  vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI no_data_tests_4_procs
    DIYAggregateDataSet.cxx
    TestAdaptiveResampleToImage.cxx
    TestGenerateGlobalIds.cxx
    )

  # We want at least 5 processes to test the TestDIYGenerateCuts properly.
  # See https://gitlab.kitware.com/paraview/paraview/-/issues/21396
  set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 5)
  vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI no_data_tests_5_procs
    TestDIYGenerateCuts.cxx
    )

  set(all_tests
    ${tests}
    ${no_data_tests_4_procs}
    ${no_data_tests_5_procs}
    )

  vtk_test_cxx_executable(vtkFiltersParallelDIY2CxxTests-MPI all_tests)
endif()

# non-mpi tests
vtk_add_test_cxx(vtkFiltersParallelDIY2CxxTests non_mpi_tests
  TestAdaptiveResampleToImage.cxx,NO_VALID
  TestExtractSubsetWithSeed.cxx
  TestOverlappingCellsDetector.cxx,NO_VALID
  TestGenerateGlobalIds.cxx,NO_VALID
  TestGenerateGlobalIdsSphere.cxx,NO_VALID
  TestRedistributeDataSetFilter.cxx,NO_VALID
  TestRedistributeDataSetFilterOnIOSS.cxx,NO_VALID
  TestRedistributeDataSetFilterWithPolyData.cxx
  TestStitchImageDataWithGhosts.cxx, NO_VALID
  TestUniformGridGhostDataGenerator.cxx,NO_VALID)
vtk_test_cxx_executable(vtkFiltersParallelDIY2CxxTests non_mpi_tests)
