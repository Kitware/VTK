if (VTK_LARGE_DATA_ROOT)
  vtk_add_test_mpi(PLSDynaReader.cxx DATADIR ${VTK_LARGE_DATA_ROOT})
endif()
