vtk_add_test_cxx(vtkFiltersParallelCxxTests testsStd
  TestAngularPeriodicFilter.cxx
  )
vtk_test_cxx_executable(vtkFiltersParallelCxxTests testsStd)

if (TARGET VTK::ParallelMPI)
  set(vtkFiltersParallelCxxTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI tests
    TESTING_DATA
    DistributedData.cxx
    DistributedDataRenderPass.cxx
    PTextureMapToSphere.cxx
    TestPExtractDataArraysOverTime.cxx,NO_VALID
    TransmitImageData.cxx
    TransmitImageDataRenderPass.cxx
    TransmitRectilinearGrid.cxx
    TransmitStructuredGrid.cxx
    )
  vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI no_data_tests
    ParallelResampling.cxx
    PCellDataToPointData.cxx
    UnitTestPMaskPoints.cxx
    )

  # We want 4 processes to test the vtkAggregateDataSetFilter
  # properly
  set(vtkFiltersParallelCxxTests-MPI_NUMPROCS 4)
  vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI no_data_tests_4_procs
    AggregateDataSet.cxx
    )


  set(all_tests
    ${tests}
    ${no_data_tests}
    ${no_data_tests_4_procs}
    )
  vtk_test_cxx_executable(vtkFiltersParallelCxxTests-MPI all_tests)
endif()
