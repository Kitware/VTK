include(vtkMPI)

set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI tests
  TESTING_DATA
  TestPResampleToImage.cxx
  TestPResampleToImageCompositeDataSet.cxx
  TestPResampleWithDataSet.cxx
  TestPResampleWithDataSet2.cxx
)

# We want 4 processes to test the vtkAggregateDataSetFilter
# properly. Note that this test may fail if the MALLOC_PERTURB_ environment
# variable is set due to what appear to be issues with MPI. This test has been disabled
# on Taanab because of this.
set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 4)
vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI no_data_tests_4_procs
  DIYAggregateDataSet.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests_4_procs}
  )

vtk_test_mpi_executable(vtkFiltersParallelDIY2CxxTests-MPI all_tests)
