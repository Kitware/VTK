set(_known_little_endian FALSE)
if (DEFINED VTK_WORDS_BIGENDIAN)
  if (NOT VTK_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()

if (enable_large_data_tests)
  set(TestPWindBladeReader_NUMPROCS 1)
  vtk_add_test_mpi(vtkIOMPIParallelCxxTests-MPI tests
    TESTING_DATA
    TestPWindBladeReader.cxx
    )
  vtk_test_cxx_executable(vtkIOMPIParallelCxxTests-MPI tests)
endif()
