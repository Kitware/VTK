if (VTK_LARGE_DATA_ROOT)
  add_test_mpi(PLSDynaReader.cxx DATADIR ${VTK_LARGE_DATA_ROOT})
endif()
