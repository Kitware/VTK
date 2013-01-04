if (VTK_LARGE_DATA_ROOT)

  # Parallel tests
  add_test_mpi(PSLACReaderLinear.cxx DATADIR ${VTK_LARGE_DATA_ROOT})
  add_test_mpi(PSLACReaderQuadratic.cxx DATADIR ${VTK_LARGE_DATA_ROOT})

endif()
