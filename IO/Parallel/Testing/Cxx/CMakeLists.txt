if (VTK_LARGE_DATA_ROOT)

  vtk_tests(TestWindBladeReader.cxx DATADIR ${VTK_LARGE_DATA_ROOT} BASELINEDIR Parallel)

  # Parallel tests
  add_test_mpi(PSLACReaderLinear.cxx DATADIR ${VTK_LARGE_DATA_ROOT})
  add_test_mpi(PSLACReaderQuadratic.cxx DATADIR ${VTK_LARGE_DATA_ROOT})

endif()
