vtk_add_test_tcl(TestPolyDataPieces.tcl Parallel)
if(VTK_DATA_ROOT)
  vtk_add_test_tcl(TestUnstructuredPieces.tcl Parallel)
endif()
