add_test_tcl(TestPolyDataPieces.tcl Parallel)
if(VTK_DATA_ROOT)
  add_test_tcl(TestUnstructuredPieces.tcl Parallel)
endif()
